#ifndef production_hh
#define production_hh

#include <string>
#include <vector>

// ROOT Headers
#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TH2F.h"

// Custom Headers
#include "RawChannel.hh"
#include "RunInfo.hh"

class ProductionAnalyzer {
public:
    ProductionAnalyzer(const std::string& filename);
    virtual ~ProductionAnalyzer();

    bool IsValid() const { return _isValid; }

    // 메인 동작 함수
    void RunBatch(bool saveWaveform); // -w 모드 (파일 저장)
    void RunInteractive();            // -d 모드 (시각화)

private:
    // 내부 헬퍼 함수
    void AnalyzeWaveform(const std::vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge);
    void ShowEvent(int entry);
    void ShowCumulative();

    // 멤버 변수
    std::string _filename;
    TFile* _infile;
    TTree* _intree;
    TClonesArray* _rawdata;
    bool        _isValid;
    int         _nEntries;

    // [FIX] 누락된 멤버 변수 추가
    unsigned long long _inTTime; // 입력 파일에서 읽은 타임스탬프 저장용
    int _polarity;               // 0: Negative, 1: Positive

    // 시각화용 객체
    TCanvas* _canvasEvent;
    TCanvas* _canvasCumul;
    TH1F* _histWave[4];
    TH2F* _histCumul[4];
};

#endif