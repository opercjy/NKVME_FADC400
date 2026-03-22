#ifndef PRODUCTION_HH
#define PRODUCTION_HH

#include <vector>
#include <cstdio>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TLine.h"
#include "TCanvas.h"
#include "TClonesArray.h"

#include "RunInfo.hh"
#include "Run.hh"
#include "RawData.hh"
#include "Pmt.hh"

#define MAX_CH 32

class ProductionAnalyzer {
private:
    bool _isValid;
    bool _useDisplay;
    bool _saveWaveform;
    
    // 💡 [Phase 6] 순수 바이너리 읽기용 포인터 및 이벤트 캐시
    FILE* _fpIn;
    size_t _fileSize;
    int _nEntries;
    RawData* _evtData;
    RunInfo* _runInfo;
    
    // Output ROOT Variables
    TFile* _fOut;
    TTree* _tOut;
    Run* _runHeader;
    TClonesArray* _pmtArray;

    // Interactive Display Variables
    TCanvas* _canvasEvent;
    TH1F* _histWave[MAX_CH];
    TLine* _lineBsl[MAX_CH];

    // Physics Analysis & Waveform Save
    TH1F* _hQtot[MAX_CH];
    std::vector<double>* _wTime[MAX_CH];
    std::vector<double>* _wDrop[MAX_CH];

    // 💡 [Phase 6] 바이너리에서 다음 1개 이벤트를 읽어 _evtData에 세팅하는 내부 함수
    bool ReadNextBinaryEvent();
    void AnalyzeWaveform(const std::vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, std::vector<double>* vTime, std::vector<double>* vDrop);

public:
    ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform);
    ~ProductionAnalyzer();

    void RunBatch();
    void RunInteractive();
    void ShowEvent(int entry); 
};

#endif
