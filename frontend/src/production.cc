#include "production.hh"
#include "ELog.hh"

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/select.h>

#include "TApplication.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TStopwatch.h"  

using namespace std;

// 💡 [UX 강화] 직관적이고 아름다운 Usage 출력 함수
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

void ProductionAnalyzer::ShowEvent(int dumpIdx) {
    if (dumpIdx < 0 || dumpIdx >= _nEntries) return;
    _tIn->GetEntry(dumpIdx);

    int nCh = _evtData->GetNChannels();
    if(nCh == 0) return;
    int dataPoints = _evtData->GetChannel(0)->GetDataPoints();
    
    cout << "\n\033[1;36m=== Dump Block " << dumpIdx << " / " << _nEntries - 1 << " (Showing 1st Event) ===\033[0m\n";

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
        for(int pt=0; pt<dataPoints; pt++) wave[pt] = ch->GetSample(0, pt); 

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
            getline(cin, cmd); // 💡 cin >> 대신 getline을 사용하여 ENTER 키 완벽 인식

            if (cmd.empty() || cmd == "n" || cmd == "N") {
                if (currEntry < _nEntries - 1) currEntry++;
                ShowEvent(currEntry);
            }
            else if (cmd == "p" || cmd == "P") {
                if (currEntry > 0) currEntry--;
                ShowEvent(currEntry);
            }
            else if (cmd == "j" || cmd == "J") {
                int dest; 
                cout << "Jump to Dump Block number (0 ~ " << _nEntries - 1 << "): "; 
                string destStr; getline(cin, destStr);
                try {
                    dest = stoi(destStr);
                    if (dest >= 0 && dest < _nEntries) { currEntry = dest; ShowEvent(currEntry); }
                    else { cout << "\033[1;31m[ERROR] Out of range!\033[0m\n"; }
                } catch(...) { cout << "\033[1;31m[ERROR] Invalid input!\033[0m\n"; }
            }
            else if (cmd == "q" || cmd == "Q") {
                cout << "\n\033[1;33mUser requested exit.\033[0m\n";
                break;
            }
            cout << "\033[1;36mEvent " << currEntry << " Loaded.\033[0m [ENTER]: Next | [q]: Quit | [j]: Jump -> " << std::flush;
        }
        usleep(10000); // 💡 10ms 대기: ROOT 캔버스가 얼지 않게 하면서 CPU 100% 폭주 방지
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

    // 💡 [UX 강화] 인자 부족 시 PrintUsage() 호출
    if(inFile == "") { PrintUsage(); return 1; }
    
    // 💡 [버그 픽스] X11 환경 변수 체크 (Core Dump 방어)
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
