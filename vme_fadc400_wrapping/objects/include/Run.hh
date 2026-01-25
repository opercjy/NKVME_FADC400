#ifndef Run_hh
#define Run_hh
#include "TNamed.h"
#include "TString.h"

class Run : public TNamed {
public:
  Run();
  virtual ~Run();
  void SetRunNumber(int run) { _runnum = run; }
  void SetTotalEvent(int nevt) { _totalevent = nevt; }
  void SetStartTime(TString time) { _start = time; }
  void SetEndTime(TString time) { _end = time; }
  void SetLog(TString log) { _log = log; }
  int GetRunNumber() const { return _runnum; }
  int GetTotalEvent() const { return _totalevent; }
  TString GetStartTime() const { return _start; }
  TString GetEndTime() const { return _end; }
private:
  int _runnum, _totalevent;
  TString _start, _end, _log;
  ClassDef(Run, 1)
};
#endif