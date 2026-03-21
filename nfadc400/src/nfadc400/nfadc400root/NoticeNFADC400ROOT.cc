#include "NoticeNFADC400ROOT.h"
#include "NoticeNFADC400.h"

ClassImp(NKNFADC400)

NKNFADC400::NKNFADC400()
{
}

NKNFADC400::~NKNFADC400()
{
}

  void NKNFADC400::NFADC400open(unsigned long mid)
  {::NFADC400open(mid);}

  void NKNFADC400::NFADC400read_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long nevt, unsigned short *data)
  {::NFADC400read_BUFFER(mid, ch, range, nevt, data);}

  void NKNFADC400::NFADC400read_TTIME(unsigned long mid, unsigned long ch, unsigned long nevt, char *data)
  {::NFADC400read_TTIME(mid, ch, nevt, data);}

  unsigned long NKNFADC400::NFADC400read_TPattern(unsigned long mid, unsigned long ch, unsigned long nevt)
  {return ::NFADC400read_TPattern(mid, ch, nevt);}

  void NKNFADC400::NFADC400dump_BUFFER(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
  {::NFADC400dump_BUFFER(mid, ch, range, page, data);}

  void NKNFADC400::NFADC400dump_TAG(unsigned long mid, unsigned long ch, unsigned long range, unsigned long page, char *data)
  {::NFADC400dump_TAG(mid, ch, range, page, data);}

  void NKNFADC400::NFADC400write_DACOFF(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_DACOFF(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_DACOFF(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_DACOFF(mid, ch);}

  void NKNFADC400::NFADC400measure_PED(unsigned long mid, unsigned long ch)
  {::NFADC400measure_PED(mid, ch);}

  unsigned long NKNFADC400::NFADC400read_PED(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_PED(mid, ch);}

  void NKNFADC400::NFADC400write_DACGAIN(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_DACGAIN(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_DACGAIN(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_DACGAIN(mid, ch);}
 
  void NKNFADC400::NFADC400write_DLY(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_DLY(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_DLY(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_DLY(mid, ch);}

  unsigned long NKNFADC400::FADC400read_PEDADC(unsigned long mid, unsigned long ch, unsigned long adc)
  {return ::FADC400read_PEDADC(mid, ch, adc);}

  void NKNFADC400::NFADC400write_POL(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_POL(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_POL(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_POL(mid, ch);}

  void NKNFADC400::NFADC400write_THR(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_THR(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_THR(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_THR(mid, ch);}

  void NKNFADC400::NFADC400write_DT(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_DT(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_DT(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_DT(mid, ch);}

  void NKNFADC400::NFADC400write_CW(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_CW(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_CW(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_CW(mid, ch);}

  void NKNFADC400::NFADC400write_TM(unsigned long mid, unsigned long ch, int ew, int en)
  {::NFADC400write_TM(mid, ch, ew, en);}

  unsigned long NKNFADC400::NFADC400read_TM(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_TM(mid, ch);}

  void NKNFADC400::NFADC400write_PCT(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_PCT(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_PCT(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_PCT(mid, ch);}

  void NKNFADC400::NFADC400write_PCI(unsigned long mid, unsigned long ch, unsigned long data)
  {::NFADC400write_PCI(mid, ch, data);}

  unsigned long NKNFADC400::NFADC400read_PCI(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_PCI(mid, ch);}

  void NKNFADC400::NFADC400write_PWT(unsigned long mid, unsigned long ch, float data)
  {::NFADC400write_PWT(mid, ch, data);}

  float NKNFADC400::NFADC400read_PWT(unsigned long mid, unsigned long ch)
  {return ::NFADC400read_PWT(mid, ch);}

  void NKNFADC400::NFADC400reset(unsigned long mid)
  {::NFADC400reset(mid);}

  void NKNFADC400::NFADC400write_RM(unsigned long mid, int st, int se, int sr)
  {::NFADC400write_RM(mid, st, se, sr);}

  unsigned long NKNFADC400::NFADC400read_RM(unsigned long mid)
  {return ::NFADC400read_RM(mid);}

  void NKNFADC400::NFADC400startL(unsigned long mid)
  {::NFADC400startL(mid);}

  void NKNFADC400::NFADC400stopL(unsigned long mid)
  {::NFADC400stopL(mid);}

  unsigned long NKNFADC400::NFADC400read_RunL(unsigned long mid)
  {return ::NFADC400read_RunL(mid);}

  void NKNFADC400::NFADC400startH(unsigned long mid)
  {::NFADC400startH(mid);}

  void NKNFADC400::NFADC400stopH(unsigned long mid)
  {::NFADC400stopH(mid);}

  unsigned long NKNFADC400::NFADC400read_RunH(unsigned long mid)
  {return ::NFADC400read_RunH(mid);}

  void NKNFADC400::NFADC400write_RL(unsigned long mid, unsigned long data)
  {::NFADC400write_RL(mid, data);}

  unsigned long NKNFADC400::NFADC400read_RL(unsigned long mid)
  {return ::NFADC400read_RL(mid);}

  void NKNFADC400::NFADC400read_TIMER(unsigned long mid, char *data)
  {::NFADC400read_TIMER(mid, data);}

  unsigned long NKNFADC400::NFADC400read_ENUM(unsigned long mid)
  {return ::NFADC400read_ENUM(mid);}

  void NKNFADC400::NFADC400write_TLT(unsigned long mid, unsigned long data)
  {::NFADC400write_TLT(mid, data);}

  unsigned long NKNFADC400::NFADC400read_TLT(unsigned long mid)
  {return ::NFADC400read_TLT(mid);}

  void NKNFADC400::NFADC400write_TOW(unsigned long mid,  unsigned long data)
  {::NFADC400write_TOW(mid,  data);}

  unsigned long NKNFADC400::NFADC400read_TOW(unsigned long mid)
  {return ::NFADC400read_TOW(mid);}

  void NKNFADC400::NFADC400enable_DCE(unsigned long mid)
  {::NFADC400enable_DCE(mid);}

  void NKNFADC400::NFADC400disable_DCE(unsigned long mid)
  {::NFADC400disable_DCE(mid);}

  unsigned long NKNFADC400::NFADC400read_DCE(unsigned long mid)
  {return ::NFADC400read_DCE(mid);}

  unsigned long NKNFADC400::NFADC400read_STAT(unsigned long mid)
  {return ::NFADC400read_STAT(mid);}



