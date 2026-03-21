#include "RunInfo.hh"
#include <iostream>

ClassImp(RunInfo)

RunInfo::RunInfo() : TNamed("RunInfo", "Configuration for Run"), 
    _runnum(0), _runtype(TEST), _runtypestr("TEST"), _rundesc("Test Run"), _shift("None"),
    _fadcrecordlength(1024), _fadcndumpedevent(10) 
{
    _fadcbdlist = new TObjArray();
    _fadcbdlist->SetOwner(kTRUE); // 삭제 시 내부 객체도 함께 자동 해제하여 누수 방지
}

RunInfo::~RunInfo() {
    if (_fadcbdlist) {
        _fadcbdlist->Delete();
        delete _fadcbdlist;
    }
}

void RunInfo::SetRunType(const char* type) {
    _runtypestr = type;
    TString s(type); s.ToUpper();
    if(s.Contains("CALIB")) _runtype = CALIBRATION;
    else if(s.Contains("PHYSICS")) _runtype = PHYSICS;
    else _runtype = TEST;
}

void RunInfo::AddFadcBD(FadcBD * bd) {
    if(bd) _fadcbdlist->Add(bd);
}

FadcBD* RunInfo::GetFadcBD(int i) const {
    if(i < 0 || i >= _fadcbdlist->GetEntriesFast()) return nullptr;
    return (FadcBD*)_fadcbdlist->At(i);
}

int RunInfo::GetNFadcBD() const {
    return _fadcbdlist->GetEntriesFast();
}

void RunInfo::Print(Option_t * /*option*/) const {
    // 1. RunInfo 글로벌 정보 (청록색 테두리 + 노란색 타이틀 + 흰색 값)
    std::cout << "\033[1;36m╔════════════════════════ RUN INFORMATION ════════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-12s\033[0m : \033[1;37m%-48s\033[0m \033[1;36m║\033[0m", "Run Number", Form("%d [%s]", _runnum, _runtypestr.Data())) << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-12s\033[0m : \033[1;37m%-48s\033[0m \033[1;36m║\033[0m", "Description", _rundesc.Data()) << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-12s\033[0m : \033[1;37m%-48s\033[0m \033[1;36m║\033[0m", "Shift", _shift.Data()) << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-12s\033[0m : \033[1;37m%-48d\033[0m \033[1;36m║\033[0m", "FADC Boards", GetNFadcBD()) << std::endl;
    std::cout << "\033[1;36m╚═════════════════════════════════════════════════════════════════╝\033[0m" << std::endl;
    
    // 2. 장착된 모든 FADC 보드의 Print() 함수 연쇄 호출 (FadcBD::Print 실행)
    for(int i = 0; i < GetNFadcBD(); i++) {
        GetFadcBD(i)->Print();
    }
}