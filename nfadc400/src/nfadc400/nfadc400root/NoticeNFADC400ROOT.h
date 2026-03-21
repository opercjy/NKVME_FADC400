#ifndef NKNFADC400_ROOT_H
#define NKNFADC400_ROOT_H

#include "TObject.h"

struct libusb_context;

class NKNFADC400 : public TObject {
public:

  NKNFADC400();
  virtual ~NKNFADC400();

  void NFADC400open(unsigned long mid);
  void NFADC400read_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data);
  void NFADC400read_TTIME(unsigned long mid, unsigned long ch, unsigned long nevt, char *data);
  unsigned long NFADC400read_TPattern(unsigned long mid, unsigned long ch, unsigned long nevt);
  void NFADC400dump_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
  void NFADC400dump_TAG(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data);
  void NFADC400write_DACOFF(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_DACOFF(unsigned long mid, unsigned long ch);
  void NFADC400measure_PED(unsigned long mid, unsigned long ch);
  unsigned long NFADC400read_PED(unsigned long mid, unsigned long ch);
  void NFADC400write_DACGAIN(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_DACGAIN(unsigned long mid, unsigned long ch);
  void NFADC400write_DLY(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_DLY(unsigned long mid, unsigned long ch);
  unsigned long FADC400read_PEDADC(unsigned long mid, unsigned long ch, unsigned long adc);
  void NFADC400write_POL(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_POL(unsigned long mid, unsigned long ch);
  void NFADC400write_THR(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_THR(unsigned long mid, unsigned long ch);
  void NFADC400write_DT(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_DT(unsigned long mid, unsigned long ch);
  void NFADC400write_CW(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_CW(unsigned long mid, unsigned long ch);
  void NFADC400write_TM(unsigned long mid, unsigned long ch, int ew, int en);
  unsigned long NFADC400read_TM(unsigned long mid, unsigned long ch);
  void NFADC400write_PCT(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_PCT(unsigned long mid, unsigned long ch);
  void NFADC400write_PCI(unsigned long mid, unsigned long ch, unsigned long data);
  unsigned long NFADC400read_PCI(unsigned long mid, unsigned long ch);
  void NFADC400write_PWT(unsigned long mid, unsigned long ch, float data);
  float NFADC400read_PWT(unsigned long mid, unsigned long ch);
  void NFADC400reset(unsigned long mid);
  void NFADC400write_RM(unsigned long mid, int st, int se, int sr);
  unsigned long NFADC400read_RM(unsigned long mid);
  void NFADC400startL(unsigned long mid);
  void NFADC400stopL(unsigned long mid);
  unsigned long NFADC400read_RunL(unsigned long mid);
  void NFADC400startH(unsigned long mid);
  void NFADC400stopH(unsigned long mid);
  unsigned long NFADC400read_RunH(unsigned long mid);
  void NFADC400write_RL(unsigned long mid, unsigned long data);
  unsigned long NFADC400read_RL(unsigned long mid);
  void NFADC400read_TIMER(unsigned long mid, char *data);
  unsigned long NFADC400read_ENUM(unsigned long mid);
  void NFADC400write_TLT(unsigned long mid, unsigned long data);
  unsigned long NFADC400read_TLT(unsigned long mid);
  void NFADC400write_TOW(unsigned long mid,  unsigned long data);
  unsigned long NFADC400read_TOW(unsigned long mid);
  void NFADC400enable_DCE(unsigned long mid);
  void NFADC400disable_DCE(unsigned long mid);
  unsigned long NFADC400read_DCE(unsigned long mid);
  unsigned long NFADC400read_STAT(unsigned long mid);

  ClassDef(NKNFADC400, 0) // NKNFADC400 wrapper class for root
};

#endif
