#include "production.hh"
#include "ELog.hh"

#include <iostream>
#include <cmath>
#include <unistd.h>
#include <sys/select.h>

#include "TApplication.h"
#include "TSystem.h"
#include "TLine.h"
#include "TStyle.h"

using namespace std;

ProductionAnalyzer::ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay) 
    : _isValid(false), _useDisplay(useDisplay), _nEntries(0),
      _fIn(nullptr), _tIn(nullptr), _evtData(nullptr), _runInfo(nullptr),
      _fOut(nullptr), _tOut(nullptr), _runHeader(nullptr), _pmtArray(nullptr),
      _canvasEvent(nullptr) 
{
    for(int i=0; i<4; i++) _histWave[i] = nullptr;

    _fIn = new TFile(inFile, "READ");
    if (!_fIn || _fIn->IsZombie()) {
        ELog::Print(ELog::FATAL, Form("Cannot open input file: %s", inFile));
        return;
    }

    // 헤더 정보(RunInfo) 로드
    _runInfo = (RunInfo*)_fIn->Get("RunInfo");
    if(!_runInfo) {
        ELog::Print(ELog::WARNING, "RunInfo not found in file. Analysis might be limited.");
    }

    _tIn = (TTree*)_fIn->Get("FADC");
    if (!_tIn) {
        ELog::Print(ELog::FATAL, "Tree 'FADC' not found.");
        return;
    }

    _evtData = new RawData();
    _tIn->SetBranchAddress("RawData", &_evtData);
    _nEntries = _tIn->GetEntries();
    
    // 출력 파일 및 트리 셋업
    if (!_useDisplay) {
        _fOut = new TFile(outFile, "RECREATE");
        _runHeader = new Run(_runInfo);
        _runHeader->Write();

        _tOut = new TTree("PROD", "Processed Data Tree");
        _pmtArray = new TClonesArray("Pmt", 100);
        _tOut->Branch("Pmt", &_pmtArray);
    } else {
        gStyle->SetOptStat(0);
        _canvasEvent = new TCanvas("cEvent", "Waveform Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }

    _isValid = true;
    ELog::Print(ELog::INFO, Form("Opened %s. Total Events: %d", inFile, _nEntries));
}

ProductionAnalyzer::~ProductionAnalyzer() {
    if (_fIn) { _fIn->Close(); delete _fIn; }
    if (_fOut) { _fOut->Close(); delete _fOut; }
    if (_evtData) { _evtData->Clear("C"); delete _evtData; }
    if (_pmtArray) { _pmtArray->Clear("C"); delete _pmtArray; }
    if (_runHeader) delete _runHeader;
    for(int i=0; i<4; i++) if(_histWave[i]) delete _histWave[i];
    if (_canvasEvent) delete _canvasEvent;
}

void ProductionAnalyzer::AnalyzeWaveform(const vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge) {
    int ndp = wave.size();
    if (ndp < 30) { bsl = 0; amp = 0; time = 0; charge = 0; return; }

    int ped_s = 2;
    int ped_e = std::min(45, ndp / 5); 
    int sig_s = ped_e + 5;

    double pedSum = 0, ped2Sum = 0;
    int nPed = 0;
    for(int i = ped_s; i <= ped_e; i++){
        pedSum += wave[i]; 
        ped2Sum += wave[i] * wave[i]; 
        nPed++;
    }

    bsl = (nPed > 0) ? pedSum / nPed : 0;
    double ped2Mean = (nPed > 0) ? ped2Sum / nPed : 0;
    
    // [핵심 해결] 부동소수점 오차(NaN) 방어
    double pedVar = ped2Mean - bsl * bsl;
    if(pedVar < 0.0) pedVar = 0.0; 
    
    double qSum = 0;
    double maxAmp = -9999;
    int maxIdx = -1;

    for (int i = sig_s; i < ndp; i++) {
        double val = (double)wave[i];
        // Negative pulse assumption (기준선에서 아래로 떨어지는 펄스)
        double curAmp = bsl - val; 
        
        qSum += curAmp;
        if (curAmp > maxAmp) {
            maxAmp = curAmp;
            maxIdx = i;
        }
    }

    amp = maxAmp;
    charge = qSum;
    time = maxIdx; // 임시로 인덱스를 타임으로 저장 (이후 ns 단위로 환산 필요)
}

void ProductionAnalyzer::RunBatch() {
    if (!_isValid || _useDisplay) return;

    ELog::Print(ELog::INFO, "Starting Batch Processing...");
    
    for (int ev = 0; ev < _nEntries; ev++) {
        _tIn->GetEntry(ev);
        _pmtArray->Clear("C"); // 매 이벤트 풀 초기화

        int nCh = _evtData->GetNChannels();
        for (int i = 0; i < nCh; i++) {
            RawChannel* ch = _evtData->GetChannel(i);
            if (!ch) continue;
            
            double bsl, amp, time, charge;
            AnalyzeWaveform(ch->GetSamples(), bsl, amp, time, charge);

            // Object Pool 재활용(ConstructedAt)
            Pmt* pmt = (Pmt*)_pmtArray->ConstructedAt(i);
            pmt->SetId(ch->GetChId());
            pmt->SetPedMean(bsl);
            pmt->SetQmax(amp);
            pmt->SetQtot(charge);
            pmt->SetFmax(time);
        }

        _tOut->Fill();
        
        if (ev % 1000 == 0) {
            printf("\rProcessed: %d / %d", ev, _nEntries);
            fflush(stdout);
        }
    }
    printf("\rProcessed: %d / %d\n", _nEntries, _nEntries);
    _tOut->AutoSave();
    ELog::Print(ELog::INFO, "Batch Processing Completed.");
}

void ProductionAnalyzer::ShowEvent(int entry) {
    if (entry < 0 || entry >= _nEntries) return;
    _tIn->GetEntry(entry);

    int nCh = _evtData->GetNChannels();
    for (int i = 0; i < nCh && i < 4; i++) {
        RawChannel* ch = _evtData->GetChannel(i);
        if (!ch) continue;
        
        const vector<unsigned short>& wav = ch->GetSamples();
        int ns = wav.size();

        // [핵심 해결] gROOT 글로벌 레지스트리 해제로 메모리 릭 차단
        if (!_histWave[i]) {
            _histWave[i] = new TH1F(Form("h_ev_ch%d", i), Form("Channel %d;Sample;ADC", ch->GetChId()), ns, 0, ns);
            _histWave[i]->SetDirectory(nullptr); 
            _histWave[i]->SetLineColor(kBlue + 1);
        }
        if (_histWave[i]->GetNbinsX() != ns) _histWave[i]->SetBins(ns, 0, ns);
        
        _histWave[i]->Reset();
        
        double bsl, amp, time, charge;
        AnalyzeWaveform(wav, bsl, amp, time, charge);

        double minV = 99999, maxV = -99999;
        for (int pt = 0; pt < ns; pt++) {
            _histWave[i]->SetBinContent(pt + 1, wav[pt]);
            if(wav[pt] > maxV) maxV = wav[pt];
            if(wav[pt] < minV) minV = wav[pt];
        }

        _canvasEvent->cd(i + 1);
        double margin = (maxV - minV) * 0.1;
        if(margin < 10) margin = 10;
        _histWave[i]->GetYaxis()->SetRangeUser(minV - margin, maxV + margin);
        _histWave[i]->Draw("HIST");

        // [핵심 해결] TLine 동적 할당 메모리 릭 방지
        TLine* lineBsl = new TLine(0, bsl, ns, bsl);
        lineBsl->SetBit(kCanDelete); 
        lineBsl->SetLineColor(kRed); lineBsl->SetLineStyle(2); lineBsl->SetLineWidth(2);
        lineBsl->Draw();
    }
    _canvasEvent->Update();
}

void ProductionAnalyzer::RunInteractive() {
    if (!_isValid || !_useDisplay) return;
    
    int currEntry = 0; 
    ShowEvent(currEntry);
    cout << "\n[Interactive Mode] Commands: (n)ext, (p)rev, (j)ump, (q)uit\n";
    cout << "Command (Event " << currEntry << "/" << _nEntries-1 << ") > " << flush;

    while(true) {
        // [핵심 해결] ROOT GUI 이벤트를 계속 처리하여 Canvas 멈춤 현상(Freezing) 방어
        gSystem->ProcessEvents(); 

        fd_set readfds; 
        FD_ZERO(&readfds); 
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval timeout = {0, 50000}; // 50ms 비동기 폴링

        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
            string cmd;
            if (!(cin >> cmd)) { 
                cin.clear(); cin.ignore(10000, '\n'); continue;
            }
            char key = cmd[0];

            if (key == 'n') { if (currEntry < _nEntries - 1) currEntry++; ShowEvent(currEntry); }
            else if (key == 'p') { if (currEntry > 0) currEntry--; ShowEvent(currEntry); }
            else if (key == 'j') {
                int dest; cout << "Jump to event number: "; 
                if (!(cin >> dest)) {
                    cin.clear(); cin.ignore(10000, '\n');
                    cout << "[ERROR] Invalid input! Number required.\n"; continue;
                }
                if (dest >= 0 && dest < _nEntries) { currEntry = dest; ShowEvent(currEntry); }
            }
            else if (key == 'q') { break; }
            
            cout << "Command (Event " << currEntry << "/" << _nEntries-1 << ") > " << flush;
        }
    }
}

int main(int argc, char ** argv) {
    TString inFile = "";
    TString outFile = "prod.root";
    bool useDisplay = false;
    
    int opt;
    while((opt = getopt(argc, argv, "i:o:w")) != -1) {
        switch(opt) {
            case 'i': inFile = optarg; break;
            case 'o': outFile = optarg; break;
            case 'w': useDisplay = true; break;
        }
    }

    if(inFile == "") {
        ELog::Print(ELog::ERROR, "Usage: production -i <raw.root> [-o <prod.root>] [-w]");
        return 1;
    }

    if (useDisplay) {
        TApplication app("ProdApp", &argc, argv);
        ProductionAnalyzer analyzer(inFile.Data(), outFile.Data(), true);
        analyzer.RunInteractive();
    } else {
        ProductionAnalyzer analyzer(inFile.Data(), outFile.Data(), false);
        analyzer.RunBatch();
    }

    return 0;
}
