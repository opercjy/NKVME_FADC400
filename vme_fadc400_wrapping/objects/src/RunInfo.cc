#include "RunInfo.hh"
#include <iostream>

using namespace std;

ClassImp(RunInfo)

RunInfo::RunInfo()
:TNamed("RunInfo", "Configuration for Run")
{
  _runnum = 0;
  _runtype = TEST;
  _runtypestr = "TEST";
  _rundesc = "Test Run";
  _shift = "Unknown";
  
  // Default Settings
  _fadcrecordlength = 1024; // Default bytes (NDP=512 * 2bytes)
  _fadcndumpedevent = 10;   // Events per bulk read

  _fadcbdlist = new TObjArray();
  _fadcbdlist->SetOwner(kTRUE);
}

RunInfo::~RunInfo()
{
  if(_fadcbdlist) {
      _fadcbdlist->Delete();
      delete _fadcbdlist;
  }
}

void RunInfo::SetRunType(const char* type)
{
  _runtypestr = type;
  TString s(type);
  s.ToUpper();
  
  if(s.Contains("TEST")) _runtype = TEST;
  else if(s.Contains("CALIB")) _runtype = CALIBRATION;
  else if(s.Contains("PHYSICS")) _runtype = PHYSICS;
  else _runtype = TEST;
}

void RunInfo::AddFadcBD(FadcBD * bd)
{
  if(bd) _fadcbdlist->Add(bd);
}

FadcBD * RunInfo::GetFadcBD(int i) const
{
  if(i < 0 || i >= _fadcbdlist->GetEntries()) return NULL;
  return (FadcBD*)_fadcbdlist->At(i);
}

FadcBD * RunInfo::GetFadcBD(const char * mid) const
{
  return (FadcBD*)_fadcbdlist->FindObject(mid);
}

int RunInfo::GetNFadcBD() const
{
  return _fadcbdlist->GetEntries();
}

void RunInfo::Print(Option_t * option) const
{
  cout << " ================================================= " << endl;
  cout << "               RUN INFORMATION                     " << endl;
  cout << " ================================================= " << endl;
  cout << "  Run Number : " << _runnum << endl;
  cout << "  Run Type   : " << _runtypestr << endl;
  cout << "  Run Desc   : " << _rundesc << endl;
  cout << "  Shift      : " << _shift << endl;
  cout << " ------------------------------------------------- " << endl;
  cout << "  FADC Boards: " << _fadcbdlist->GetEntries() << endl;
  cout << "  Rec Length : " << _fadcrecordlength << " bytes" << endl;
  cout << " ------------------------------------------------- " << endl;
  
  for(int i = 0; i < _fadcbdlist->GetEntries(); i++) {
      ((FadcBD*)_fadcbdlist->At(i))->Print();
  }
  cout << " ================================================= " << endl;
}