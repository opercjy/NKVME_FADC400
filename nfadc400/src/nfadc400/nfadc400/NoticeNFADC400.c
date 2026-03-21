#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Notice6UVME.h"
#include "NoticeNFADC400.h"

// Open NFADC400
void NFADC400open(unsigned long mid)
{
  waitCPU(mid);
}

// Read Data Buffer
void NFADC400read_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data)
{
  unsigned long baseaddr, nch;
  unsigned long hevt;
  unsigned long num;
  unsigned long i;
  unsigned long addr;
  char rdat[0x100000];

  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0;
  
  if (range > 4)
    hevt = 4096 / range;
  else
    hevt = 512;
  
  num =  range << 7;
  
  if (nevt < hevt)
    addr = baseaddr + nch + nevt * 2 * num;
  else
    addr = baseaddr + nch + 0x100000 + (nevt - hevt) * 2 * num;
  
  VMEblockread(A32D16, 100, addr, 2 * num, rdat);
  
  for (i = 0; i < num; i++) {
    data[i] = rdat[2 * i + 1] & 0xFF;
    data[i] = data[i] << 8;
    data[i] = data[i] + (rdat[2 * i] & 0xFF);
  }
}

// Read Tagging Time
void NFADC400read_TTIME(unsigned long mid, unsigned long ch, unsigned long nevt, char *data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x200000 + nevt * 8;
  
  VMEblockread(A32D16, 100, addr, 6, data);
}

// Read Tagging Pattern
unsigned long NFADC400read_TPattern(unsigned long mid, unsigned long ch, unsigned long nevt)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x200000 + nevt * 8 + 6;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Dump Data Buffer
void NFADC400dump_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
{
  unsigned long baseaddr, nch;
  unsigned long num;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  if (page)
    addr = baseaddr + nch + 0x100000;
  else
    addr = baseaddr + nch;
  
  if (range > 4)
    num = 0x100000;
  else
    num = range << 17;
  
  VMEblockread(A32D16, 100, addr, num, data);
}

// Dump Tag
void NFADC400dump_TAG(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
{
  unsigned long baseaddr, nch;
  unsigned long num;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  if (range < 8)
    num = 4096;
  else
    num = 32768 / range;
  
  if (page)
    addr = baseaddr + nch + 0x200000 + num;
  else
    addr = baseaddr + nch + 0x200000;
  
  VMEblockread(A32D16, 100, addr, num, data);
}

// Write Offset Adjustment
void NFADC400write_DACOFF(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
  
  addr = baseaddr + nch + 0x280000;
  
  VMEwrite(A32D16, 100, addr, data);
}

// Read Offset Adjustment
unsigned long NFADC400read_DACOFF(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x280000;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Measure Pedestal
void NFADC400measure_PED(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x280002;

  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read Pedestal
unsigned long NFADC400read_PED(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x280002;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Amp Gain
void NFADC400write_DACGAIN(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
  
  addr = baseaddr + nch + 0x280004;
  
  VMEwrite(A32D16, 100, addr, data);
}

// Read Amp Gain
unsigned long NFADC400read_DACGAIN(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x280004;
  
  data = VMEread(A32D16, 100, addr);

  return data;
}

// Write Input Delay
void NFADC400write_DLY(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
  
  addr = baseaddr + nch + 0x280006;
  
  data = data / 10;
  
  VMEwrite(A32D16, 100, addr, data);
}

// Read Input Delay
unsigned long NFADC400read_DLY(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;
  
  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
  
  addr = baseaddr + nch + 0x280006;
  
  data = VMEread(A32D16, 100, addr);
  
  data = data * 10;
  
  return data;
}

// Read individual ADC Pedestal
unsigned long FADC400read_PEDADC(unsigned long mid, unsigned long ch, unsigned long adc)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280008 + adc * 2;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Input Pulse Polarity
void NFADC400write_POL(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
  
  addr = baseaddr + nch + 0x280010;
  
  VMEwrite(A32D16, 100, addr, data);
}

// Read Input Pulse Polarity
unsigned long NFADC400read_POL(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280010;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Discriminator Threshold
void NFADC400write_THR(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x280012;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Discriminator Threshold
unsigned long NFADC400read_THR(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280012;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Deadtime
void NFADC400write_DT(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x280014;

  data = data / 40960;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Deadtime
unsigned long NFADC400read_DT(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280014;

  data = VMEread(A32D16, 100, addr);

  data = data * 40960;

  return data;
}

// Write Coincidence Window
void NFADC400write_CW(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x280016;

  data = data / 160;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Coincidence Window
unsigned long NFADC400read_CW(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280016;

  data = VMEread(A32D16, 100, addr);

  data = data * 160;

  return data;
}

// Write Trigger Mode
void NFADC400write_TM(unsigned long mid, unsigned long ch, int ew, int en)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
  
  addr = baseaddr + nch + 0x280018;
	
  data = 0;

  if (ew)
    data = data | 0x02;

  if (en)
    data = data | 0x01;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Trigger Mode
unsigned long NFADC400read_TM(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x280018;

  data = VMEread(A32D16, 100, addr);

  return data;
}

// Write Pulse Count Threshold
void NFADC400write_PCT(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x28001A;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Pulse Count Threshold
unsigned long NFADC400read_PCT(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x28001A;

  data = VMEread(A32D16, 100, addr);

  return data;
}

// Write Pulse Count Interval
void NFADC400write_PCI(unsigned long mid, unsigned long ch, unsigned long data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x28001C;

  data = data / 40;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Pulse Count Interval
unsigned long NFADC400read_PCI(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x28001C;

  data = VMEread(A32D16, 100, addr);

  data = data * 40;

  return data;
}

// Write Pulse Width Threshold
void NFADC400write_PWT(unsigned long mid, unsigned long ch, float data)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  unsigned long value;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x500000;
				
  addr = baseaddr + nch + 0x28001E;

  data = data / 2.5;
  value = (unsigned long)data;
	
  VMEwrite(A32D16, 100, addr, value);
}

// Read Pulse Width Threshold
float NFADC400read_PWT(unsigned long mid, unsigned long ch)
{
  unsigned long baseaddr, nch;
  unsigned long addr;
  float data;
  unsigned long value;
	
  baseaddr = (mid & 0xFF) << 24;

  if (ch)
    nch = (ch - 1) << 22;
  else
    nch = 0x0;
				
  addr = baseaddr + nch + 0x28001E;

  value = VMEread(A32D16, 100, addr);

  data = value;
  	
  data = data * 2.5;

  return data;
}

// Send Reset signal
void NFADC400reset(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380000;

  VMEwrite(A32D16, 100, addr, 0x0);
}

// Write Reset Mode
void NFADC400write_RM(unsigned long mid, int st, int se, int sr)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380002;

  data = 0;

  if (st)
    data = data | 0x1;
  
  if (se)
    data = data | 0x2;
  
  if (sr)
    data = data | 0x4;
  
  VMEwrite(A32D16, 100, addr, data);
}

// Read Reset Mode
unsigned long NFADC400read_RM(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;
  
  addr = baseaddr + 0x380002;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Run Buffer 0
void NFADC400startL(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380004;

  VMEwrite(A32D16, 100, addr, 0x1);
}

// Stop Buffer 0
void NFADC400stopL(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380004;

  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read Buffer 0 Flag
unsigned long NFADC400read_RunL(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
  
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x38000A;

  VMEwrite(A32D16, 100, addr, 0x0);
				
  addr = baseaddr + 0x380004;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Run Buffer 1
void NFADC400startH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380006;

  VMEwrite(A32D16, 100, addr, 0x1);
}

// Stop Buffer 1
void NFADC400stopH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380006;

  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read Buffer 1 Flag
unsigned long NFADC400read_RunH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x38000A;

  VMEwrite(A32D16, 100, addr, 0x0);
				
  addr = baseaddr + 0x380006;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Segment Setting
void NFADC400write_RL(unsigned long mid, unsigned long data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380008;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Segment Setting
unsigned long NFADC400read_RL(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380008;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Read Timer
void NFADC400read_TIMER(unsigned long mid, char *data)
{
  unsigned long baseaddr;
  unsigned long addr;

  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x38000A;

//  VMEwrite(A32D16, 100, addr, 0x0);

  addr = baseaddr + 0x380010;

  VMEblockread(A32D16, 100, addr, 0x6, data);
}


// Read Event Number
unsigned long NFADC400read_ENUM(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;
  
  addr = baseaddr + 0x38000A;
  
  VMEwrite(A32D16, 100, addr, 0x0);
  
  addr = baseaddr + 0x380016;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Trigger Lookup Table
void NFADC400write_TLT(unsigned long mid, unsigned long data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380020;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Trigger Lookup Table
unsigned long NFADC400read_TLT(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380020;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Write Trigger Output Width
void NFADC400write_TOW(unsigned long mid,  unsigned long data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380022;

  if (data < 40)
    data = 0;
  else
    data = (data / 20) - 2;

  VMEwrite(A32D16, 100, addr, data);
}

// Read Trigger Output Width
unsigned long NFADC400read_TOW(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380022;

  data = VMEread(A32D16, 100, addr);

  data = (data + 2) * 20;

  return data;
}

// Enable Trigger Daisy Chain
void NFADC400enable_DCE(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380024;

  VMEwrite(A32D16, 100, addr, 0x0);
}

// Disable Trigger Daisy Chain
void NFADC400disable_DCE(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380024;

  VMEwrite(A32D16, 100, addr, 0x1);
}

// Read Trigger Daisy Chain setting
unsigned long NFADC400read_DCE(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380024;

  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Read Module Status
unsigned long NFADC400read_STAT(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00000;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Check CPU busy
unsigned long NFADC400check_CPU(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0x380026;

  data = VMEread(A32D16, 100, addr);

  return data;
}

// wait for CPU ready
void waitCPU(unsigned long mid)
{
  unsigned long flag;
	
  flag = 0;
  while (!flag) 
    flag = NFADC400check_CPU(mid);
}

// Reprogram FPGA
void NFADC400prog_FPGA(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;

  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00000;
  
  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read FPGA Status
unsigned long NFADC400stat_FPGA(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00000;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Read FPGA version
void NFADC400version_FPGA(unsigned long mid, char *data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00002;
  
  VMEblockread(A32D16, 100, addr, 6, data);
}

// Reprogram CPLD
void NFADC400prog_CPLD(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00008;
  
  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read CPLD Status
unsigned long NFADC400stat_CPLD(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00008;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// Read CPLD version
void NFADC400version_CPLD(unsigned long mid, char *data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB0000A;
  
  VMEblockread(A32D16, 100, addr, 6, data);
}

// Read serial number
void NFADC400read_SN(unsigned long mid, char *data)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB00010;
  
  VMEblockread(A32D16, 100, addr, 14, data);
}

// Enable flash memory programming
void NFADC400enable_FLASH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB0001E;
  
  VMEwrite(A32D16, 100, addr, 0x1);
}

// Disable flash memory programming
void NFADC400disable_FLASH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB0001E;
  
  VMEwrite(A32D16, 100, addr, 0x0);
}

// Read program switch for flash memory
unsigned long NFADC400check_FLASH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long addr;
  unsigned long data;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  addr = baseaddr + 0xB0001E;
  
  data = VMEread(A32D16, 100, addr);
  
  return data;
}

// write flash memory
void NFADC400write_FLASH(unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data)
{
  unsigned long baseaddr;
  unsigned long waddr;
  unsigned long wdat;
	
  waitCPU(mid);
	
  baseaddr = (mid & 0xFF) << 24;

  waddr = baseaddr + 0xF00000 + ((addr & 0x7FFFF) << 1);
  wdat = opcode & 0xFF;
  wdat = wdat << 8;
  wdat = wdat + (data & 0xFF);
  
  VMEwrite(A32D16, 100, waddr, wdat);
}

// Read flash memory
unsigned long NFADC400read_FLASH(unsigned long mid, unsigned long addr)
{
  unsigned long baseaddr;
  unsigned long raddr;
  unsigned long rdat;
  
  baseaddr = (mid & 0xFF) << 24;
  
  raddr = baseaddr + 0xF00000;
   	
  NFADC400write_FLASH(mid, 0x03, addr, 0x00);
    
  waitCPU(mid);
	
  rdat = VMEread(A32D16, 100, raddr);
  
  return rdat;
}

// Read status of flash memory
unsigned long NFADC400stat_FLASH(unsigned long mid)
{
  unsigned long baseaddr;
  unsigned long raddr;
  unsigned long rdat;
  
  baseaddr = (mid & 0xFF) << 24;

  raddr = baseaddr + 0xF00000;
   	
  NFADC400write_FLASH(mid, 0x05, 0x00, 0x00);
    
  waitCPU(mid);
  
  rdat = VMEread(A32D16, 100, raddr) & 0x01;
  
  return rdat;
}

// Update FPGA
void NFADC400update_FPGA(unsigned long mid, char *filename)
{
  FILE *fp;
  unsigned long addr;
  unsigned long flag;
  unsigned long i;
  unsigned char rdat;
	
  // enable flash memory programming
  NFADC400enable_FLASH(mid);
	
  // unprotec sector 0-3
  for (i = 0; i < 4; i++) {
    addr = i << 16;
    NFADC400write_FLASH(mid, 0x39, addr, 0);
  }
	
  // erase sector 0-3
  for (i = 0; i < 4; i++) {
    addr = i << 16;
    NFADC400write_FLASH(mid, 0xD8, addr, 0);
    
    sleep(2);
		
    // wait for block being erased
    flag = 1;
    while (flag)
      flag = NFADC400stat_FLASH(mid);
  }
	
  // open bit stream file
  fp = fopen(filename, "rb");
	
  // write FPGA downloading data
  for (addr = 0; addr < 0x3F000; addr++) {
    fscanf(fp, "%c", &rdat);
    NFADC400write_FLASH(mid, 0x02, addr, rdat);
  }
	
  fclose(fp);
	
  // write FPGA firmware version
  for (i = 0; i < 6; i++) {
    addr = 0x3F000 + i;
    NFADC400write_FLASH(mid, 0x02, addr, filename[i + 11] & 0xFF);
  }
			
  // protec sector 0-3
  for (i = 0; i < 4; i++) {
    addr = i << 16;
    NFADC400write_FLASH(mid, 0x36, addr, 0);
  }
  
  // disable flash memory programming
  NFADC400disable_FLASH(mid);
  
  // re-download FPGA
  NFADC400prog_FPGA(mid);
	
  waitCPU(mid);
}

// Update CPLD
void NFADC400update_CPLD(unsigned long mid, char *filename)
{
  FILE *fp;
  unsigned long addr;
  unsigned long flag;
  unsigned long i, j, k, l;
  unsigned long iindex;
  unsigned char rdat[0x6540];
  char cmd[260];
  char cdat;
  
  // open bit stream file
  fp = fopen(filename, "rt");

  flag = 1;

  // find the starting pattern
  while (flag) {
    fgets(cmd, 256, fp);
    if (cmd[0] == 'L')
      flag = 0;
  }

  for (i = 0; i < 108; i++) {
    for (j = 0; j < 9; j++) {
      for (k = 0; k < 16; k++) {
	iindex = i * 240 + j * 16 + k;
			
	rdat[iindex] = 0;
	for (l = 0; l < 8; l++) {
	  cdat = cmd[9 * k + l + 9];
	  if (cdat == '1')
	    rdat[iindex] = rdat[iindex] + (1 << l);
	}					
      }
      
      fgets(cmd, 256, fp); 
    }
    
    for (j = 0; j < 6; j++) {
      for (k = 0; k < 16; k++) {
	iindex = i * 240 + j * 16 + k + 144;
	
	rdat[iindex] = 0;
	for (l = 0; l < 6; l++) {
	  cdat = cmd[7 * k + l + 9];
	  if (cdat == '1')
	    rdat[iindex] = rdat[iindex] + (1 << l);
	}					
      }
      
      fgets(cmd, 256, fp); 
    }
  }
		
  fclose(fp);

  // enable flash memory programming
  NFADC400enable_FLASH(mid);
	
  // unprotec sector 4
  addr = 4 << 16;
  NFADC400write_FLASH(mid, 0x39, addr, 0);
	
  // erase sector 4
  addr = 4 << 16;
  NFADC400write_FLASH(mid, 0xD8, addr, 0);
	
  sleep(2);
		
  // wait for block being erased
  flag = 1;
  while (flag)
    flag = NFADC400stat_FLASH(mid);
	
  // write CPLD downloading data
  for (i = 0; i < 0x6540; i++) {
    addr = 0x40000 + i;
    NFADC400write_FLASH(mid, 0x02, addr, rdat[i]);
  }
	
  // write CPLD firmware version
  for (i = 0; i < 6; i++) {
    addr = 0x4F000 + i;
    NFADC400write_FLASH(mid, 0x02, addr, filename[i + 11] & 0xFF);
  }
  
  // protec sector 4
  addr = 4 << 16;
  NFADC400write_FLASH(mid, 0x36, addr, 0);
  
  // disable flash memory programming
  NFADC400disable_FLASH(mid);
	
  // re-program CPLD
  NFADC400prog_CPLD(mid);

  waitCPU(mid);
}

// Update ADC lookup table
void NFADC400update_ALUT(unsigned long mid, unsigned long ch, char *filename)
{
  FILE *fp;
  unsigned long addr;
  unsigned long flag;
  unsigned long i;
  unsigned char tdat;
  int tmp;
			
  // open TLT file
  fp = fopen(filename, "rt");
	
  // enable flash memory programming
  NFADC400enable_FLASH(mid);
	
  // unprotec sector 5
  addr = 5 << 16;
  NFADC400write_FLASH(mid, 0x39, addr, 0);
	
  // erase sector 5
  addr = 5 << 16;
  addr = addr + (ch -1) * 0x2000;
  NFADC400write_FLASH(mid, 0x20, addr, 0);
		
  sleep(1);
			
  // wait for block being erased
  flag = 1;
  while (flag)
    flag = NFADC400stat_FLASH(mid);

  addr = addr + 0x1000;
  NFADC400write_FLASH(mid, 0x20, addr, 0);
		
  sleep(1);
			
  // wait for block being erased
  flag = 1;
  while (flag)
    flag = NFADC400stat_FLASH(mid);
	
  // set starting flash memory address
  addr = 0x50000 + (ch - 1) * 0x2000;

  for (i = 0; i < 0x1800; i++) {
    fscanf(fp, "%d", &tmp);
    
    tdat = tmp & 0xFF;
	
    NFADC400write_FLASH(mid, 0x02, addr, tdat);
    
    addr = addr + 1;
  }
  
  fclose(fp);

  // protec sector 5
  addr = 5 << 16;
  NFADC400write_FLASH(mid, 0x36, addr, 0);
	
  // disable flash memory programming
  NFADC400disable_FLASH(mid);
  
  // re-downloading FPGA & trigger lookup table
  NFADC400prog_FPGA(mid);

  // wait for CPU ready
  waitCPU(mid);
}





