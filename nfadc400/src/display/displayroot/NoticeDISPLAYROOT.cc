#include "NoticeDISPLAYROOT.h"
#include "NoticeDISPLAY.h"

ClassImp(NKDISPLAY)

NKDISPLAY::NKDISPLAY()
{
}

NKDISPLAY::~NKDISPLAY()
{
}

void NKDISPLAY::DISPwriteData(unsigned long mid, unsigned long data)
{
  ::DISPwriteData(mid, data);
}

unsigned long NKDISPLAY::DISPreadData(unsigned long mid)
{
  return ::DISPreadData(mid);
}

void NKDISPLAY::DISPsetTimeout(unsigned long mid, unsigned long data)
{
  ::DISPsetTimeout(mid, data);
}

unsigned long NKDISPLAY::DISPreadTimeout(unsigned long mid)
{
  return ::DISPreadTimeout(mid);
}


