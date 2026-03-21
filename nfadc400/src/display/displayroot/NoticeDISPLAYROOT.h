#ifndef NKDISPLAY_ROOT_H
#define NKDISPLAY_ROOT_H

#include "TObject.h"

struct libusb_context;

class NKDISPLAY : public TObject {
public:

  NKDISPLAY();
  virtual ~NKDISPLAY();

  void DISPwriteData(unsigned long mid, unsigned long data);
  unsigned long DISPreadData(unsigned long mid);
  void DISPsetTimeout(unsigned long mid, unsigned long data);
  unsigned long DISPreadTimeout(unsigned long mid);

  ClassDef(NKDISPLAY, 0) // NKDISPLAY wrapper class for root
};

#endif
