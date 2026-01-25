/*
 * NFADC400 Firmware Update Tool
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NoticeNFADC400.h" // C Library Header
#include "updateNFADC400.h"

int main(int argc, char **argv)
{
    int devnum = 0;
    unsigned long mid;
    int mode;
    char *filename;
    int i;

    if (argc != 4) {
        printf("Usage: nfadc400_update [mid] [mode] [filename]\n");
        printf("  mode 0 : FPGA update\n");
        printf("  mode 1 : CPLD update\n");
        printf("  mode 2 : ALUT update (ADC Lookup Table)\n");
        return 1;
    }

    mid = strtoul(argv[1], NULL, 0);
    mode = atoi(argv[2]);
    filename = argv[3];

    // Open VME
    if (VMEopen(devnum) < 0) {
        printf("Error: Cannot open VME controller %d\n", devnum);
        return 1;
    }

    // Initialize NFADC400
    NFADC400open(devnum, mid);
    
    // Check Status
    unsigned long stat = NFADC400read_STAT(devnum, mid);
    printf("NFADC400 [mid=0x%lx] Status = 0x%lx\n", mid, stat);

    if (mode == 0) {
        printf(">>> Updating FPGA with %s ...\n", filename);
        NFADC400update_FPGA(devnum, mid, filename);
        printf(">>> FPGA Update Done.\n");
    } 
    else if (mode == 1) {
        printf(">>> Updating CPLD with %s ...\n", filename);
        NFADC400update_CPLD(devnum, mid, filename);
        printf(">>> CPLD Update Done.\n");
    }
    else if (mode == 2) {
        printf(">>> Updating ALUT with %s ...\n", filename);
        // ALUT update for 4 channels
        for (i = 1; i <= 4; i++) {
            printf("    Processing Channel %d...\n", i);
            NFADC400update_ALUT(devnum, mid, i, filename);
        }
        printf(">>> ALUT Update Done.\n");
    }
    else {
        printf("Unknown mode: %d\n", mode);
    }

    VMEclose(devnum);
    return 0;
}