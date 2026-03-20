#ifndef production_hh
#define production_hh

#include <vector>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"

#include "RawData.hh"
#include "RawChannel.hh"
#include "RunInfo.hh"
#include "Run.hh"
#include "Pmt.hh"

class ProductionAnalyzer {
public:
    ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay);
    ~ProductionAnalyzer();

    bool IsValid() const { return _isValid; }
    
    // 배치(Batch) 모드로 모든 이벤트를 분석하여 새 ROOT 파일로 저장
    void RunBatch();
    
    // 터미널 인터랙티브 모드 (파형을 한 장씩 넘겨보며 분석)
    void RunInteractive();

private:
    void AnalyzeWaveform(const std::vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge);
    void ShowEvent(int entry);

    bool _isValid;
    bool _useDisplay;
    int _nEntries;

    TFile* _fIn;
    TTree* _tIn;
    RawData* _evtData;
    RunInfo* _runInfo;

    TFile* _fOut;
    TTree* _tOut;
    Run* _runHeader;
    TClonesArray* _pmtArray;

    TCanvas* _canvasEvent;
    TH1F* _histWave[4]; // 최대 4채널 드로잉 껍데기
};

#endif
