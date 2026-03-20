#ifndef RunInfo_hh
#define RunInfo_hh

#include "TNamed.h"
#include "TString.h"
#include "TObjArray.h"
#include "FadcBD.hh"

class RunInfo : public TNamed {
public:
    enum { TEST = 0, CALIBRATION, PHYSICS };

    RunInfo();
    virtual ~RunInfo();

    void SetRunNumber(int runnum) { _runnum = runnum; }
    void SetRunType(const char* type);
    void SetRunDesc(TString desc) { _rundesc = desc; }
    void SetShift(TString shift)  { _shift = shift; }
    
    void SetFadcRecordLength(int rl) { _fadcrecordlength = rl; }
    void SetFadcNDumpedEvent(int nevt) { _fadcndumpedevent = nevt; }

    void AddFadcBD(FadcBD * bd);
    FadcBD* GetFadcBD(int i) const;
    int GetNFadcBD() const;

    int RunNumber() const { return _runnum; }
    int GetFadcRecordLength() const { return _fadcrecordlength; }
    int GetFadcNDumpedEvent() const { return _fadcndumpedevent; }

    virtual void Print(Option_t * option = "") const override;

private:
    int _runnum;
    int _runtype;
    TString _runtypestr;
    TString _rundesc;
    TString _shift;
    
    int _fadcrecordlength;
    int _fadcndumpedevent;
    
    TObjArray * _fadcbdlist; //-> FadcBD 객체들을 메모리 소유권과 함께 보관

    ClassDefOverride(RunInfo, 1)
};
#endif
