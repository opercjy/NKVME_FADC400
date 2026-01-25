/*
 * NoticeNFADC400 Implementation
 * Refactored for Rocky Linux 9 & ROOT 6
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // sleep

#include "NoticeNFADC400.h"

#ifdef NKROOT
  #include "TROOT.h"
  ClassImp(NKNFADC400)
#endif

// -----------------------------------------------------------------------------
// Constructor / Destructor
// -----------------------------------------------------------------------------
#ifndef NKC
NKNFADC400::NKNFADC400() {
    printf("NKNFADC400 Initialization \n");
}

NKNFADC400::~NKNFADC400() {
    printf("Leaving NKNFADC400 now \n");
}
#endif

// -----------------------------------------------------------------------------
// Open & Wait CPU
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400open(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400open(int devnum, unsigned long mid)
#endif
{
    waitCPU(devnum, mid);
}

// -----------------------------------------------------------------------------
// Read Data Buffer
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400read_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data)
#else
void NKNFADC400::NFADC400read_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long hevt;
    unsigned long num;
    unsigned long i;
    unsigned long addr;
    char rdat[0x100000]; // 1MB buffer

    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0;
  
    if (range > 4) hevt = 4096 / range;
    else hevt = 512;
  
    num = range << 7;
  
    if (nevt < hevt)
        addr = baseaddr + nch + nevt * 2 * num;
    else
        addr = baseaddr + nch + 0x100000 + (nevt - hevt) * 2 * num;
  
    VMEblockread(devnum, A32D16, 100, addr, 2 * num, rdat);
  
    for (i = 0; i < num; i++) {
        data[i] = rdat[2 * i + 1] & 0xFF;
        data[i] = data[i] << 8;
        data[i] = data[i] + (rdat[2 * i] & 0xFF);
    }
}

// -----------------------------------------------------------------------------
// Read Tagging Time
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400read_TTIME(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt, char *data)
#else
void NKNFADC400::NFADC400read_TTIME(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt, char *data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x200000 + nevt * 8;
  
    VMEblockread(devnum, A32D16, 100, addr, 6, data);
}

// -----------------------------------------------------------------------------
// Read Tagging Pattern
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long NFADC400read_TPattern(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt)
#else
unsigned long NKNFADC400::NFADC400read_TPattern(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x200000 + nevt * 8 + 6;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Dump Data Buffer
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400dump_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
#else
void NKNFADC400::NFADC400dump_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long num;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    if (page) addr = baseaddr + nch + 0x100000;
    else addr = baseaddr + nch;
  
    if (range > 4) num = 0x100000;
    else num = range << 17;
  
    VMEblockread(devnum, A32D16, 100, addr, num, data);
}

// -----------------------------------------------------------------------------
// Dump Tag
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400dump_TAG(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
#else
void NKNFADC400::NFADC400dump_TAG(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long num;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    if (range < 8) num = 4096;
    else num = 32768 / range;
  
    if (page) addr = baseaddr + nch + 0x200000 + num;
    else addr = baseaddr + nch + 0x200000;
  
    VMEblockread(devnum, A32D16, 100, addr, num, data);
}

// -----------------------------------------------------------------------------
// DACOFF
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_DACOFF(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_DACOFF(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
  
    addr = baseaddr + nch + 0x280000;
  
    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_DACOFF(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_DACOFF(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x280000;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Measure/Read Pedestal
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400measure_PED(int devnum, unsigned long mid, unsigned long ch)
#else
void NKNFADC400::NFADC400measure_PED(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x280002;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400read_PED(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_PED(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x280002;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// DACGAIN
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_DACGAIN(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_DACGAIN(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
  
    addr = baseaddr + nch + 0x280004;
  
    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_DACGAIN(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_DACGAIN(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x280004;
  
    data = VMEread(devnum, A32D16, 100, addr);

    return data;
}

// -----------------------------------------------------------------------------
// Delay (DLY)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_DLY(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_DLY(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
  
    addr = baseaddr + nch + 0x280006;
  
    data = data / 10;
  
    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_DLY(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_DLY(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;
  
    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
  
    addr = baseaddr + nch + 0x280006;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    data = data * 10;
  
    return data;
}

// -----------------------------------------------------------------------------
// Read individual ADC Pedestal
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long FADC400read_PEDADC(int devnum, unsigned long mid, unsigned long ch, unsigned long adc)
#else
unsigned long NKNFADC400::FADC400read_PEDADC(int devnum, unsigned long mid, unsigned long ch, unsigned long adc)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280008 + adc * 2;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Polarity (POL)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_POL(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_POL(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
  
    addr = baseaddr + nch + 0x280010;
  
    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_POL(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_POL(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280010;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Threshold (THR)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_THR(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_THR(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x280012;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_THR(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_THR(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280012;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Deadtime (DT)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_DT(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_DT(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x280014;

    data = data / 40960;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_DT(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_DT(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280014;

    data = VMEread(devnum, A32D16, 100, addr);

    data = data * 40960;

    return data;
}

// -----------------------------------------------------------------------------
// Coincidence Window (CW)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_CW(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_CW(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x280016;

    data = data / 160;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_CW(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_CW(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280016;

    data = VMEread(devnum, A32D16, 100, addr);

    data = data * 160;

    return data;
}

// -----------------------------------------------------------------------------
// Trigger Mode (TM)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_TM(int devnum, unsigned long mid, unsigned long ch, int ew, int en)
#else
void NKNFADC400::NFADC400write_TM(int devnum, unsigned long mid, unsigned long ch, int ew, int en)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
  
    addr = baseaddr + nch + 0x280018;
	
    data = 0;

    if (ew) data = data | 0x02;
    if (en) data = data | 0x01;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_TM(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_TM(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x280018;

    data = VMEread(devnum, A32D16, 100, addr);

    return data;
}

// -----------------------------------------------------------------------------
// Pulse Count Threshold (PCT)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_PCT(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_PCT(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x28001A;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_PCT(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_PCT(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x28001A;

    data = VMEread(devnum, A32D16, 100, addr);

    return data;
}

// -----------------------------------------------------------------------------
// Pulse Count Interval (PCI)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_PCI(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#else
void NKNFADC400::NFADC400write_PCI(int devnum, unsigned long mid, unsigned long ch, unsigned long data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x28001C;

    data = data / 40;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_PCI(int devnum, unsigned long mid, unsigned long ch)
#else
unsigned long NKNFADC400::NFADC400read_PCI(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x28001C;

    data = VMEread(devnum, A32D16, 100, addr);

    data = data * 40;

    return data;
}

// -----------------------------------------------------------------------------
// Pulse Width Threshold (PWT)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_PWT(int devnum, unsigned long mid, unsigned long ch, float data)
#else
void NKNFADC400::NFADC400write_PWT(int devnum, unsigned long mid, unsigned long ch, float data)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    unsigned long value;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x500000;
				
    addr = baseaddr + nch + 0x28001E;

    data = data / 2.5;
    value = (unsigned long)data;
	
    VMEwrite(devnum, A32D16, 100, addr, value);
}

#ifdef NKC
float NFADC400read_PWT(int devnum, unsigned long mid, unsigned long ch)
#else
float NKNFADC400::NFADC400read_PWT(int devnum, unsigned long mid, unsigned long ch)
#endif
{
    unsigned long baseaddr, nch;
    unsigned long addr;
    float data;
    unsigned long value;
	
    baseaddr = (mid & 0xFF) << 24;

    if (ch) nch = (ch - 1) << 22;
    else nch = 0x0;
				
    addr = baseaddr + nch + 0x28001E;

    value = VMEread(devnum, A32D16, 100, addr);

    data = value;
    data = data * 2.5;

    return data;
}

// -----------------------------------------------------------------------------
// Reset (RST)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400reset(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400reset(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380000;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

// -----------------------------------------------------------------------------
// Reset Mode (RM)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_RM(int devnum, unsigned long mid, int st, int se, int sr)
#else
void NKNFADC400::NFADC400write_RM(int devnum, unsigned long mid, int st, int se, int sr)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380002;

    data = 0;

    if (st) data = data | 0x1;
    if (se) data = data | 0x2;
    if (sr) data = data | 0x4;
  
    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_RM(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_RM(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;
  
    addr = baseaddr + 0x380002;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Start/Stop Buffer L
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400startL(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400startL(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380004;

    VMEwrite(devnum, A32D16, 100, addr, 0x1);
}

#ifdef NKC
void NFADC400stopL(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400stopL(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380004;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400read_RunL(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_RunL(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
  
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x38000A;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
				
    addr = baseaddr + 0x380004;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Start/Stop Buffer H
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400startH(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400startH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380006;

    VMEwrite(devnum, A32D16, 100, addr, 0x1);
}

#ifdef NKC
void NFADC400stopH(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400stopH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380006;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400read_RunH(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_RunH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x38000A;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
				
    addr = baseaddr + 0x380006;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Record Length (RL)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_RL(int devnum, unsigned long mid, unsigned long data)
#else
void NKNFADC400::NFADC400write_RL(int devnum, unsigned long mid, unsigned long data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380008;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_RL(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_RL(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380008;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Read Timer
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400read_TIMER(int devnum, unsigned long mid, char *data)
#else
void NKNFADC400::NFADC400read_TIMER(int devnum, unsigned long mid, char *data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;

    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x38000A;

    // VMEwrite(devnum, A32D16, 100, addr, 0x0);

    addr = baseaddr + 0x380010;

    VMEblockread(devnum, A32D16, 100, addr, 0x6, data);
}

// -----------------------------------------------------------------------------
// Read Event Number (ENUM)
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long NFADC400read_ENUM(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_ENUM(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;
  
    addr = baseaddr + 0x38000A;
  
    VMEwrite(devnum, A32D16, 100, addr, 0x0);
  
    addr = baseaddr + 0x380016;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Trigger Lookup Table (TLT)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_TLT(int devnum, unsigned long mid, unsigned long data)
#else
void NKNFADC400::NFADC400write_TLT(int devnum, unsigned long mid, unsigned long data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380020;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_TLT(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_TLT(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380020;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Trigger Output Width (TOW)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400write_TOW(int devnum, unsigned long mid,  unsigned long data)
#else
void NKNFADC400::NFADC400write_TOW(int devnum, unsigned long mid,  unsigned long data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380022;

    if (data < 40) data = 0;
    else data = (data / 20) - 2;

    VMEwrite(devnum, A32D16, 100, addr, data);
}

#ifdef NKC
unsigned long NFADC400read_TOW(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_TOW(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380022;

    data = VMEread(devnum, A32D16, 100, addr);

    data = (data + 2) * 20;

    return data;
}

// -----------------------------------------------------------------------------
// Daisy Chain (DCE)
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400enable_DCE(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400enable_DCE(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380024;

    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
void NFADC400disable_DCE(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400disable_DCE(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380024;

    VMEwrite(devnum, A32D16, 100, addr, 0x1);
}

#ifdef NKC
unsigned long NFADC400read_DCE(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_DCE(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380024;

    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// Module Status (STAT)
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long NFADC400read_STAT(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400read_STAT(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00000;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

// -----------------------------------------------------------------------------
// CPU Check
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long NFADC400check_CPU(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400check_CPU(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0x380026;

    data = VMEread(devnum, A32D16, 100, addr);

    return data;
}

#ifdef NKC
void waitCPU(int devnum, unsigned long mid)
#else
void NKNFADC400::waitCPU(int devnum, unsigned long mid)
#endif
{
    unsigned long flag;
	
    flag = 0;
    while (!flag) 
        flag = NFADC400check_CPU(devnum, mid);
}

// -----------------------------------------------------------------------------
// FPGA Operations
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400prog_FPGA(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400prog_FPGA(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;

    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00000;
  
    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400stat_FPGA(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400stat_FPGA(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00000;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

#ifdef NKC
void NFADC400version_FPGA(int devnum, unsigned long mid, char *data)
#else
void NKNFADC400::NFADC400version_FPGA(int devnum, unsigned long mid, char *data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00002;
  
    VMEblockread(devnum, A32D16, 100, addr, 6, data);
}

// -----------------------------------------------------------------------------
// CPLD Operations
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400prog_CPLD(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400prog_CPLD(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00008;
  
    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400stat_CPLD(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400stat_CPLD(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00008;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

#ifdef NKC
void NFADC400version_CPLD(int devnum, unsigned long mid, char *data)
#else
void NKNFADC400::NFADC400version_CPLD(int devnum, unsigned long mid, char *data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB0000A;
  
    VMEblockread(devnum, A32D16, 100, addr, 6, data);
}

// -----------------------------------------------------------------------------
// Read Serial Number
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400read_SN(int devnum, unsigned long mid, char *data)
#else
void NKNFADC400::NFADC400read_SN(int devnum, unsigned long mid, char *data)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB00010;
  
    VMEblockread(devnum, A32D16, 100, addr, 14, data);
}

// -----------------------------------------------------------------------------
// Flash Memory Operations
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400enable_FLASH(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400enable_FLASH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB0001E;
  
    VMEwrite(devnum, A32D16, 100, addr, 0x1);
}

#ifdef NKC
void NFADC400disable_FLASH(int devnum, unsigned long mid)
#else
void NKNFADC400::NFADC400disable_FLASH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB0001E;
  
    VMEwrite(devnum, A32D16, 100, addr, 0x0);
}

#ifdef NKC
unsigned long NFADC400check_FLASH(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400check_FLASH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long addr;
    unsigned long data;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    addr = baseaddr + 0xB0001E;
  
    data = VMEread(devnum, A32D16, 100, addr);
  
    return data;
}

#ifdef NKC
void NFADC400write_FLASH(int devnum, unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data)
#else
void NKNFADC400::NFADC400write_FLASH(int devnum, unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data)
#endif
{
    unsigned long baseaddr;
    unsigned long waddr;
    unsigned long wdat;
	
    waitCPU(devnum, mid);
	
    baseaddr = (mid & 0xFF) << 24;

    waddr = baseaddr + 0xF00000 + ((addr & 0x7FFFF) << 1);
    wdat = opcode & 0xFF;
    wdat = wdat << 8;
    wdat = wdat + (data & 0xFF);
  
    VMEwrite(devnum, A32D16, 100, waddr, wdat);
}

#ifdef NKC
unsigned long NFADC400read_FLASH(int devnum, unsigned long mid, unsigned long addr)
#else
unsigned long NKNFADC400::NFADC400read_FLASH(int devnum, unsigned long mid, unsigned long addr)
#endif
{
    unsigned long baseaddr;
    unsigned long raddr;
    unsigned long rdat;
  
    baseaddr = (mid & 0xFF) << 24;
  
    raddr = baseaddr + 0xF00000;
   	
    NFADC400write_FLASH(devnum, mid, 0x03, addr, 0x00);
    
    waitCPU(devnum, mid);
	
    rdat = VMEread(devnum, A32D16, 100, raddr);
  
    return rdat;
}

#ifdef NKC
unsigned long NFADC400stat_FLASH(int devnum, unsigned long mid)
#else
unsigned long NKNFADC400::NFADC400stat_FLASH(int devnum, unsigned long mid)
#endif
{
    unsigned long baseaddr;
    unsigned long raddr;
    unsigned long rdat;
  
    baseaddr = (mid & 0xFF) << 24;

    raddr = baseaddr + 0xF00000;
   	
    NFADC400write_FLASH(devnum, mid, 0x05, 0x00, 0x00);
    
    waitCPU(devnum, mid);
  
    rdat = VMEread(devnum, A32D16, 100, raddr) & 0x01;
  
    return rdat;
}

// -----------------------------------------------------------------------------
// Update FPGA
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400update_FPGA(int devnum, unsigned long mid, char *filename)
#else
void NKNFADC400::NFADC400update_FPGA(int devnum, unsigned long mid, char *filename)
#endif
{
    FILE *fp;
    unsigned long addr;
    unsigned long flag;
    unsigned long i;
    unsigned char rdat;
	
    // enable flash memory programming
    NFADC400enable_FLASH(devnum, mid);
	
    // unprotect sector 0-3
    for (i = 0; i < 4; i++) {
        addr = i << 16;
        NFADC400write_FLASH(devnum, mid, 0x39, addr, 0);
    }
	
    // erase sector 0-3
    for (i = 0; i < 4; i++) {
        addr = i << 16;
        NFADC400write_FLASH(devnum, mid, 0xD8, addr, 0);
    
        sleep(2);
		
        // wait for block being erased
        flag = 1;
        while (flag)
            flag = NFADC400stat_FLASH(devnum, mid);
    }
	
    // open bit stream file
    fp = fopen(filename, "rb");
	
    // write FPGA downloading data
    for (addr = 0; addr < 0x3F000; addr++) {
        fscanf(fp, "%c", &rdat);
        NFADC400write_FLASH(devnum, mid, 0x02, addr, rdat);
    }
	
    fclose(fp);
	
    // write FPGA firmware version
    for (i = 0; i < 6; i++) {
        addr = 0x3F000 + i;
        NFADC400write_FLASH(devnum, mid, 0x02, addr, filename[i + 11] & 0xFF);
    }
			
    // protect sector 0-3
    for (i = 0; i < 4; i++) {
        addr = i << 16;
        NFADC400write_FLASH(devnum, mid, 0x36, addr, 0);
    }
  
    // disable flash memory programming
    NFADC400disable_FLASH(devnum, mid);
  
    // re-download FPGA
    NFADC400prog_FPGA(devnum, mid);
	
    waitCPU(devnum, mid);
}

// -----------------------------------------------------------------------------
// Update CPLD
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400update_CPLD(int devnum, unsigned long mid, char *filename)
#else
void NKNFADC400::NFADC400update_CPLD(int devnum, unsigned long mid, char *filename)
#endif
{
    FILE *fp;
    unsigned long addr;
    unsigned long flag;
    unsigned long i, j, k, l;
    unsigned long index;
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
                index = i * 240 + j * 16 + k;
			
                rdat[index] = 0;
                for (l = 0; l < 8; l++) {
                    cdat = cmd[9 * k + l + 9];
                    if (cdat == '1')
                        rdat[index] = rdat[index] + (1 << l);
                }					
            }
      
            fgets(cmd, 256, fp); 
        }
    
        for (j = 0; j < 6; j++) {
            for (k = 0; k < 16; k++) {
                index = i * 240 + j * 16 + k + 144;
	
                rdat[index] = 0;
                for (l = 0; l < 6; l++) {
                    cdat = cmd[7 * k + l + 9];
                    if (cdat == '1')
                        rdat[index] = rdat[index] + (1 << l);
                }					
            }
      
            fgets(cmd, 256, fp); 
        }
    }
		
    fclose(fp);

    // enable flash memory programming
    NFADC400enable_FLASH(devnum, mid);
	
    // unprotect sector 4
    addr = 4 << 16;
    NFADC400write_FLASH(devnum, mid, 0x39, addr, 0);
	
    // erase sector 4
    addr = 4 << 16;
    NFADC400write_FLASH(devnum, mid, 0xD8, addr, 0);
	
    sleep(2);
		
    // wait for block being erased
    flag = 1;
    while (flag)
        flag = NFADC400stat_FLASH(devnum, mid);
	
    // write CPLD downloading data
    for (i = 0; i < 0x6540; i++) {
        addr = 0x40000 + i;
        NFADC400write_FLASH(devnum, mid, 0x02, addr, rdat[i]);
    }
	
    // write CPLD firmware version
    for (i = 0; i < 6; i++) {
        addr = 0x4F000 + i;
        NFADC400write_FLASH(devnum, mid, 0x02, addr, filename[i + 11] & 0xFF);
    }
  
    // protect sector 4
    addr = 4 << 16;
    NFADC400write_FLASH(devnum, mid, 0x36, addr, 0);
  
    // disable flash memory programming
    NFADC400disable_FLASH(devnum, mid);
	
    // re-program CPLD
    NFADC400prog_CPLD(devnum, mid);

    waitCPU(devnum, mid);
}

// -----------------------------------------------------------------------------
// Update ALUT
// -----------------------------------------------------------------------------
#ifdef NKC
void NFADC400update_ALUT(int devnum, unsigned long mid, unsigned long ch, char *filename)
#else
void NKNFADC400::NFADC400update_ALUT(int devnum, unsigned long mid, unsigned long ch, char *filename)
#endif
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
    NFADC400enable_FLASH(devnum, mid);
	
    // unprotect sector 5
    addr = 5 << 16;
    NFADC400write_FLASH(devnum, mid, 0x39, addr, 0);
	
    // erase sector 5
    addr = 5 << 16;
    addr = addr + (ch -1) * 0x2000;
    NFADC400write_FLASH(devnum, mid, 0x20, addr, 0);
		
    sleep(1);
			
    // wait for block being erased
    flag = 1;
    while (flag)
        flag = NFADC400stat_FLASH(devnum, mid);

    addr = addr + 0x1000;
    NFADC400write_FLASH(devnum, mid, 0x20, addr, 0);
		
    sleep(1);
			
    // wait for block being erased
    flag = 1;
    while (flag)
        flag = NFADC400stat_FLASH(devnum, mid);
	
    // set starting flash memory address
    addr = 0x50000 + (ch - 1) * 0x2000;

    for (i = 0; i < 0x1800; i++) {
        fscanf(fp, "%d", &tmp);
    
        tdat = tmp & 0xFF;
	
        NFADC400write_FLASH(devnum, mid, 0x02, addr, tdat);
    
        addr = addr + 1;
    }
  
    fclose(fp);

    // protect sector 5
    addr = 5 << 16;
    NFADC400write_FLASH(devnum, mid, 0x36, addr, 0);
	
    // disable flash memory programming
    NFADC400disable_FLASH(devnum, mid);
  
    // re-downloading FPGA & trigger lookup table
    NFADC400prog_FPGA(devnum, mid);

    // wait for CPU ready
    waitCPU(devnum, mid);
}