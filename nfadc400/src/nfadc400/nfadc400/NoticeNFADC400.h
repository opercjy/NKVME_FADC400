#ifndef NFADC400_H
#define NFADC400_H

#include <libusb.h>
#ifdef __cplusplus
extern "C" {
#endif

extern void NFADC400open(unsigned long mid);
extern void NFADC400read_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data);
extern void NFADC400read_TTIME(unsigned long mid, unsigned long ch, unsigned long nevt, char *data);
extern unsigned long NFADC400read_TPattern(unsigned long mid, unsigned long ch, unsigned long nevt);
extern void NFADC400dump_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
extern void NFADC400dump_TAG(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
extern void NFADC400write_DACOFF(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_DACOFF(unsigned long mid, unsigned long ch);
extern void NFADC400measure_PED(unsigned long mid, unsigned long ch);
extern unsigned long NFADC400read_PED(unsigned long mid, unsigned long ch);
extern void NFADC400write_DACGAIN(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_DACGAIN(unsigned long mid, unsigned long ch);
extern void NFADC400write_DLY(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_DLY(unsigned long mid, unsigned long ch);
extern unsigned long FADC400read_PEDADC(unsigned long mid, unsigned long ch, unsigned long adc);
extern void NFADC400write_POL(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_POL(unsigned long mid, unsigned long ch);
extern void NFADC400write_THR(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_THR(unsigned long mid, unsigned long ch);
extern void NFADC400write_DT(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_DT(unsigned long mid, unsigned long ch);
extern void NFADC400write_CW(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_CW(unsigned long mid, unsigned long ch);
extern void NFADC400write_TM(unsigned long mid, unsigned long ch, int ew, int en);
extern unsigned long NFADC400read_TM(unsigned long mid, unsigned long ch);
extern void NFADC400write_PCT(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_PCT(unsigned long mid, unsigned long ch);
extern void NFADC400write_PCI(unsigned long mid, unsigned long ch, unsigned long data);
extern unsigned long NFADC400read_PCI(unsigned long mid, unsigned long ch);
extern void NFADC400write_PWT(unsigned long mid, unsigned long ch, float data);
extern float NFADC400read_PWT(unsigned long mid, unsigned long ch);
extern void NFADC400reset(unsigned long mid);
extern void NFADC400write_RM(unsigned long mid, int st, int se, int sr);
extern unsigned long NFADC400read_RM(unsigned long mid);
extern void NFADC400startL(unsigned long mid);
extern void NFADC400stopL(unsigned long mid);
extern unsigned long NFADC400read_RunL(unsigned long mid);
extern void NFADC400startH(unsigned long mid);
extern void NFADC400stopH(unsigned long mid);
extern unsigned long NFADC400read_RunH(unsigned long mid);
extern void NFADC400write_RL(unsigned long mid, unsigned long data);
extern unsigned long NFADC400read_RL(unsigned long mid);
extern void NFADC400read_TIMER(unsigned long mid, char *data);
extern unsigned long NFADC400read_ENUM(unsigned long mid);
extern void NFADC400write_TLT(unsigned long mid, unsigned long data);
extern unsigned long NFADC400read_TLT(unsigned long mid);
extern void NFADC400write_TOW(unsigned long mid,  unsigned long data);
extern unsigned long NFADC400read_TOW(unsigned long mid);
extern void NFADC400enable_DCE(unsigned long mid);
extern void NFADC400disable_DCE(unsigned long mid);
extern unsigned long NFADC400read_DCE(unsigned long mid);
extern unsigned long NFADC400read_STAT(unsigned long mid);
extern unsigned long NFADC400check_CPU(unsigned long mid);
extern void waitCPU(unsigned long mid);
extern void NFADC400prog_FPGA(unsigned long mid);
extern unsigned long NFADC400stat_FPGA(unsigned long mid);
extern void NFADC400version_FPGA(unsigned long mid, char *data);
extern void NFADC400prog_CPLD(unsigned long mid);
extern unsigned long NFADC400stat_CPLD(unsigned long mid);
extern void NFADC400version_CPLD(unsigned long mid, char *data);
extern void NFADC400read_SN(unsigned long mid, char *data);
extern void NFADC400enable_FLASH(unsigned long mid);
extern void NFADC400disable_FLASH(unsigned long mid);
extern unsigned long NFADC400check_FLASH(unsigned long mid);
extern void NFADC400write_FLASH(unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data);
extern unsigned long NFADC400read_FLASH(unsigned long mid, unsigned long addr);
extern unsigned long NFADC400stat_FLASH(unsigned long mid);
extern void NFADC400update_FPGA(unsigned long mid, char *filename);
extern void NFADC400update_CPLD(unsigned long mid, char *filename);
extern void NFADC400update_ALUT(unsigned long mid, unsigned long ch, char *filename);


#ifdef __cplusplus
}
#endif

#endif
