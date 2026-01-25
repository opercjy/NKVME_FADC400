#ifndef _NOTICE_6UVME_H_
#define _NOTICE_6UVME_H_

#include "NKUSBVME_Linux.h"

// VME Mode Definitions
#define A16D16  0x69
#define A16D32  0x29
#define A24D16  0x79
#define A24D32  0x39
#define A32D16  0x49
#define A32D32  0x09

// -----------------------------------------------------------------------------
// C++ Class Definition (For ROOT)
// -----------------------------------------------------------------------------
#ifndef NKC
#include "TObject.h"

class NK6UVME : public TObject {
public:
    NK6UVME();
    virtual ~NK6UVME();

    // High-level VME Functions
    int VMEopen(int devnum);
    int VMEclose(int devnum);
    int VMEwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
    unsigned long VMEread(int devnum, unsigned short am, unsigned short tout, unsigned long address);
    int VMEblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data);

    // Low-level Helper Functions
    int Vwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
    int Vread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long *data);
    int Vblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data);

    ClassDef(NK6UVME, 2) // ROOT Dictionary Definition
};
#endif // NKC

// -----------------------------------------------------------------------------
// C API Prototypes (For C Library)
// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

int VMEopen(int devnum);
int VMEclose(int devnum);
int VMEwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
unsigned long VMEread(int devnum, unsigned short am, unsigned short tout, unsigned long address);
int VMEblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data);

// Low-level helpers exposed for C
int Vwrite(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
int Vread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long *data);
int Vblockread(int devnum, unsigned short am, unsigned short tout, unsigned long address, unsigned long counts, char *data);

#ifdef __cplusplus
}
#endif

#endif // _NOTICE_6UVME_H_