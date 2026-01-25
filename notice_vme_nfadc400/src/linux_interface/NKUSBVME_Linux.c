/*
 * Low-level Interface for NKUSBVME Driver
 * Refactored for Rocky Linux 9
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "NKUSBVME_Linux.h"

// File handles for up to 10 devices
static int handle[10] = {0};

int uopen(int devnum)
{
  char devname[64];
  if(devnum < 0 || devnum > 9) return -1;

  // Modern udev creates /dev/nkusbvme0, /dev/nkusbvme1, etc.
  sprintf(devname, "/dev/nkusbvme%d", devnum);

  handle[devnum] = open(devname, O_RDWR);
  
  if(handle[devnum] < 0) {
      perror("NKUSBVME Open Failed");
      return -1;
  }
  return handle[devnum];
}

int uclose(int devnum)
{
  if(handle[devnum] > 0) {
      close(handle[devnum]);
      handle[devnum] = 0;
  }
  return 0;
}

int uwrite(int devnum, char *buffer, unsigned long length)
{
  if(handle[devnum] <= 0) return -1;
  return write(handle[devnum], buffer, length);
}

int uread(int devnum, char *buffer, unsigned long length)
{
  if(handle[devnum] <= 0) return -1;
  return read(handle[devnum], buffer, length);
}