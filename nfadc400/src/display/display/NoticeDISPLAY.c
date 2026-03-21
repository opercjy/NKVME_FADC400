#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Notice6UVME.h"
#include "NoticeDISPLAY.h"

// write data to display module
void DISPwriteData(unsigned long mid, unsigned long data)
{
  unsigned long baseaddr;
  unsigned long addr;

  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr;

  VMEwrite(A32D32, 0, addr, data);
}

// read data from display module
unsigned long DISPreadData(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;

  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr;

  data = VMEread(A32D32, 0, addr);

  return data;
}


// set timeout in usec
void DISPsetTimeout(unsigned long mid, unsigned long data)
{
  unsigned long baseaddr;
  unsigned long addr;

  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x4;

  VMEwrite(A32D32, 0, addr, data);
}

// read timout setting from display module
unsigned long DISPreadTimeout(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;

  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x4;

  data = VMEread(A32D32, 0, addr);

  return data;
}





