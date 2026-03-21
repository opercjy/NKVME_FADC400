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

// =======================================================================
// Constructor & Destructor
// =======================================================================
ProductionAnalyzer::ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform) 
    : _isValid(false), _useDisplay(useDisplay), _saveWaveform(saveWaveform), _nEntries(0),
      _fIn(nullptr), _tIn(nullptr), _evtData(nullptr), _runInfo(nullptr),
      _fOut(nullptr), _tOut(nullptr), _runHeader(nullptr), _pmtArray(nullptr),
      _canvasEvent(nullptr) 
{
    // 포인터 초기화
    for(int i=0; i<4; i++) { 
        _histWave[i] = nullptr; 
        _lineBsl[i] = nullptr; 
        _hQtot[i] = nullptr;
        _wTime[i] = new std::vector<double>();
        _wDrop[i] = new std::vector<double>();
    }

    // 1. 입력 파일 열기
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
    
    // 2. 배치 모드 (파일 쓰기) 설정
    if (!_useDisplay) {
        _fOut = new TFile(outFile, "RECREATE");
        _runHeader = new Run(_runInfo);
        _runHeader->Write();

        _tOut = new TTree("PROD", "Processed Data Tree");
        _pmtArray = new TClonesArray("Pmt", 100);
        _tOut->Branch("Pmt", &_pmtArray);
        
        // 파형 저장 옵션(-w)이 켜져 있으면, 시계열 벡터 브랜치 생성
        if (_saveWaveform) {
            ELog::Print(ELog::INFO, "Waveform (-w) mode ON. Saving full time-series into the tree.");
            for(int i=0; i<4; i++) {
                _tOut->Branch(Form("wTime_Ch%d", i), &_wTime[i]);
                _tOut->Branch(Form("wDrop_Ch%d", i), &_wDrop[i]);
            }
        }
        
        // 전하합 히스토그램 생성
        for(int i=0; i<4; i++) {
            _hQtot[i] = new TH1F(Form("hQtot_Ch%d", i), Form("Charge Distribution Ch%d;Charge (ADC sum);Counts", i), 1000, 0, 50000);
        }
    } 
    // 3. 디스플레이 모드 (-d) 설정
    else {
        gStyle->SetOptStat(0);
        _canvasEvent = new TCanvas("cEvent", "NoticeDAQ Interactive Viewer", 1200, 800);
        _canvasEvent->Divide(2, 2);
    }

    _isValid = true;
    ELog::Print(ELog::INFO, Form("Opened %s. Total Events: %d", inFile, _nEntries));
}

ProductionAnalyzer::~ProductionAnalyzer() {
    // 1. 메모리에 직접 할당한 객체들을 먼저 삭제 (Double Free 방지)
    if (_evtData) { _evtData->Clear("C"); delete _evtData; }
    if (_pmtArray) { _pmtArray->Clear("C"); delete _pmtArray; }
    if (_runHeader) { delete _runHeader; }
    
    for(int i=0; i<4; i++) {
        if(_histWave[i]) delete _histWave[i];
        if(_lineBsl[i]) delete _lineBsl[i];
        
        // [핵심 픽스] _hQtot는 _fOut이 닫힐 때 ROOT가 자동으로 메모리를 해제합니다!
        // 여기서 수동으로 delete 하면 이중 해제(Double Free)가 발생하여 SegFault가 터집니다.
        // if(_hQtot[i]) delete _hQtot[i];  <-- 이 부분을 영구 삭제했습니다.
        
        if(_wTime[i]) delete _wTime[i];
        if(_wDrop[i]) delete _wDrop[i];
    }
    if (_canvasEvent) delete _canvasEvent;

    // 2. ROOT 파일 포인터는 가장 마지막에 닫고 삭제 (내부에 종속된 Tree, Histogram은 이때 자동 소멸)
    if (_fOut) { _fOut->Close(); delete _fOut; }
    if (_fIn)  { _fIn->Close();  delete _fIn; }
}
// =======================================================================
// Core Analysis Logic
// =======================================================================
void ProductionAnalyzer::AnalyzeWaveform(const vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, std::vector<double>* vTime, std::vector<double>* vDrop) {
    int ndp = wave.size();
    if (ndp < 30) { bsl = 0; amp = 0; time = 0; charge = 0; return; }

    // 1. 페데스탈(Baseline) 계산
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
    
    // 2. 펄스 반전 (Voltage Drop) 및 분석
    double qSum = 0;
    double maxAmp = -9999;
    int maxIdx = -1;

    for (int i = 0; i < ndp; i++) {
        double val = (double)wave[i];
        double dropAmp = bsl - val; // 반전 로직 (음의 신호를 양으로)
        
        // 신호 영역에서만 전하량 적분 및 최대 피크 탐색
        if (i >= sig_s) {
            if (dropAmp > 0) qSum += dropAmp; 
            if (dropAmp > maxAmp) {
                maxAmp = dropAmp;
                maxIdx = i;
            }
        }
        
        // -w 모드이거나 vTime, vDrop 벡터가 전달되었을 때 시계열 저장
        if (_saveWaveform && vTime && vDrop) {
            vTime->push_back(i * 2.5);  // 물리적 시간 (ns)
            vDrop->push_back(dropAmp);  // 보정된 전압 강하량
        }
    }

    amp = maxAmp;
    charge = qSum;
    time = maxIdx * 2.5; // 최대 피크 지점의 물리적 시간 (400MHz = 2.5ns/sample)
}

// =======================================================================
// Batch Processing Mode
// =======================================================================
void ProductionAnalyzer::RunBatch() {
    if (!_isValid || _useDisplay) return;

    ELog::Print(ELog::INFO, "Starting Batch Processing...");
    TStopwatch sw; sw.Start(); // 타이머 시작
    
    for (int ev = 0; ev < _nEntries; ev++) {
        _tIn->GetEntry(ev);
        _pmtArray->Clear("C"); 
        
        if (_saveWaveform) {
            for(int i=0; i<4; i++) { _wTime[i]->clear(); _wDrop[i]->clear(); }
        }

        int nCh = _evtData->GetNChannels();
        for (int i = 0; i < nCh; i++) {
            RawChannel* ch = _evtData->GetChannel(i);
            if (!ch) continue;
            
            double bsl, amp, time, charge;
            int chId = ch->GetChId();
            AnalyzeWaveform(ch->GetSamples(), bsl, amp, time, charge, _wTime[chId], _wDrop[chId]);

            Pmt* pmt = (Pmt*)_pmtArray->ConstructedAt(i);
            pmt->SetId(chId); pmt->SetPedMean(bsl); pmt->SetQmax(amp); pmt->SetQtot(charge); pmt->SetFmax(time);

            if(chId < 4) _hQtot[chId]->Fill(charge);
        }
        _tOut->Fill(); 
        
        // [핵심 추가] 실시간 처리 속도(Speed) 계산
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
    for(int i=0; i<4; i++) { if(_hQtot[i]->GetEntries() > 0) _hQtot[i]->Write(); }

    // [핵심 추가] Production 서머리 출력
    std::cout << "\n\033[1;35m╔═══════════════════ PRODUCTION SUMMARY ════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35d\033[0m \033[1;35m║\033[0m", "Total Processed", _nEntries) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35.2f sec\033[0m \033[1;35m║\033[0m", "Total Elapsed Time", totalTime) << std::endl;
    std::cout << Form("\033[1;35m║\033[0m \033[1;33m%-20s\033[0m : \033[1;32m%-35.2f evts/s\033[0m \033[1;35m║\033[0m", "Average Speed", avgSpeed) << std::endl;
    std::cout << "\033[1;35m╚═══════════════════════════════════════════════════════════╝\033[0m\n" << std::endl;

    ELog::Print(ELog::INFO, "Batch Processing Completed. Saved to output file.");
}

// =======================================================================
// Interactive Display Mode
// =======================================================================
void ProductionAnalyzer::ShowEvent(int entry) {
    if (entry < 0 || entry >= _nEntries) return;
    _tIn->GetEntry(entry);

    int nCh = _evtData->GetNChannels();
    cout << "\n\033[1;36m=== Event " << entry << " / " << _nEntries - 1 << " ===\033[0m\n";

    for (int i = 0; i < nCh && i < 4; i++) {
        RawChannel* ch = _evtData->GetChannel(i);
        if (!ch) continue;
        
        const vector<unsigned short>& wav = ch->GetSamples();
        int ns = wav.size();

        if (!_histWave[i]) {
            _histWave[i] = new TH1F(Form("h_ev_ch%d", i), Form("Channel %d (Inverted);Time (ns);Voltage Drop (ADC)", ch->GetChId()), ns, 0, ns * 2.5);
            _histWave[i]->SetDirectory(nullptr); 
            _histWave[i]->SetLineColor(kBlue + 1);
            _histWave[i]->SetFillColorAlpha(kBlue - 9, 0.3); 
        }
        if (_histWave[i]->GetNbinsX() != ns) _histWave[i]->SetBins(ns, 0, ns * 2.5);
        
        _histWave[i]->Reset();
        
        double bsl, amp, time, charge;
        AnalyzeWaveform(wav, bsl, amp, time, charge, nullptr, nullptr); // 화면 출력 시 벡터 기록 생략

        double minV = 99999, maxV = -99999;
        for (int pt = 0; pt < ns; pt++) {
            double drop = bsl - wav[pt]; 
            _histWave[i]->SetBinContent(pt + 1, drop);
            if(drop > maxV) maxV = drop;
            if(drop < minV) minV = drop;
        }
        
        if (maxV <= minV) { maxV = minV + 100; minV = minV - 100; }

        _canvasEvent->cd(i + 1);
        double margin = (maxV - minV) * 0.1;
        if(margin < 10) margin = 10;
        _histWave[i]->GetYaxis()->SetRangeUser(minV - margin, maxV + margin);
        _histWave[i]->Draw("HIST");

        if (_lineBsl[i]) delete _lineBsl[i];
        _lineBsl[i] = new TLine(0, 0, ns * 2.5, 0); // 반전되었으므로 Baseline은 항상 0
        _lineBsl[i]->SetLineColor(kRed); 
        _lineBsl[i]->SetLineStyle(2); 
        _lineBsl[i]->SetLineWidth(2);
        _lineBsl[i]->Draw();

        printf(" \033[1;33m[Ch %d]\033[0m Bsl: %6.1f | Amp: %6.1f | \033[1;32mTime: %6.1f ns\033[0m | Charge: %6.1f \n", 
               ch->GetChId(), bsl, amp, time, charge);
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

        // GUI 프로세스(QProcess)나 터미널의 입력 대기
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

// =======================================================================
// Main Entry
// =======================================================================
int main(int argc, char ** argv) {
    TString inFile = "";
    TString outFile = "prod.root";
    bool useDisplay = false;
    bool saveWaveform = false; 
    
    int opt;
    // 명령줄 인자 파싱
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
    
    // -o 가 입력되지 않은 경우 원본파일명_prod.root 생성
    if(outFile == "prod.root" && inFile != "") {
        TString s(inFile);
        s.ReplaceAll(".root", "_prod.root");
        outFile = s;
    }

    // 실행 분기 (디스플레이 vs 배치)
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