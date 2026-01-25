/*
 * Notice6UVME Protocol Implementation
 * Refactored for Rocky Linux 9 & ROOT 6
 */

#include <stdio.h>
#include <unistd.h> // for sleep
#include "Notice6UVME.h"

#ifdef NKROOT
  #include "TROOT.h"
  ClassImp(NK6UVME)
#endif

// -----------------------------------------------------------------------------
// Constructor / Destructor (C++ Only)
// -----------------------------------------------------------------------------
#ifndef NKC
NK6UVME::NK6UVME() {
    printf("NK6UVME Initialization \n");
}

NK6UVME::~NK6UVME() {
    printf("Leaving NK6UVME now \n");
}
#endif

// -----------------------------------------------------------------------------
// VME Open
// -----------------------------------------------------------------------------
#ifdef NKC
int VMEopen(int devnum)
#else
int NK6UVME::VMEopen(int devnum)
#endif
{
    return uopen(devnum);
}

// -----------------------------------------------------------------------------
// VME Close
// -----------------------------------------------------------------------------
#ifdef NKC
int VMEclose(int devnum)
#else
int NK6UVME::VMEclose(int devnum)
#endif
{
    return uclose(devnum);
}

// -----------------------------------------------------------------------------
// VME Write Cycle (with Retry)
// -----------------------------------------------------------------------------
#ifdef NKC
int VMEwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
#else
int NK6UVME::VMEwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
#endif
{
    int flag = -1;
    int ntry = 0;

    while (flag < 0) {
        if (ntry > 5) {
            printf(" 6UVME : VMEwrite does not respond. After 5 try, giving up! \n");
            return -99;
        }
        ntry++;
        
        // Call internal Vwrite
        flag = Vwrite(devnum, am, tout, address, data);
        
        if (flag < 0) {
            printf(" 6UVME : VMEwrite fail. Resetting connection (Try %d)\n", ntry);
            VMEclose(devnum);
            VMEopen(devnum);
        }
    }
    return 0;
}

// -----------------------------------------------------------------------------
// VME Read Cycle (with Retry)
// -----------------------------------------------------------------------------
#ifdef NKC
unsigned long VMEread(int devnum, unsigned short am, unsigned short tout, unsigned long address)
#else
unsigned long NK6UVME::VMEread(int devnum, unsigned short am, unsigned short tout, unsigned long address)
#endif
{
    unsigned long data[2];
    int flag = -1;
    int ntry = 0;

    while (flag < 0) {
        if (ntry > 5) {
            printf(" 6UVME : VMEread does not respond. After 5 try, giving up! \n");
            return 0;      
        }
        ntry++;
        
        // Call internal Vread
        flag = Vread(devnum, am, tout, address, data);
        
        if (flag < 0) {
            printf(" 6UVME : VMEread fail. Resetting connection (Try %d)\n", ntry);
            VMEclose(devnum);
            VMEopen(devnum);
        }
    }
    return data[0];
}

// -----------------------------------------------------------------------------
// VME Block Read Cycle (with Retry)
// -----------------------------------------------------------------------------
#ifdef NKC
int VMEblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data)
#else
int NK6UVME::VMEblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data)
#endif
{
    int flag = -1;
    int ntry = 0;

    while (flag < 0) {
        if (ntry > 5) {
            printf(" 6UVME : VMEblockread does not respond. After 5 try, giving up! \n");
            return -99;
        }
        ntry++;
        
        // Call internal Vblockread
        flag = Vblockread(devnum, am, tout, address, counts, data);
        
        if (flag < 0) {
            printf(" 6UVME : VMEblockread fail. Resetting connection (Try %d)\n", ntry);
            VMEclose(devnum);
            VMEopen(devnum);
        }
    }
    return 0;
}

// -----------------------------------------------------------------------------
// Internal: Vwrite (Packet Construction)
// -----------------------------------------------------------------------------
#ifdef NKC
int Vwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
#else
int NK6UVME::Vwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
#endif
{
    char wbuf[16]; // Buffer for Endpoint 6 write
    int flag;

    wbuf[0] = 0x00;
    wbuf[1] = 0x00;
    wbuf[2] = am & 0xFF;           // AM Code
    wbuf[3] = tout & 0xFF;         // Timeout
    wbuf[4] = address & 0xFF;      // Address LSB
    wbuf[5] = (address >> 8) & 0xFF;
    wbuf[6] = (address >> 16) & 0xFF;
    wbuf[7] = (address >> 24) & 0xFF; // Address MSB
    wbuf[8] = data & 0xFF;         // Data LSB
    wbuf[9] = (data >> 8) & 0xFF;
    wbuf[10] = (data >> 16) & 0xFF;
    wbuf[11] = (data >> 24) & 0xFF; // Data MSB

    flag = uwrite(devnum, wbuf, 12);
    if (flag < 0) return flag;

    return 0;
}

// -----------------------------------------------------------------------------
// Internal: Vread (Packet Construction & Readback)
// -----------------------------------------------------------------------------
#ifdef NKC
int Vread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long *data)
#else
int NK6UVME::Vread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long *data)
#endif
{
    char wbuf[16];
    char rbuf[16];
    unsigned int lword;
    unsigned long temp;
    unsigned char vmemode;
    int flag;
  
    // Set READ bit (0x80)
    vmemode = am | 0x0080; 
    
    // Check LWORD bit (0x40) to determine data width
    lword = vmemode & 0x40;

    wbuf[0] = 0x00;
    wbuf[1] = 0x00;
    wbuf[2] = vmemode & 0xFF;
    wbuf[3] = tout & 0xFF;
    wbuf[4] = address & 0xFF;
    wbuf[5] = (address >> 8) & 0xFF;
    wbuf[6] = (address >> 16) & 0xFF;
    wbuf[7] = (address >> 24) & 0xFF;

    // Set Data length bytes based on LWORD
    if (lword) {
        wbuf[8] = 2; // 2 bytes
    } else {
        wbuf[8] = 4; // 4 bytes
    }
    wbuf[9] = 0;
    wbuf[10] = 0;
    wbuf[11] = 0;

    // Send Request
    flag = uwrite(devnum, wbuf, 12);
    if (flag < 0) return flag;

    // Read Response
    if (lword) {
        flag = uread(devnum, rbuf, 2);
        if (flag < 0) return flag;

        // Reconstruct 16-bit data
        data[0] = rbuf[0] & 0xFF;
        temp = rbuf[1] & 0xFF;
        temp = temp << 8;
        data[0] = data[0] + temp;
    } else {
        flag = uread(devnum, rbuf, 4);
        if (flag < 0) return flag;

        // Reconstruct 32-bit data
        data[0] = rbuf[0] & 0xFF;
        temp = rbuf[1] & 0xFF;
        temp = temp << 8;
        data[0] = data[0] + temp;
        
        temp = rbuf[2] & 0xFF;
        temp = temp << 16;
        data[0] = data[0] + temp;
        
        temp = rbuf[3] & 0xFF;
        temp = temp << 24;
        data[0] = data[0] + temp;
    }
  
    return 0;
}

// -----------------------------------------------------------------------------
// Internal: Vblockread (Chunked Read)
// -----------------------------------------------------------------------------
#ifdef NKC
int Vblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data)
#else
int NK6UVME::Vblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data)
#endif
{
    char wbuf[16];
    unsigned char vmemode;
    int flag;
    unsigned long chunk, remain;
    unsigned long i;
  
    // Calculate chunks (64KB per chunk)
    chunk = counts / 0x10000;
    remain = counts % 0x10000;

    vmemode = am | 0x0080; // Read mode

    wbuf[0] = 0x00;
    wbuf[1] = 0x00;
    wbuf[2] = vmemode & 0xFF;
    wbuf[3] = tout & 0xFF;
    wbuf[4] = address & 0xFF;
    wbuf[5] = (address >> 8) & 0xFF;
    wbuf[6] = (address >> 16) & 0xFF;
    wbuf[7] = (address >> 24) & 0xFF;

    wbuf[8] = counts & 0xFF;
    wbuf[9] = (counts >> 8) & 0xFF;
    wbuf[10] = (counts >> 16) & 0xFF;
    wbuf[11] = (counts >> 24) & 0xFF;

    // Send Request
    flag = uwrite(devnum, wbuf, 12);
    if (flag < 0) return flag;

    // Read full chunks
    for (i = 0; i < chunk; i++) {
        flag = uread(devnum, data + (0x10000 * i), 0x10000);
        if (flag < 0) return flag;
    }

    // Read remainder
    if (remain > 0) {
        flag = uread(devnum, data + (0x10000 * chunk), remain);
        if (flag < 0) return flag;
    }

    return 0;
}