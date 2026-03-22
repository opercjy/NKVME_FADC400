#include "production.hh"
#include "ELog.hh"

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>
#include <iomanip>

#include "TApplication.h"
#include "TSystem.h"
#include "TStyle.h"
#include "TStopwatch.h"  

using namespace std;

// =======================================================================
// Constructor & Destructor
// =======================================================================
ProductionAnalyzer::ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform) 
    : _isValid(false), _useDisplay(useDisplay), _saveWaveform(saveWaveform), _nEntries(0),
      _fpIn(nullptr), _evtData(nullptr), _runInfo(nullptr),
      _fOut(nullptr), _tOut(nullptr), _runHeader(nullptr), _pmtArray(nullptr),
      _canvasEvent(nullptr) 
{
    for(int i=0; i<MAX_CH; i++) { 
        _histWave[i] = nullptr; 
        _lineBsl[i] = nullptr; 
        _hQtot[i] = nullptr;
        _wTime[i] = new std::vector<double>();
        _wDrop[i] = new std::vector<double>();
    }

    // 💡 [Phase 6] ROOT 파일 대신 순수 바이너리(.dat) 파일 열기
    _fpIn = fopen(inFile, "rb");
    if (!_fpIn) {
        ELog::Print(ELog::FATAL, Form("Cannot open binary input file: %s", inFile));
        return;
    }

    fseek(_fpIn, 0, SEEK_END);
    _fileSize = ftell(_fpIn);
    fseek(_fpIn, 0, SEEK_SET);

    _evtData = new RawData();
    _runInfo = new RunInfo(); // ※ 필요시 별도의 Config에서 RunInfo 복원 로직 추가 요망

    if (!_useDisplay) {
        _fOut = new TFile(outFile, "RECREATE");
        _runHeader = new Run(_runInfo);
        _runHeader->Write();

        _tOut = new TTree("PROD", "Processed Data Tree (Optimized)");
        _pmtArray = new TClonesArray("Pmt", 100);
        _tOut->Branch("Pmt", &_pmtArray);
        
        if (_saveWaveform) {
            ELog::Print(ELog::INFO, "Waveform (-w) mode ON. Saving full time-series into the tree.");
            for(int i=0; i<MAX_CH; i++) {
                _tOut->Branch(Form("wTime_Ch%d", i), &_wTime[i]);
                _tOut->Branch(Form("wDrop_Ch%d", i), &_wDrop[i]);
            }
        }
        
        for(int i=0; i<MAX_CH; i++) {
            _hQtot[i] = new TH1F(Form("hQtot_Ch%d", i), Form("Charge Distribution Ch%d;Charge (ADC sum);Counts", i), 1000, 0, 50000);
        }
    } 
    else {
        gStyle->SetOptStat(0);
        _canvasEvent = new TCanvas("cEvent", "NoticeDAQ Interactive Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }

    _isValid = true;
    ELog::Print(ELog::INFO, Form("Opened Binary Stream: %s (Size: %.2f MB)", inFile, _fileSize / 1048576.0));
}

ProductionAnalyzer::~ProductionAnalyzer() {
    if (_evtData) { _evtData->Clear("C"); delete _evtData; }
    if (_pmtArray) { _pmtArray->Clear("C"); delete _pmtArray; }
    if (_runHeader) { delete _runHeader; }
    if (_runInfo) { delete _runInfo; }
    
    for(int i=0; i<MAX_CH; i++) {
        if(_histWave[i]) delete _histWave[i];
        if(_lineBsl[i]) delete _lineBsl[i];
        if(_wTime[i]) delete _wTime[i];
        if(_wDrop[i]) delete _wDrop[i];
    }
    if (_canvasEvent) delete _canvasEvent;

    if (_fOut) { _fOut->Close(); delete _fOut; }
    if (_fpIn) { fclose(_fpIn); }
}

// =======================================================================
// 💡 [Phase 6] 바이너리 스트림에서 1개 이벤트 디코딩
// =======================================================================
bool ProductionAnalyzer::ReadNextBinaryEvent() {
    uint32_t header[3];
    if (fread(header, sizeof(uint32_t), 3, _fpIn) != 3) return false;

    _evtData->Clear("C");
    uint32_t evtNum = header[0];
    uint32_t nCh = header[1];
    uint32_t nPts = header[2];

    for (uint32_t c = 0; c < nCh; c++) {
        uint32_t chId;
        fread(&chId, sizeof(uint32_t), 1, _fpIn);
        RawChannel* ch = _evtData->AddChannel(chId, nPts);
        
        std::vector<unsigned short> trace_buf(nPts);
        fread(trace_buf.data(), sizeof(unsigned short), nPts, _fpIn);
        for (uint32_t k = 0; k < nPts; k++) ch->AddSample(trace_buf[k]);
    }
    return true;
}

void ProductionAnalyzer::AnalyzeWaveform(const vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, std::vector<double>* vTime, std::vector<double>* vDrop) {
    int ndp = wave.size();
    if (ndp < 30) { bsl = 0; amp = 0; time = 0; charge = 0; return; }

    int ped_s = 2;
    int ped_e = std::min(40, ndp / 5); 
    int sig_s = ped_e + 5;

    double pedSum = 0;
    int nPed = 0;
    for(int i = ped_s; i <= ped_e; i++){
        pedSum += wave[i]; 
        nPed++;
    }
    bsl = (nPed > 0) ? pedSum / nPed : 0;
    
    double qSum = 0;
    double maxAmp = -9999;
    int maxIdx = -1;

    for (int i = 0; i < ndp; i++) {
        double val = (double)wave[i];
        double dropAmp = bsl - val; 
        
        if (i >= sig_s) {
            if (dropAmp > 0) qSum += dropAmp; 
            if (dropAmp > maxAmp) {
                maxAmp = dropAmp;
                maxIdx = i;
            }
        }
        
        if (_saveWaveform && vTime && vDrop) {
            vTime->push_back(i * 2.5);  
            vDrop->push_back(dropAmp);  
        }
    }

    amp = maxAmp;
    charge = qSum;
    time = maxIdx * 2.5; 
}

void ProductionAnalyzer::RunBatch() {
    if (!_isValid || _useDisplay) return;

    ELog::Print(ELog::INFO, "Starting High-Speed Binary Batch Processing...");
    TStopwatch sw; sw.Start(); 
    
    int ev = 0;
    auto last_ui_time = std::chrono::steady_clock::now();

    fseek(_fpIn, 0, SEEK_SET);

    while (ReadNextBinaryEvent()) {
        _pmtArray->Clear("C"); 
        
        if (_saveWaveform) {
            for(int i=0; i<MAX_CH; i++) { _wTime[i]->clear(); _wDrop[i]->clear(); }
        }

        int nCh = _evtData->GetNChannels();
        for (int i = 0; i < nCh; i++) {
            RawChannel* ch = _evtData->GetChannel(i);
            if (!ch) continue;
            
            int chId = ch->GetChId();
            if (chId < 0 || chId >= MAX_CH) continue;

            double bsl, amp, time, charge;
            AnalyzeWaveform(ch->GetSamples(), bsl, amp, time, charge, _wTime[chId], _wDrop[chId]);

            Pmt* pmt = (Pmt*)_pmtArray->ConstructedAt(i);
            pmt->SetId(chId); pmt->SetPedMean(bsl); pmt->SetQmax(amp); pmt->SetQtot(charge); pmt->SetFmax(time);

            _hQtot[chId]->Fill(charge);
        }
        _tOut->Fill(); 
        ev++;
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_ui_time).count() > 500) {
            double curTime = sw.RealTime(); sw.Continue();
            double rate = ev / curTime;
            double progress = (double)ftell(_fpIn) / _fileSize * 100.0;
            printf("\r\033[1;32m[PROD]\033[0m Progress: %5.1f%% | Evts: %-7d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m", progress, ev, rate);
            fflush(stdout);
            last_ui_time = now;
        }
    }
    _nEntries = ev;
    
    double totalTime = sw.RealTime();
    double avgSpeed = (totalTime > 0) ? (_nEntries / totalTime) : 0.0;
    printf("\r\033[1;32m[PROD]\033[0m Progress: 100.0%% | Evts: %-7d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m\n", _nEntries, avgSpeed);
    
    _fOut->cd();
    _tOut->AutoSave();
    for(int i=0; i<MAX_CH; i++) { if(_hQtot[i]->GetEntries() > 0) _hQtot[i]->Write(); }

    std::cout << "\n\033[1;35m╔═══════════════════ PRODUCTION SUMMARY ════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35d\033[0m \033[1;35m║\033[0m", "Total Parsed", _nEntries) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35.2f sec\033[0m \033[1;35m║\033[0m", "Total Elapsed Time", totalTime) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;32m%-35.2f evts/s\033[0m \033[1;35m║\033[0m", "Average Speed", avgSpeed) << std::endl;
    std::cout << "\033[1;35m╚═══════════════════════════════════════════════════════════╝\033[0m\n" << std::endl;
}

void ProductionAnalyzer::ShowEvent(int target_entry) {
    if (target_entry < 0) return;
    
    fseek(_fpIn, 0, SEEK_SET);
    int curr = 0;
    bool found = false;
    
    // 타겟 이벤트까지 바이너리 스트림 전진
    while (curr <= target_entry) {
        if (!ReadNextBinaryEvent()) break;
        if (curr == target_entry) { found = true; break; }
        curr++;
    }
    if (!found) { cout << "Event " << target_entry << " not found.\n"; return; }

    int nCh = _evtData->GetNChannels();
    cout << "\n\033[1;36m=== Live Binary Event View: " << target_entry << " ===\033[0m\n";

    for (int i = 0; i < nCh && i < MAX_CH; i++) {
        RawChannel* ch = _evtData->GetChannel(i);
        if (!ch) continue;
        
        int chId = ch->GetChId();
        const vector<unsigned short>& wav = ch->GetSamples();
        int ns = wav.size();

        if (!_histWave[chId]) {
            _histWave[chId] = new TH1F(Form("h_ev_ch%d", chId), Form("Channel %d (Inverted);Time (ns);Voltage Drop (ADC)", chId), ns, 0, ns * 2.5);
            _histWave[chId]->SetDirectory(nullptr); 
            _histWave[chId]->SetLineColor(kBlue + 1);
            _histWave[chId]->SetFillColorAlpha(kBlue - 9, 0.3); 
        }
        if (_histWave[chId]->GetNbinsX() != ns) _histWave[chId]->SetBins(ns, 0, ns * 2.5);
        _histWave[chId]->Reset();
        
        double bsl, amp, time, charge;
        AnalyzeWaveform(wav, bsl, amp, time, charge, nullptr, nullptr); 

        double minV = 99999, maxV = -99999;
        for (int pt = 0; pt < ns; pt++) {
            double drop = bsl - wav[pt]; 
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
            _lineBsl[chId] = new TLine(0, 0, ns * 2.5, 0); 
            _lineBsl[chId]->SetLineColor(kRed); 
            _lineBsl[chId]->SetLineStyle(2); 
            _lineBsl[chId]->SetLineWidth(2);
            _lineBsl[chId]->Draw();
        }

        printf(" \033[1;33m[Ch %d]\033[0m Bsl: %6.1f | Amp: %6.1f | \033[1;32mTime: %6.1f ns\033[0m | Charge: %6.1f \n", 
               chId, bsl, amp, time, charge);
    }
    _canvasEvent->Update();
}

void ProductionAnalyzer::RunInteractive() {
    if (!_isValid || !_useDisplay) return;
    
    int currEntry = 0; 
    ShowEvent(currEntry);
    cout << "Commands: (n)ext, (j)ump, (q)uit > " << flush;

    while(true) {
        gSystem->ProcessEvents(); 

        fd_set readfds; 
        FD_ZERO(&readfds); 
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout = {0, 50000}; 

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
            string cmd;
            if (!(cin >> cmd)) { 
                cin.clear(); cin.ignore(10000, '\n'); continue;
            }
            char key = cmd[0];

            if (key == 'n') { 
                currEntry++; 
                ShowEvent(currEntry); 
            }
            else if (key == 'j') {
                int dest; 
                cout << "Jump to event number: "; 
                if (!(cin >> dest)) {
                    cin.clear(); cin.ignore(10000, '\n');
                    cout << "\033[1;31m[ERROR] Invalid input! Number required.\033[0m\n"; 
                    continue;
                }
                currEntry = dest; 
                ShowEvent(currEntry); 
            }
            else if (key == 'q') { 
                break; 
            }
            
            cout << "\nCommands: (n)ext, (j)ump, (q)uit > " << flush;
        }
    }
}

int main(int argc, char ** argv) {
    TString inFile = "";
    TString outFile = "prod.root";
    bool useDisplay = false;
    bool saveWaveform = false; 
    
    int opt;
    while((opt = getopt(argc, argv, "i:o:dw")) != -1) {
        switch(opt) {
            case 'i': inFile = optarg; break;
            case 'o': outFile = optarg; break;
            case 'd': useDisplay = true; break;
            case 'w': saveWaveform = true; break;
        }
    }

    if(inFile == "") {
        ELog::Print(ELog::ERROR, "Usage: production -i <run.dat> [-o <prod.root>] [-d] [-w]");
        return 1;
    }
    
    if(outFile == "prod.root" && inFile != "") {
        TString s(inFile);
        s.ReplaceAll(".dat", "_prod.root");
        outFile = s;
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
