#ifndef _NOTICE_NFADC400_H_
#define _NOTICE_NFADC400_H_

#include "Notice6UVME.h"

#ifndef NKC
// -----------------------------------------------------------------------------
// C++ Class Definition (For ROOT)
// Inherits from NK6UVME
// -----------------------------------------------------------------------------
class NKNFADC400 : public NK6UVME {
public:
    NKNFADC400();
    virtual ~NKNFADC400();

    // Basic Operations
    void NFADC400open(int devnum, unsigned long mid);
    void NFADC400read_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data);
    void NFADC400read_TTIME(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt, char *data);
    unsigned long NFADC400read_TPattern(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt);
    void NFADC400dump_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
    void NFADC400dump_TAG(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);

    // Register Access (DAC, Pedestal, Gain, Delay, etc)
    void NFADC400write_DACOFF(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_DACOFF(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400measure_PED(int devnum, unsigned long mid, unsigned long ch);
    unsigned long NFADC400read_PED(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_DACGAIN(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_DACGAIN(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_DLY(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_DLY(int devnum, unsigned long mid, unsigned long ch);
    unsigned long FADC400read_PEDADC(int devnum, unsigned long mid, unsigned long ch, unsigned long adc);
    
    // Trigger & Thresholds
    void NFADC400write_POL(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_POL(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_THR(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_THR(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_DT(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_DT(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_CW(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_CW(int devnum, unsigned long mid, unsigned long ch);
    
    // Mode & Pulse Settings
    void NFADC400write_TM(int devnum, unsigned long mid, unsigned long ch, int ew, int en);
    unsigned long NFADC400read_TM(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_PCT(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_PCT(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_PCI(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
    unsigned long NFADC400read_PCI(int devnum, unsigned long mid, unsigned long ch);
    void NFADC400write_PWT(int devnum, unsigned long mid, unsigned long ch, float data);
    float NFADC400read_PWT(int devnum, unsigned long mid, unsigned long ch);

    // System Control (Reset, Start/Stop)
    void NFADC400reset(int devnum, unsigned long mid);
    void NFADC400write_RM(int devnum, unsigned long mid, int st, int se, int sr);
    unsigned long NFADC400read_RM(int devnum, unsigned long mid);
    void NFADC400startL(int devnum, unsigned long mid);
    void NFADC400stopL(int devnum, unsigned long mid);
    unsigned long NFADC400read_RunL(int devnum, unsigned long mid);
    void NFADC400startH(int devnum, unsigned long mid);
    void NFADC400stopH(int devnum, unsigned long mid);
    unsigned long NFADC400read_RunH(int devnum, unsigned long mid);
    
    // Global Settings
    void NFADC400write_RL(int devnum, unsigned long mid, unsigned long data);
    unsigned long NFADC400read_RL(int devnum, unsigned long mid);
    void NFADC400read_TIMER(int devnum, unsigned long mid, char *data);
    unsigned long NFADC400read_ENUM(int devnum, unsigned long mid);
    void NFADC400write_TLT(int devnum, unsigned long mid, unsigned long data);
    unsigned long NFADC400read_TLT(int devnum, unsigned long mid);
    void NFADC400write_TOW(int devnum, unsigned long mid,  unsigned long data);
    unsigned long NFADC400read_TOW(int devnum, unsigned long mid);
    void NFADC400enable_DCE(int devnum, unsigned long mid);
    void NFADC400disable_DCE(int devnum, unsigned long mid);
    unsigned long NFADC400read_DCE(int devnum, unsigned long mid);
    unsigned long NFADC400read_STAT(int devnum, unsigned long mid);
    unsigned long NFADC400check_CPU(int devnum, unsigned long mid);
    void waitCPU(int devnum, unsigned long mid);

    // Firmware & Flash Operations
    void NFADC400prog_FPGA(int devnum, unsigned long mid);
    unsigned long NFADC400stat_FPGA(int devnum, unsigned long mid);
    void NFADC400version_FPGA(int devnum, unsigned long mid, char *data);
    void NFADC400prog_CPLD(int devnum, unsigned long mid);
    unsigned long NFADC400stat_CPLD(int devnum, unsigned long mid);
    void NFADC400version_CPLD(int devnum, unsigned long mid, char *data);
    void NFADC400read_SN(int devnum, unsigned long mid, char *data);
    void NFADC400enable_FLASH(int devnum, unsigned long mid);
    void NFADC400disable_FLASH(int devnum, unsigned long mid);
    unsigned long NFADC400check_FLASH(int devnum, unsigned long mid);
    void NFADC400write_FLASH(int devnum, unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data);
    unsigned long NFADC400read_FLASH(int devnum, unsigned long mid, unsigned long addr);
    unsigned long NFADC400stat_FLASH(int devnum, unsigned long mid);
    void NFADC400update_FPGA(int devnum, unsigned long mid, char *filename);
    void NFADC400update_CPLD(int devnum, unsigned long mid, char *filename);
    void NFADC400update_ALUT(int devnum, unsigned long mid, unsigned long ch, char *filename);

    ClassDef(NKNFADC400, 2)
};
#endif // NKC

// -----------------------------------------------------------------------------
// C API Prototypes
// -----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif

void NFADC400open(int devnum, unsigned long mid);
void NFADC400read_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data);
void NFADC400read_TTIME(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt, char *data);
unsigned long NFADC400read_TPattern(int devnum, unsigned long mid, unsigned long ch, unsigned long nevt);
void NFADC400dump_BUFFER(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
void NFADC400dump_TAG(int devnum, unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
void NFADC400write_DACOFF(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_DACOFF(int devnum, unsigned long mid, unsigned long ch);
void NFADC400measure_PED(int devnum, unsigned long mid, unsigned long ch);
unsigned long NFADC400read_PED(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_DACGAIN(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_DACGAIN(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_DLY(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_DLY(int devnum, unsigned long mid, unsigned long ch);
unsigned long FADC400read_PEDADC(int devnum, unsigned long mid, unsigned long ch, unsigned long adc);
void NFADC400write_POL(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_POL(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_THR(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_THR(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_DT(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_DT(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_CW(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_CW(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_TM(int devnum, unsigned long mid, unsigned long ch, int ew, int en);
unsigned long NFADC400read_TM(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_PCT(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_PCT(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_PCI(int devnum, unsigned long mid, unsigned long ch, unsigned long data);
unsigned long NFADC400read_PCI(int devnum, unsigned long mid, unsigned long ch);
void NFADC400write_PWT(int devnum, unsigned long mid, unsigned long ch, float data);
float NFADC400read_PWT(int devnum, unsigned long mid, unsigned long ch);
void NFADC400reset(int devnum, unsigned long mid);
void NFADC400write_RM(int devnum, unsigned long mid, int st, int se, int sr);
unsigned long NFADC400read_RM(int devnum, unsigned long mid);
void NFADC400startL(int devnum, unsigned long mid);
void NFADC400stopL(int devnum, unsigned long mid);
unsigned long NFADC400read_RunL(int devnum, unsigned long mid);
void NFADC400startH(int devnum, unsigned long mid);
void NFADC400stopH(int devnum, unsigned long mid);
unsigned long NFADC400read_RunH(int devnum, unsigned long mid);
void NFADC400write_RL(int devnum, unsigned long mid, unsigned long data);
unsigned long NFADC400read_RL(int devnum, unsigned long mid);
void NFADC400read_TIMER(int devnum, unsigned long mid, char *data);
unsigned long NFADC400read_ENUM(int devnum, unsigned long mid);
void NFADC400write_TLT(int devnum, unsigned long mid, unsigned long data);
unsigned long NFADC400read_TLT(int devnum, unsigned long mid);
void NFADC400write_TOW(int devnum, unsigned long mid,  unsigned long data);
unsigned long NFADC400read_TOW(int devnum, unsigned long mid);
void NFADC400enable_DCE(int devnum, unsigned long mid);
void NFADC400disable_DCE(int devnum, unsigned long mid);
unsigned long NFADC400read_DCE(int devnum, unsigned long mid);
unsigned long NFADC400read_STAT(int devnum, unsigned long mid);
unsigned long NFADC400check_CPU(int devnum, unsigned long mid);
void waitCPU(int devnum, unsigned long mid);
void NFADC400prog_FPGA(int devnum, unsigned long mid);
unsigned long NFADC400stat_FPGA(int devnum, unsigned long mid);
void NFADC400version_FPGA(int devnum, unsigned long mid, char *data);
void NFADC400prog_CPLD(int devnum, unsigned long mid);
unsigned long NFADC400stat_CPLD(int devnum, unsigned long mid);
void NFADC400version_CPLD(int devnum, unsigned long mid, char *data);
void NFADC400read_SN(int devnum, unsigned long mid, char *data);
void NFADC400enable_FLASH(int devnum, unsigned long mid);
void NFADC400disable_FLASH(int devnum, unsigned long mid);
unsigned long NFADC400check_FLASH(int devnum, unsigned long mid);
void NFADC400write_FLASH(int devnum, unsigned long mid, unsigned char opcode, unsigned long addr, unsigned char data);
unsigned long NFADC400read_FLASH(int devnum, unsigned long mid, unsigned long addr);
unsigned long NFADC400stat_FLASH(int devnum, unsigned long mid);
void NFADC400update_FPGA(int devnum, unsigned long mid, char *filename);
void NFADC400update_CPLD(int devnum, unsigned long mid, char *filename);
void NFADC400update_ALUT(int devnum, unsigned long mid, unsigned long ch, char *filename);

#ifdef __cplusplus
}
#endif

#endif // _NOTICE_NFADC400_H_