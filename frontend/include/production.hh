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
    ProductionAnalyzer(const char* inFile, const char* outFile, bool useDisplay, bool saveWaveform);
    ~ProductionAnalyzer();

    bool IsValid() const { return _isValid; }
    void RunBatch();
    void RunInteractive();

private:
    void AnalyzeWaveform(const std::vector<unsigned short>& wave, double &bsl, double &amp, double &time, double &charge, std::vector<double>* vTime, std::vector<double>* vDrop);
    void ShowEvent(int entry);

    bool _isValid;
    bool _useDisplay;
    bool _saveWaveform;
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
    
    static const int MAX_CH = 32;
    TH1F* _hQtot[MAX_CH]; 
    std::vector<double>* _wTime[MAX_CH];
    std::vector<double>* _wDrop[MAX_CH];
    TH1F* _histWave[MAX_CH]; 
    TLine* _lineBsl[MAX_CH]; 
};

#endif
