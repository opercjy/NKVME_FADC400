#ifndef FadcBD_hh
#define FadcBD_hh

#include "TNamed.h"
#include "TString.h"
#include <iostream>

class FadcBD : public TNamed {
public:
    FadcBD();
    FadcBD(int mid);
    FadcBD(TString mid);
    virtual ~FadcBD();

    void SetTrgBD()                { _isTrgBD = true; }
    void SetMID(int id)            { _mid = id; }
    void SetNDP(int ndp)           { _ndp = ndp; } 
    void SetNCH(int nch)           { _nch = nch; }
    void SetRST(int rst)           { _rst = rst; }
    void SetTLT(int tlt)           { _tlt = tlt; }
    void SetTOW(int tow)           { _tow = tow; }
    void SetDCE(int dce)           { _dce = dce; }

    void SetCID(int ch, int id)    { if(ch>=0 && ch<4) _cid[ch] = id; }
    void SetPOL(int ch, int pol)   { if(ch>=0 && ch<4) _pol[ch] = pol; }
    void SetTHR(int ch, int thr)   { if(ch>=0 && ch<4) _thr[ch] = thr; }
    void SetDLY(int ch, int dly)   { if(ch>=0 && ch<4) _dly[ch] = dly; }
    void SetDACOFF(int ch, int off){ if(ch>=0 && ch<4) _dacoff[ch] = off; }
    void SetDACGAIN(int ch, int gain){ if(ch>=0 && ch<4) _dacgain[ch] = gain; }
    void SetDT(int ch, int dt)     { if(ch>=0 && ch<4) _dt[ch] = dt; }
    void SetCW(int ch, int cw)     { if(ch>=0 && ch<4) _cw[ch] = cw; }
    void SetTM(int ch, int tm)     { if(ch>=0 && ch<4) _tm[ch] = tm; }
    void SetPCT(int ch, int pct)   { if(ch>=0 && ch<4) _pct[ch] = pct; }
    void SetPCI(int ch, int pci)   { if(ch>=0 && ch<4) _pci[ch] = pci; }
    void SetPWT(int ch, int pwt)   { if(ch>=0 && ch<4) _pwt[ch] = pwt; }

    bool IsTrgBD() const   { return _isTrgBD; }
    int MID() const        { return _mid; }
    int NDP() const        { return _ndp; }
    int RL() const         { return _ndp / 128; } 
    int NCHANNEL() const   { return _nch; }
    int RST() const        { return _rst; }
    int TLT() const        { return _tlt; }
    int TOW() const        { return _tow; }
    int DCE() const        { return _dce; }

    int CID(int ch) const  { return (ch>=0 && ch<4) ? _cid[ch] : 0; }
    int POL(int ch) const  { return (ch>=0 && ch<4) ? _pol[ch] : 0; }
    int THR(int ch) const  { return (ch>=0 && ch<4) ? _thr[ch] : 0; }
    int DLY(int ch) const  { return (ch>=0 && ch<4) ? _dly[ch] : 0; }
    int DACOFF(int ch) const{ return (ch>=0 && ch<4) ? _dacoff[ch] : 0; }
    int DACGAIN(int ch) const{ return (ch>=0 && ch<4) ? _dacgain[ch] : 0; }
    int DT(int ch) const   { return (ch>=0 && ch<4) ? _dt[ch] : 0; }
    int CW(int ch) const   { return (ch>=0 && ch<4) ? _cw[ch] : 0; }
    int TM(int ch) const   { return (ch>=0 && ch<4) ? _tm[ch] : 0; }
    int PCT(int ch) const  { return (ch>=0 && ch<4) ? _pct[ch] : 0; }
    int PCI(int ch) const  { return (ch>=0 && ch<4) ? _pci[ch] : 0; }
    int PWT(int ch) const  { return (ch>=0 && ch<4) ? _pwt[ch] : 0; }

    virtual void Print(Option_t * option = "") const override;

private:
    void InitDefaults();

    bool _isTrgBD;
    int _mid, _ndp, _nch, _rst, _tlt, _tow, _dce;
    int _cid[4], _pol[4], _thr[4], _dly[4], _dacoff[4], _dacgain[4];
    int _dt[4], _cw[4], _tm[4], _pct[4], _pci[4], _pwt[4];

    ClassDefOverride(FadcBD, 1)
};
#endif
