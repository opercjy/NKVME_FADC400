#ifndef _PVMEX_H_
#define _PVMEX_H_

// Wrapper for VME API
int Vopen(int devid);
int Vclose(int devid);
int Vwrite(int devid, unsigned long addr, unsigned long data, unsigned short am, unsigned short tout);
unsigned long Vread(int devid, unsigned long addr, unsigned short am, unsigned short tout);

#endif