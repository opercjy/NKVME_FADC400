#include "pvmex.h"
#include "Notice6UVME.h" // C Library Header

int Vopen(int devid) {
    return VMEopen(devid);
}

int Vclose(int devid) {
    return VMEclose(devid);
}

int Vwrite(int devid, unsigned long addr, unsigned long data, unsigned short am, unsigned short tout) {
    return VMEwrite(devid, am, tout, addr, data);
}

unsigned long Vread(int devid, unsigned long addr, unsigned short am, unsigned short tout) {
    return VMEread(devid, am, tout, addr);
}