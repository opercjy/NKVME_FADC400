#ifndef Run_hh
#define Run_hh

#include "TNamed.h"
#include "RunInfo.hh"

class Run : public TNamed {
public:
    Run();
    Run(RunInfo * run);
    virtual ~Run();

    void InitByRunInfo(RunInfo * run);

    int RunNumber() const { return _runNumber; }
    int NPmt()      const { return _nPmt     ; }

private:
    int _runNumber;
    int _nPmt     ;

    ClassDefOverride(Run, 1)
};
#endif
