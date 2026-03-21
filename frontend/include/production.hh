#ifndef production_hh
#define production_hh

#include <vector>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TLine.h"
#include "TClonesArray.h"

#include "RawData.hh"
#include "RawChannel.hh"
#include "RunInfo.hh"
#include "Run.hh"
#include "Pmt.hh"

class ProductionAnalyzer {
public:
    // 생성자 및 소멸자
    ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform);
    ~ProductionAnalyzer();

    // 상태 확인 및 실행 메서드
    bool IsValid() const { return _isValid; }
    void RunBatch();
    void RunInteractive();

private:
    // 파형 분석 및 드로잉 코어 함수
    void AnalyzeWaveform(const std::vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, std::vector<double>* vTime, std::vector<double>* vDrop);
    void ShowEvent(int entry);

    // 내부 상태 변수
    bool _isValid;
    bool _useDisplay;
    bool _saveWaveform;
    int _nEntries;

    // 입력 ROOT 파일 객체
    TFile* _fIn;
    TTree* _tIn;
    RawData* _evtData;
    RunInfo* _runInfo;

    // 출력 ROOT 파일 객체
    TFile* _fOut;
    TTree* _tOut;
    Run* _runHeader;
    TClonesArray* _pmtArray;
    
    // 분석용 히스토그램 (채널별 전하합 분포)
    TH1F* _hQtot[4]; 
    
    // 이벤트별 파형 전체 시계열 저장을 위한 벡터 (-w 옵션)
    std::vector<double>* _wTime[4];
    std::vector<double>* _wDrop[4];

    // 디스플레이 모드(-d 옵션)용 렌더링 객체
    TCanvas* _canvasEvent;
    TH1F* _histWave[4]; 
    TLine* _lineBsl[4]; 
};

#endif