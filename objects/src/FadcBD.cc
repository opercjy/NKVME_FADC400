#include "FadcBD.hh"

ClassImp(FadcBD)

FadcBD::FadcBD() : TNamed() { InitDefaults(); }


FadcBD::FadcBD(int mid) : TNamed(TString::Format("%d", mid).Data(), "") { 
    InitDefaults(); 
    _mid = mid; 
}

FadcBD::FadcBD(TString mid) : TNamed(mid.Data(), "") { 
    InitDefaults(); 
    _mid = mid.Atoi(); 
}

FadcBD::~FadcBD() {}

void FadcBD::InitDefaults() {
    _isTrgBD = false;
    _mid = 0; _ndp = 512; _nch = 4; _rst = 2; _tlt = 0xFFFE; _tow = 1000; _dce = 0;
    for(int i=0; i<4; i++) {
        _cid[i]=i; _pol[i]=0; _thr[i]=2; _dly[i]=800; 
        _dacoff[i]=3400; _dacgain[i]=1000; _dt[i]=0; _cw[i]=200; 
        _tm[i]=1; _pct[i]=1; _pci[i]=1000; _pwt[i]=10;
    }
}

void FadcBD::Print(Option_t * /*option*/) const {
    std::cout << Form(" FADC [mid=%d] NCH:%d NDP:%d RST:%d TLT:0x%X TOW:%d DCE:%d", 
                      _mid, _nch, _ndp, _rst, _tlt, _tow, _dce) << std::endl;
}