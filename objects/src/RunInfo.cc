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
    std::cout << "========== RUN INFORMATION ==========" << std::endl;
    std::cout << " Run Number : " << _runnum << " [" << _runtypestr << "]" << std::endl;
    std::cout << " Description: " << _rundesc << std::endl;
    std::cout << " Shift      : " << _shift << std::endl;
    std::cout << " FADC Boards: " << GetNFadcBD() << std::endl;
    std::cout << "=====================================" << std::endl;
    for(int i = 0; i < GetNFadcBD(); i++) {
        GetFadcBD(i)->Print();
    }
}
