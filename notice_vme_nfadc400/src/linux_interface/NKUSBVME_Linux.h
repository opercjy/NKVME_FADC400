#ifndef _NKUSBVME_LINUX_H_
#define _NKUSBVME_LINUX_H_

#ifdef __cplusplus
extern "C" {
#endif

int uopen(int devnum);
int uclose(int devnum);
int uwrite(int devnum, char *buffer, unsigned long length);
int uread(int devnum, char *buffer, unsigned long length);

#ifdef __cplusplus
}
#endif

#endif