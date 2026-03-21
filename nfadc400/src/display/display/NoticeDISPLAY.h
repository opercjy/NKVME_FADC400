#ifndef DISPLAY_H
#define DISPLAY_H

#include <libusb.h>
#ifdef __cplusplus
extern "C" {
#endif

extern void DISPwriteData(unsigned long mid, unsigned long data);
extern unsigned long DISPreadData(unsigned long mid);
extern void DISPsetTimeout(unsigned long mid, unsigned long data);
extern unsigned long DISPreadTimeout(unsigned long mid);

#ifdef __cplusplus
}
#endif

#endif
