#include "FadcBD.hh"
#include <iostream> // [FIX] cout 사용을 위해 추가

using namespace std;

ClassImp(FadcBD)

FadcBD::FadcBD()
:TNamed()
{
  _isTrgBD = false;
  _mid = 0     ;
  _ndp = 512   ; // Fixed to 512
  _nch = 4     ;
  _rst = 2     ;
  _tlt = 0xFFFE; 
  _tow = 1000  ;
  _dce = 0     ;

  for(int i = 0; i < 4; i++){
    _cid[i]     = i   ;
    _pol[i]     = 0   ;
    _thr[i]     = 2   ;
    _dly[i]     = 800 ;
    _dacoff[i]  = 3400;
    _dacgain[i] = 1000;
    _dt[i]      = 0   ;
    _cw[i]      = 200 ;
    _tm[i]      = 1   ;
    _pct[i]     = 1   ;
    _pci[i]     = 1000;
    _pwt[i]     = 10  ;
  }
}

FadcBD::FadcBD(int mid)
  // [FIX] Ambiguous call 해결: TString::Format(...).Data() 사용
  :TNamed(TString::Format("%d", mid).Data(), "")
{
  _isTrgBD = false;
  _mid = mid   ;
  _ndp = 512   ; 
  _nch = 4     ;
  _rst = 2     ;
  _tlt = 0xFFFE; 
  _tow = 1000  ;
  _dce = 0     ;

  for(int i = 0; i < 4; i++){
    _cid[i]     = i   ;
    _pol[i]     = 0   ;
    _thr[i]     = 2   ;
    _dly[i]     = 800 ;
    _dacoff[i]  = 3400;
    _dacgain[i] = 1000;
    _dt[i]      = 0   ;
    _cw[i]      = 200 ;
    _tm[i]      = 1   ;
    _pct[i]     = 1   ;
    _pci[i]     = 1000;
    _pwt[i]     = 10  ;
  }
}

FadcBD::FadcBD(TString mid)
  :TNamed(mid.Data(),"")
{
  _isTrgBD = false;
  _mid = mid.Atoi();
  _ndp = 512   ; 
  _nch = 4     ;
  _rst = 2     ;
  _tlt = 0xFFFE; 
  _tow = 1000  ;
  _dce = 0     ;

  for(int i = 0; i < 4; i++){
    _cid[i]     = i   ;
    _pol[i]     = 0   ;
    _thr[i]     = 2   ;
    _dly[i]     = 800 ;
    _dacoff[i]  = 3400;
    _dacgain[i] = 1000;
    _dt[i]      = 0   ;
    _cw[i]      = 200 ;
    _tm[i]      = 1   ;
    _pct[i]     = 1   ;
    _pci[i]     = 1000;
    _pwt[i]     = 10  ;
  }
}

FadcBD::~FadcBD()
{
}

void FadcBD::Print(Option_t * option) const
{
  cout<<Form(" FADC [mid=%d] %8d %8d %8d %8d %8d %8d", _mid, _nch, _ndp, _rst, _tlt, _tow, _dce)<<endl;
  cout<<" -----------------------------------------------"<<endl;
  cout<<"        CID : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _cid[i]);
  cout<<endl;
  cout<<"        POL : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _pol[i]);
  cout<<endl;
  cout<<"        THR : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _thr[i]);
  cout<<endl;
  cout<<"        DLY : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _dly[i]);
  cout<<endl;
  cout<<"     DACOFF : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _dacoff[i]);
  cout<<endl;
  cout<<"    DACGAIN : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _dacgain[i]);
  cout<<endl;
  cout<<"         DT : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _dt[i]);
  cout<<endl;
  cout<<"         CW : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _cw[i]);
  cout<<endl;
  cout<<"         TM : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _tm[i]);
  cout<<endl;
  cout<<"        PCT : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _pct[i]);
  cout<<endl;
  cout<<"        PCI : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _pci[i]);
  cout<<endl;
  cout<<"        PWT : ";
  for(int i = 0; i < _nch; i++)
    cout<<Form("%8d", _pwt[i]);
  cout<<endl; 
  cout<<" -----------------------------------------------"<<endl;
  cout<<endl;
}