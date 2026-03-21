#ifndef NK6UVME_ROOT_H
#define NK6UVME_ROOT_H

#include "TObject.h"

struct libusb_context;

class NK6UVME : public TObject {
public:
	
  NK6UVME();
  virtual ~NK6UVME();
  int VMEopen(void);
  void VMEclose(void);
  int VMEwrite(unsigned short am, unsigned short tout, unsigned long address, unsigned long data);
  unsigned long VMEread(unsigned short am, unsigned short tout, unsigned long address);
  int VMEblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data);

  ClassDef(NK6UVME, 0) // NK6UVME wrapper class for root
};

#endif
