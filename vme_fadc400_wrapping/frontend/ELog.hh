#ifndef ELog_hh
#define ELog_hh

#include "TString.h"

class ELog {
public:
  enum ELogType { INFO = 0, WARNING, ERROR };

  ELog();
  virtual ~ELog();

  void Print(ELogType type, TString msg);
};
#endif