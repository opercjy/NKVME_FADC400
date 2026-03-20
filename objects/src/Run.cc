#include "Run.hh"

ClassImp(Run)

Run::Run() : TNamed("RUN", ""), _runNumber(0), _nPmt(0) 
{
}

Run::Run(RunInfo * run) : TNamed("RUN", "") 
{
    InitByRunInfo(run);
}

Run::~Run() 
{
}

void Run::InitByRunInfo(RunInfo * run) 
{
    if (!run) return;
    
    _runNumber = run->RunNumber();
    
    // [수정] 우리가 새로 구축한 RunInfo 아키텍처와 호환되도록 로직 변경
    // 등록된 모든 FADC 보드들의 채널 수를 합산하여 전체 PMT 개수를 구합니다.
    _nPmt = 0;
    for (int i = 0; i < run->GetNFadcBD(); i++) {
        FadcBD* bd = run->GetFadcBD(i);
        if (bd) {
            _nPmt += bd->NCHANNEL();
        }
    }
}
