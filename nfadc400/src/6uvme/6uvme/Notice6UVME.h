#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libusb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VME_VENDOR_ID  (0x0547)
#define VME_PRODUCT_ID (0x1095)

#define USB3_SF_READ   (0x82)
#define USB3_SF_WRITE  (0x06)

#define A16D16  0x69
#define A16D32  0x29
#define A24D16  0x79
#define A24D32  0x39
#define A32D16  0x49
#define A32D32  0x09

extern int VMEopen(void);
extern void VMEclose(void);
extern int VMEwrite(unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
extern unsigned long VMEread(unsigned short am, unsigned short tout, unsigned long address);
extern int VMEblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data);

#ifdef __cplusplus
}
#endif



