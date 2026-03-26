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

ProductionAnalyzer::ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform) 
    : _isValid(false), _useDisplay(useDisplay), _saveWaveform(saveWaveform), _nEntries(0),
      _fIn(nullptr), _tIn(nullptr), _evtData(nullptr), _runInfo(nullptr),
      _fOut(nullptr), _tOut(nullptr), _runHeader(nullptr), _pmtArray(nullptr),
      _canvasEvent(nullptr) 
{
    for(int i=0; i<MAX_CH; i++) { 
        _histWave[i] = nullptr; 
        _lineBsl[i] = nullptr; 
    }

    _fIn = new TFile(inFile, "READ");
    if (!_fIn || _fIn->IsZombie()) {
        ELog::Print(ELog::FATAL, Form("Cannot open input file: %s", inFile));
        return;
    }

    _runInfo = (RunInfo*)_fIn->Get("RunInfo");
    _tIn = (TTree*)_fIn->Get("FADC");
    if (!_tIn) {
        ELog::Print(ELog::FATAL, "Tree 'FADC' not found in the input file.");
        return;
    }

    _evtData = new RawData();
    _tIn->SetBranchAddress("RawData", &_evtData);
    _nEntries = _tIn->GetEntries();
    
    if (!_useDisplay) {
        _fOut = new TFile(outFile, "RECREATE");
        _runHeader = new Run(_runInfo);
        _runHeader->Write();

        _tOut = new TTree("PROD", "Processed Data Tree");
        _pmtArray = new TClonesArray("Pmt", 100);
        _tOut->Branch("Pmt", &_pmtArray);
        
        if (_saveWaveform) {
            ELog::Print(ELog::INFO, "Waveform (-w) mode ON. Waveforms will be saved directly into Pmt._wave array.");
        }
    } 
    else {
        gStyle->SetOptStat(0);
        _canvasEvent = new TCanvas("cEvent", "NoticeDAQ Interactive Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }

    _isValid = true;
    ELog::Print(ELog::INFO, Form("Opened %s. Total Events: %d", inFile, _nEntries));
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
        
        // 💡 [핵심] Pmt._wave 배열 포인터가 넘어왔다면 다이렉트로 기록
        if (outWave) {
            outWave[i] = (float)dropAmp;  
        }
    }

    amp = maxAmp;
    charge = qSum;
    time = maxIdx * 2.5; 
}

void ProductionAnalyzer::RunBatch() {
    if (!_isValid || _useDisplay) return;

    ELog::Print(ELog::INFO, "Starting Batch Processing...");
    TStopwatch sw; sw.Start(); 
    
    for (int ev = 0; ev < _nEntries; ev++) {
        _tIn->GetEntry(ev);
        _pmtArray->Clear("C"); 

        int nCh = _evtData->GetNChannels();
        for (int i = 0; i < nCh; i++) {
            RawChannel* ch = _evtData->GetChannel(i);
            if (!ch) continue;
            
            int chId = ch->GetChId();
            if (chId < 0 || chId >= MAX_CH) continue;

            Pmt* pmt = (Pmt*)_pmtArray->ConstructedAt(i);
            pmt->SetId(chId);

            if (_saveWaveform) {
                pmt->AllocateWaveform(ch->GetNPoints()); // TClonesArray 내부에서 동적 할당
            }

            double bsl, amp, time, charge;
            // -w 옵션이 켜져 있으면 pmt->Waveform() 포인터를 전달
            AnalyzeWaveform(ch->GetSamples(), bsl, amp, time, charge, _saveWaveform ? pmt->Waveform() : nullptr);

            pmt->SetPedMean(bsl); pmt->SetQmax(amp); pmt->SetQtot(charge); pmt->SetFmax(time);
        }
        _tOut->Fill(); 
        
        if (ev % 1000 == 0 && ev > 0) {
            double curTime = sw.RealTime(); sw.Continue();
            double rate = ev / curTime;
            printf("\r\033[1;32m[PROD]\033[0m Processed: %d / %d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m", ev, _nEntries, rate);
            fflush(stdout);
        }
    }
    
    double totalTime = sw.RealTime();
    double avgSpeed = (totalTime > 0) ? (_nEntries / totalTime) : 0.0;
    printf("\r\033[1;32m[PROD]\033[0m Processed: %d / %d | \033[1;35m⚡ Speed: %-6.1f evts/s\033[0m\n", _nEntries, _nEntries, avgSpeed);
    
    _fOut->cd();
    _tOut->AutoSave();

    std::cout << "\n\033[1;35m╔═══════════════════ PRODUCTION SUMMARY ════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35d\033[0m \033[1;35m║\033[0m", "Total Processed", _nEntries) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35.2f sec\033[0m \033[1;35m║\033[0m", "Total Elapsed Time", totalTime) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;32m%-35.2f evts/s\033[0m \033[1;35m║\033[0m", "Average Speed", avgSpeed) << std::endl;
    std::cout << "\033[1;35m╚═══════════════════════════════════════════════════════════╝\033[0m\n" << std::endl;

    ELog::Print(ELog::INFO, "Batch Processing Completed. Saved to output file.");
}

void ProductionAnalyzer::ShowEvent(int entry) {
    if (entry < 0 || entry >= _nEntries) return;
    _tIn->GetEntry(entry);

    int nCh = _evtData->GetNChannels();
    cout << "\n\033[1;36m=== Event " << entry << " / " << _nEntries - 1 << " ===\033[0m\n";

    for (int i = 0; i < nCh && i < MAX_CH; i++) {
        RawChannel* ch = _evtData->GetChannel(i);
        if (!ch) continue;
        
        int chId = ch->GetChId();
        if (chId < 0 || chId >= MAX_CH) continue;

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
        AnalyzeWaveform(wav, bsl, amp, time, charge, nullptr); 

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
    cout << "Commands: (n)ext, (p)rev, (j)ump, (q)uit > " << flush;

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
                if (currEntry < _nEntries - 1) currEntry++; 
                ShowEvent(currEntry); 
            }
            else if (key == 'p') { 
                if (currEntry > 0) currEntry--; 
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
                if (dest >= 0 && dest < _nEntries) { 
                    currEntry = dest; 
                    ShowEvent(currEntry); 
                }
            }
            else if (key == 'q') { 
                break; 
            }
            
            cout << "\nCommands: (n)ext, (p)rev, (j)ump, (q)uit > " << flush;
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
        ELog::Print(ELog::ERROR, "Usage: production -i <raw.root> [-o <prod.root>] [-d] [-w]");
        return 1;
    }
    
    if(outFile == "prod.root" && inFile != "") {
        TString s(inFile);
        s.ReplaceAll(".root", "_prod.root");
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