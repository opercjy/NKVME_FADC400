#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "usb3tcb.h"
#include "Notice6UVME.h"
#include "NoticeDISPLAY.h"

void iDISPwriteData(unsigned long mid);
void iDISPreadData(unsigned long mid);
void iDISPsetTimeout(unsigned long mid);
void iDISPwriteRepeat(unsigned long mid);
void iDISPreadRepeat(unsigned long mid);
void iDISPreadBulk(unsigned long mid);
void iDISPtestLED(unsigned long mid);
void iDISPtestTOUT(unsigned long mid);

int main(void)
{
  int command;
  unsigned long mid;

//  VMEopen();
  printf("open = %d\n", VMEopen());

  printf("Enter module ID(in HEX format) : ");
  scanf("%lX", &mid);

  command = 99;
  
  while (command) {
    printf("\n\n\n\n\n");
    printf("  1. write            2. read          \n");
    printf(" 11. set timeout                       \n");
    printf(" 21. write repeat    22. read repeat   \n");
    printf("                     32. bulk read test\n");
    printf(" 41. test LEDs       43. test Timeout  \n");
    printf("  0. quit                              \n");

    printf("Enter command : ");
    scanf("%d", &command);

    switch (command) {
      case 1:
        iDISPwriteData(mid);
        break;
      case 2:
        iDISPreadData(mid);
        break;
      case 11:
        iDISPsetTimeout(mid);
        break;
      case 21:
        iDISPwriteRepeat(mid);
        break;
      case 22:
        iDISPreadRepeat(mid);
        break;
      case 32:
        iDISPreadBulk(mid);
        break;
      case 41:
        iDISPtestLED( mid);
        break;
      case 43:
        iDISPtestTOUT(mid);
        break;
    }
  }

  VMEclose();

  return 0;
}

void iDISPwriteData(unsigned long mid)
{
  unsigned long data;

  printf("enter data(in HEX) : ");
  scanf("%lX", &data);

  DISPwriteData(mid, data); 
}

void iDISPreadData(unsigned long mid)
{
  unsigned long data;

  data = DISPreadData(mid);

  printf("data = %lX\n", data);
}

void iDISPsetTimeout(unsigned long mid)
{
  unsigned long data;

  printf("enter timeout(in usec) : ");
  scanf("%ld", &data);

  data = data & 0x3FF;

  DISPsetTimeout(mid, data); 
}

void iDISPwriteRepeat(unsigned long mid)
{
  unsigned long data;
  unsigned long addr;
  unsigned long count;
  unsigned long i;

  printf("enter start address(in HEX) : ");
  scanf("%lX", &addr);

  printf("enter start data(in HEX) : ");
  scanf("%lX", &data);

  printf("enter repeat counts(in HEX) : ");
  scanf("%lX", &count);

  for (i = 0; i < count; i++) {
    VMEwrite(A32D32, 0, addr, data);
    addr++;
    data++;
  }
}

void iDISPreadRepeat(unsigned long mid)
{
  unsigned long data;
  unsigned long addr;
  unsigned long count;
  unsigned long i;

  printf("enter start address(in HEX) : ");
  scanf("%lX", &addr);

  printf("enter repeat counts(in HEX) : ");
  scanf("%lX", &count);

  for (i = 0; i < count; i++) {
    data = VMEread(A32D32, 0, addr);
    addr++;
  }
}

void iDISPreadBulk(unsigned long mid)
{
  unsigned long bs;
  unsigned long addr;
  unsigned long count;
  unsigned long i;
  char rdat[0x100000];

  printf("enter start address(in HEX) : ");
  scanf("%lX", &addr);

  printf("enter block size(in HEX) : ");
  scanf("%lX", &bs);

  printf("enter repeat counts : ");
  scanf("%ld", &count);

  for (i = 0; i < count; i++) {
    VMEblockread(A32D32, 0, addr, bs, rdat);
    printf("%ld read\n", i+1);
  }
}

void iDISPtestLED(unsigned long mid)
{
  unsigned long addr;
  unsigned long data;
  int cont;
  int i;

  printf("readback test, enter any number(in HEX) : ");
  scanf("%lX", &data);

  DISPwriteData(mid, data);

  data = DISPreadData(mid);

  printf("%lX returned\n", data);

  printf("AM LED test, enter any number to ALL ON : ");
  scanf("%d", &cont);

  VMEwrite(0x3F, 0, 0x0, 0x0);

  printf("AM LED test, enter any number to ALL OFF : ");
  scanf("%d", &cont);

  VMEwrite(0x0, 0, 0x0, 0x0);
  
  printf("A16 mode A1 LED test, enter any number : ");
  scanf("%d", &cont);

  addr = 0x2;

  VMEwrite(A32D16, 0, 0x2, 0x0);

  printf("Now test address LED, enter any number to continue : ");
  scanf("%d", &cont);

  for (i = 0; i < 30; i++) {
    addr = addr * 2;
    
    printf("enter any number to continue : ");
    scanf("%d", &cont);

    VMEwrite(A32D32, 0, addr, 0x0);
    printf("addr = %lX\n", addr);
  }

  printf("Now test data LED, enter any number to continue : ");
  scanf("%d", &cont);

  data = 1;

  VMEwrite(A32D32, 0, 0x0, data);
  printf("data = 1\n");

  for (i = 0; i < 31; i++) {
    data = data * 2;

    printf("enter any number to continue : ");
    scanf("%d", &cont);

    VMEwrite(A32D32, 0, 0x0, data);
    printf("data = %lX\n", data);
  }
}
  
void iDISPtestTOUT(unsigned long mid)
{
  unsigned long data;
  int cont;
  int i;

  printf("timeout test\n");

  printf("now it's default\n");

  data = 0;

  DISPsetTimeout(mid, data);

  printf("Enter any number to continue : ");
  scanf("%d", &cont);

  VMEwrite(A32D32, 0, 0xFF000000, 0x0);

  data = 1;

  for (i = 0; i < 10; i++) {
    printf("now timeout is %ldus\n", data);

    DISPsetTimeout(mid, data);

    data = data * 2;

    printf("Enter any number to continue : ");
    scanf("%d", &cont);

    VMEwrite(A32D32, 0, 0xFF000000, 0x0);
  }
}


