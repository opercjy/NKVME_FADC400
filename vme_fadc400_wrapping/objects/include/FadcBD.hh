#ifndef FadcBD_hh
#define FadcBD_hh

#include <iostream>
#include "TNamed.h"
#include "TString.h"

class FadcBD : public TNamed {
public:
  FadcBD();
  FadcBD(int mid);
  FadcBD(TString mid); // [FIX] 누락된 생성자 선언 추가
  virtual ~FadcBD();

  // Setters
  void SetTrgBD()                { _isTrgBD = true; }
  void SetMID(int id)            { _mid = id; }
  void SetNDP(int ndp)           { _ndp = ndp; } 
  void SetNCH(int nch)           { _nch = nch; }
  void SetRST(int rst)           { _rst = rst; }
  void SetTLT(int tlt)           { _tlt = tlt; }
  void SetTOW(int tow)           { _tow = tow; }
  void SetDCE(int dce)           { _dce = dce; }

  // Channel Settings
  void SetCID(int ch, int id)    { _cid[ch] = id; }
  void SetPOL(int ch, int pol)   { _pol[ch] = pol; }
  void SetTHR(int ch, int thr)   { _thr[ch] = thr; }
  void SetDLY(int ch, int dly)   { _dly[ch] = dly; }
  void SetDACOFF(int ch, int off)    { _dacoff[ch] = off; }
  void SetDACGAIN(int ch, int gain)  { _dacgain[ch] = gain; }
  void SetDT(int ch, int dt)     { _dt[ch] = dt; }
  void SetCW(int ch, int cw)     { _cw[ch] = cw; }
  void SetTM(int ch, int tm)     { _tm[ch] = tm; }
  void SetPCT(int ch, int pct)   { _pct[ch] = pct; }
  void SetPCI(int ch, int pci)   { _pci[ch] = pci; }
  void SetPWT(int ch, int pwt)   { _pwt[ch] = pwt; }

  // Getters
  int MID() const        { return _mid; }
  int NDP() const        { return _ndp; }
  int RL() const         { return _ndp / 128; } 
  int NCHANNEL() const   { return _nch; }
  int RST() const        { return _rst; }
  int TLT() const        { return _tlt; }
  int TOW() const        { return _tow; }
  int DCE() const        { return _dce; }
  
  // Channel Getters
  int CID(int ch) const  { return _cid[ch]; }
  int POL(int ch) const  { return _pol[ch]; }
  int THR(int ch) const  { return _thr[ch]; }
  int DLY(int ch) const  { return _dly[ch]; }
  int DACOFF(int ch) const   { return _dacoff[ch]; }
  int DACGAIN(int ch) const  { return _dacgain[ch]; }
  int DT(int ch) const   { return _dt[ch]; }
  int CW(int ch) const   { return _cw[ch]; }
  int TM(int ch) const   { return _tm[ch]; }
  int PCT(int ch) const  { return _pct[ch]; }
  int PCI(int ch) const  { return _pci[ch]; }
  int PWT(int ch) const  { return _pwt[ch]; }

  bool IsTrgBD() const   { return _isTrgBD; }

  virtual void Print(Option_t * option = "") const;

private:
  bool _isTrgBD;

  int _mid;
  int _ndp; 
  int _nch;
  int _rst;
  int _tlt;
  int _tow;
  int _dce;

  int _cid[4];
  int _pol[4];
  int _thr[4];
  int _dly[4];
  int _dacoff[4];
  int _dacgain[4];
  int _dt[4];
  int _cw[4];
  int _tm[4];
  int _pct[4];
  int _pci[4];
  int _pwt[4];
  
  ClassDef(FadcBD, 1)
};
#endif