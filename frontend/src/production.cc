#include "production.hh"
#include "ELog.hh"

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/select.h>
#include <vector>

#include "TApplication.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TStopwatch.h"  

using namespace std;

// 💡 [버그픽스] 덩어리(Block) 데이터를 개별 이벤트로 풀어주는 전역 인덱싱 맵
static std::vector<std::pair<int, int>> s_eventMap; 

// 💡 직관적이고 아름다운 Usage 출력 함수
void PrintUsage() {
    std::cout << "\n\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[1;32m      NKVME_FADC400 - Offline Production & Analysis Tool\033[0m\n";
    std::cout << "\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[1;33mUsage:\033[0m production_nfadc400 -i <raw_file.root> [options]\n\n";
    std::cout << "\033[1;37m[Required]\033[0m\n";
    std::cout << "  -i <file>    : Input raw ROOT file path\n\n";
    std::cout << "\033[1;37m[Optional]\033[0m\n";
    std::cout << "  -o <file>    : Output processed ROOT file path (default: _prod.root)\n";
    std::cout << "  -w           : Save full waveforms in the output tree (Warning: Large File)\n";
    std::cout << "  -d           : Interactive Event Display Mode (Visual Waveform Debugger)\n";
    std::cout << "\033[1;36m======================================================================\033[0m\n\n";
}

bool kbhit() {
    struct timeval tv = { 0L, 0L }; fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

ProductionAnalyzer::ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform) 
    : _isValid(false), _useDisplay(useDisplay), _saveWaveform(saveWaveform), _nEntries(0),
      _fIn(nullptr), _tIn(nullptr), _evtData(nullptr), _runInfo(nullptr),
      _fOut(nullptr), _tOut(nullptr), _runHeader(nullptr), _pmtArray(nullptr),
      _canvasEvent(nullptr) 
{
    for(int i=0; i<MAX_CH; i++) { 
        _histWave[i] = nullptr; _lineBsl[i] = nullptr; 
    }

    _fIn = new TFile(inFile, "READ");
    if (!_fIn || _fIn->IsZombie()) { ELog::Print(ELog::FATAL, Form("Cannot open input file: %s", inFile)); return; }

    _runInfo = (RunInfo*)_fIn->Get("RunInfo");
    _tIn = (TTree*)_fIn->Get("FADC");
    if (!_tIn) { ELog::Print(ELog::FATAL, "Tree 'FADC' not found."); return; }

    _evtData = new RawData();
    _tIn->SetBranchAddress("RawData", &_evtData);
    _nEntries = _tIn->GetEntries(); 
    
    if (!_useDisplay) {
        _fOut = new TFile(outFile, "RECREATE");
        _runHeader = new Run(_runInfo); _runHeader->Write();

        _tOut = new TTree("PROD", "Processed Data Tree");
        _pmtArray = new TClonesArray("Pmt", 100);
        _tOut->Branch("Pmt", &_pmtArray);
        
        if (_saveWaveform) ELog::Print(ELog::INFO, "Waveform (-w) mode ON. Saving full time-series into Pmt._wave array.");
    } else {
        gStyle->SetOptStat(0);
        _canvasEvent = new TCanvas("cEvent", "NoticeDAQ Interactive Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }
    _isValid = true;
    ELog::Print(ELog::INFO, Form("Opened %s. Total Dump Blocks: %d", inFile, _nEntries));
}

ProductionAnalyzer::~ProductionAnalyzer() {
    if (_evtData) { _evtData->Clear("C"); delete _evtData; }
    if (_pmtArray) { _pmtArray->Clear("C"); delete _pmtArray; }
    if (_runHeader) { delete _runHeader; }
    
    for(int i=0; i<MAX_CH; i++) {
        if(_histWave[i]) delete _histWave[i];
        if(_lineBsl[i]) delete _lineBsl[i];
    }
    if (_canvasEvent) delete _canvasEvent;
    if (_fOut) { _fOut->Close(); delete _fOut; }
    if (_fIn)  { _fIn->Close();  delete _fIn; }
}

void ProductionAnalyzer::AnalyzeWaveform(const vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, float* outWave) {
    int ndp = wave.size();
    if (ndp < 30) { bsl = 0; amp = 0; time = 0; charge = 0; return; }

    int ped_s = 2; int ped_e = std::min(40, ndp / 5); int sig_s = ped_e + 5;
    double pedSum = 0; int nPed = 0;
    for(int i = ped_s; i <= ped_e; i++){ pedSum += wave[i]; nPed++; }
    bsl = (nPed > 0) ? pedSum / nPed : 0;
    
    double qSum = 0; double maxAmp = -9999; int maxIdx = -1;

    for (int i = 0; i < ndp; i++) {
        double val = (double)wave[i];
        double dropAmp = bsl - val; 
        
        if (i >= sig_s) {
            if (dropAmp > 0) qSum += dropAmp; 
            if (dropAmp > maxAmp) { maxAmp = dropAmp; maxIdx = i; }
        }
        if (outWave) outWave[i] = (float)dropAmp;  
    }
    amp = maxAmp; charge = qSum; time = maxIdx * 2.5; 
}

void ProductionAnalyzer::RunBatch() {
    if (!_isValid || _useDisplay) return;

    ELog::Print(ELog::INFO, "Starting Batch Processing (Flattening Dump Blocks)...");
    TStopwatch sw; sw.Start(); 
    int totalEvents = 0;
    
    for (int dumpIdx = 0; dumpIdx < _nEntries; dumpIdx++) {
        _tIn->GetEntry(dumpIdx);
        
        int nCh = _evtData->GetNChannels();
        if(nCh == 0) continue;
        
        int nEventsInDump = _evtData->GetChannel(0)->GetNumEvents();
        int dataPoints = _evtData->GetChannel(0)->GetDataPoints();

        for (int evtIdx = 0; evtIdx < nEventsInDump; evtIdx++) {
            _pmtArray->Clear("C"); 

            for (int i = 0; i < nCh; i++) {
                RawChannel* ch = _evtData->GetChannel(i);
                if (!ch) continue;
                
                int chId = ch->GetChId();
                if (chId < 0 || chId >= MAX_CH) continue;

                Pmt* pmt = (Pmt*)_pmtArray->ConstructedAt(i);
                pmt->SetId(chId);

                if (_saveWaveform) pmt->AllocateWaveform(dataPoints);

                std::vector<unsigned short> wave(dataPoints);
                for(int pt=0; pt<dataPoints; pt++) wave[pt] = ch->GetSample(evtIdx, pt);

                double bsl, amp, time, charge;
                AnalyzeWaveform(wave, bsl, amp, time, charge, _saveWaveform ? pmt->Waveform() : nullptr);

                pmt->SetPedMean(bsl); pmt->SetQmax(amp); pmt->SetQtot(charge); pmt->SetFmax(time);
            }
            _tOut->Fill(); 
            totalEvents++;
        }

        if (dumpIdx % 10 == 0 && dumpIdx > 0) {
            double curTime = sw.RealTime(); sw.Continue();
            double rate = totalEvents / curTime;
            printf("\r\033[1;32m[PROD]\033[0m Processing Dump: %d / %d | Flattened Events: %d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m", dumpIdx, _nEntries, totalEvents, rate);
            fflush(stdout);
        }
    }
    
    double totalTime = sw.RealTime();
    double avgSpeed = (totalTime > 0) ? (totalEvents / totalTime) : 0.0;
    printf("\r\033[1;32m[PROD]\033[0m Processing Dump: %d / %d | Flattened Events: %d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m\n", _nEntries, _nEntries, totalEvents, avgSpeed);
    
    _fOut->cd(); _tOut->AutoSave();

    std::cout << "\n\033[1;35m╔═══════════════════ PRODUCTION SUMMARY ════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35d\033[0m \033[1;35m║\033[0m", "Total Processed", totalEvents) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35.2f sec\033[0m \033[1;35m║\033[0m", "Total Elapsed Time", totalTime) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;32m%-35.2f evts/s\033[0m \033[1;35m║\033[0m", "Average Speed", avgSpeed) << std::endl;
    std::cout << "\033[1;35m╚═══════════════════════════════════════════════════════════╝\033[0m\n" << std::endl;
    ELog::Print(ELog::INFO, "Batch Processing Completed. Saved to output file.");
}

// 💡 [핵심 버그 픽스] dumpIdx가 아닌 "글로벌 이벤트 인덱스"를 받아서 처리하도록 개조
void ProductionAnalyzer::ShowEvent(int globalIdx) {
    if (globalIdx < 0 || globalIdx >= (int)s_eventMap.size()) return;
    
    // 맵에서 이 이벤트가 속한 덩어리(Block)와 그 안에서의 번호(Sub-event)를 꺼냄
    int targetDumpIdx = s_eventMap[globalIdx].first;
    int targetEvtIdx  = s_eventMap[globalIdx].second;

    _tIn->GetEntry(targetDumpIdx);

    int nCh = _evtData->GetNChannels();
    if(nCh == 0) return;
    int dataPoints = _evtData->GetChannel(0)->GetDataPoints();
    
    cout << "\n\033[1;36m=== Global Event " << globalIdx << " / " << s_eventMap.size() - 1 
         << " (Dump Block: " << targetDumpIdx << ", Sub-event: " << targetEvtIdx << ") ===\033[0m\n";

    for (int i = 0; i < nCh && i < MAX_CH; i++) {
        RawChannel* ch = _evtData->GetChannel(i);
        if (!ch) continue;
        int chId = ch->GetChId();
        if (chId < 0 || chId >= MAX_CH) continue;

        if (!_histWave[chId]) {
            _histWave[chId] = new TH1F(Form("h_ev_ch%d", chId), Form("Channel %d (Inverted);Time (ns);Voltage Drop (ADC)", chId), dataPoints, 0, dataPoints * 2.5);
            _histWave[chId]->SetDirectory(nullptr); 
            _histWave[chId]->SetLineColor(kBlue + 1);
            _histWave[chId]->SetFillColorAlpha(kBlue - 9, 0.3); 
        }
        if (_histWave[chId]->GetNbinsX() != dataPoints) _histWave[chId]->SetBins(dataPoints, 0, dataPoints * 2.5);
        _histWave[chId]->Reset();
        
        std::vector<unsigned short> wave(dataPoints);
        // 💡 [버그 픽스] 강제 0번 파형이 아닌, 매핑된 서브 이벤트 인덱스의 파형을 가져옴!
        for(int pt=0; pt<dataPoints; pt++) wave[pt] = ch->GetSample(targetEvtIdx, pt); 

        double bsl, amp, time, charge;
        AnalyzeWaveform(wave, bsl, amp, time, charge, nullptr); 

        double minV = 99999, maxV = -99999;
        for (int pt = 0; pt < dataPoints; pt++) {
            double drop = bsl - wave[pt]; 
            _histWave[chId]->SetBinContent(pt + 1, drop);
            if(drop > maxV) maxV = drop;
            if(drop < minV) minV = drop;
        }
        if (maxV <= minV) { maxV = minV + 100; minV = minV - 100; }

        if (i < 4) { 
            _canvasEvent->cd(i + 1);
            double margin = (maxV - minV) * 0.1;
            if(margin < 10) margin = 10;
            _histWave[chId]->GetYaxis()->SetRangeUser(minV - margin, maxV + margin);
            _histWave[chId]->Draw("HIST");

            if (_lineBsl[chId]) delete _lineBsl[chId];
            _lineBsl[chId] = new TLine(0, 0, dataPoints * 2.5, 0); 
            _lineBsl[chId]->SetLineColor(kRed); _lineBsl[chId]->SetLineStyle(2); _lineBsl[chId]->SetLineWidth(2);
            _lineBsl[chId]->Draw();
        }
        printf(" \033[1;33m[Ch %d]\033[0m Bsl: %6.1f | Amp: %6.1f | \033[1;32mTime: %6.1f ns\033[0m | Charge: %6.1f \n", chId, bsl, amp, time, charge);
    }
    _canvasEvent->Update();
}

void ProductionAnalyzer::RunInteractive() {
    if (!_isValid || !_useDisplay) return;
    
    // 💡 [버그 픽스] 시작 시 파일을 스캔하여 모든 이벤트의 매핑 주소를 메모리에 기록
    s_eventMap.clear();
    std::cout << "\033[1;33mIndexing Events for Interactive Mode...\033[0m" << std::flush;
    for (int i = 0; i < _nEntries; i++) {
        _tIn->GetEntry(i);
        if (_evtData->GetNChannels() == 0) continue;
        int nEvtsInDump = _evtData->GetChannel(0)->GetNumEvents();
        for (int j = 0; j < nEvtsInDump; j++) {
            s_eventMap.push_back({i, j}); // {Dump 블록 번호, 덩어리 내 서브 번호}
        }
    }
    int totalEvts = s_eventMap.size();
    std::cout << " Done. Found " << totalEvts << " events.\n";

    if (totalEvts == 0) {
        std::cout << "\033[1;31m[ERROR] No valid events found in this file!\033[0m\n";
        return;
    }

    int currEntry = 0; 
    ShowEvent(currEntry);

    std::cout << "\n\033[1;35m========================================================\033[0m\n";
    std::cout << "\033[1;32m   [ Interactive Display Mode Activated ]\033[0m\n";
    std::cout << "   -> \033[1;33m[ENTER]\033[0m   : Next Event (다음 이벤트로 이동)\n";
    std::cout << "   -> \033[1;36m[j]\033[0m       : Jump to Event (특정 이벤트로 워프)\n";
    std::cout << "   -> \033[1;31m[q]\033[0m       : Quit (종료)\n";
    std::cout << "\033[1;35m========================================================\033[0m\n\n";

    std::cout << "\033[1;36mEvent " << currEntry << " Loaded.\033[0m [ENTER]: Next | [q]: Quit | [j]: Jump -> " << std::flush;

    while(true) {
        gSystem->ProcessEvents(); 

        if (kbhit()) {
            string cmd;
            getline(cin, cmd); 

            if (cmd.empty() || cmd == "n" || cmd == "N") {
                if (currEntry < totalEvts - 1) currEntry++;
                ShowEvent(currEntry);
            }
            else if (cmd == "p" || cmd == "P") {
                if (currEntry > 0) currEntry--;
                ShowEvent(currEntry);
            }
            else if (cmd == "j" || cmd == "J") {
                int dest; 
                cout << "Jump to Global Event number (0 ~ " << totalEvts - 1 << "): "; 
                string destStr; getline(cin, destStr);
                try {
                    dest = stoi(destStr);
                    if (dest >= 0 && dest < totalEvts) { currEntry = dest; ShowEvent(currEntry); }
                    else { cout << "\033[1;31m[ERROR] Out of range!\033[0m\n"; }
                } catch(...) { cout << "\033[1;31m[ERROR] Invalid input!\033[0m\n"; }
            }
            else if (cmd == "q" || cmd == "Q") {
                cout << "\n\033[1;33mUser requested exit.\033[0m\n";
                break;
            }
            cout << "\033[1;36mEvent " << currEntry << " Loaded.\033[0m [ENTER]: Next | [q]: Quit | [j]: Jump -> " << std::flush;
        }
        usleep(10000); 
    }
}

int main(int argc, char ** argv) {
    TString inFile = ""; TString outFile = "prod.root";
    bool useDisplay = false; bool saveWaveform = false; 
    
    int opt;
    while((opt = getopt(argc, argv, "i:o:dw")) != -1) {
        switch(opt) {
            case 'i': inFile = optarg; break;
            case 'o': outFile = optarg; break;
            case 'd': useDisplay = true; break;
            case 'w': saveWaveform = true; break;
        }
    }

    if(inFile == "") { PrintUsage(); return 1; }
    
    if (useDisplay && gSystem->Getenv("DISPLAY") == nullptr) {
        std::cout << "\n\033[1;31m[FATAL ERROR]\033[0m No X11 DISPLAY found!\n";
        std::cout << "Cannot open Interactive Canvas in Headless mode.\n";
        std::cout << "Please reconnect via SSH using '\033[1;33mssh -X\033[0m' or '\033[1;33mssh -Y\033[0m'.\n\n";
        return 1;
    }

    if(outFile == "prod.root" && inFile != "") {
        TString s(inFile); s.ReplaceAll(".root", "_prod.root"); outFile = s;
    }

    if (useDisplay) {
        TApplication app("ProdApp", &argc, argv);
        ProductionAnalyzer analyzer(inFile.Data(), outFile.Data(), true, saveWaveform);
        analyzer.RunInteractive();
    } else {
        ProductionAnalyzer analyzer(inFile.Data(), outFile.Data(), false, saveWaveform);
        analyzer.RunBatch();
    }

    return 0;
}
