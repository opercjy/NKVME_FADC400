#include "Run.hh"

ClassImp(Run)

Run::Run()
: TNamed("Run", "Run Summary")
{
  _runnum = 0;
  _totalevent = 0;
}

Run::~Run()
{
}