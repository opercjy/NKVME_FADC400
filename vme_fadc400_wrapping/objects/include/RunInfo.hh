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
  void SetRunType(int type) { _runtype = type; }
  void SetRunType(const char* type);
  void SetRunDesc(TString desc) { _rundesc = desc; }
  void SetShift(TString shift) { _shift = shift; }
  
  void AddFadcBD(FadcBD * bd);
  FadcBD * GetFadcBD(int i) const;
  FadcBD * GetFadcBD(const char * mid) const;
  int GetNFadcBD() const;

  void SetFadcRecordLength(int rl) { _fadcrecordlength = rl; }
  int  GetFadcRecordLength() const { return _fadcrecordlength; }
  void SetFadcNDumpedEvent(int nevt) { _fadcndumpedevent = nevt; }
  int  GetFadcNDumpedEvent() const { return _fadcndumpedevent; }

  int RunNumber() const { return _runnum; }
  TString RunType() const { return _runtypestr; }
  TString RunDesc() const { return _rundesc; }
  TString Shift() const { return _shift; }

  virtual void Print(Option_t * option = "") const;

private:
  int _runnum, _runtype, _fadcrecordlength, _fadcndumpedevent;
  TString _runtypestr, _rundesc, _shift;
  TObjArray * _fadcbdlist;
  ClassDef(RunInfo, 1)
};
#endif