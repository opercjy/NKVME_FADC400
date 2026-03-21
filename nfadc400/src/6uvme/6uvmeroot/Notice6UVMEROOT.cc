#include "Notice6UVMEROOT.h"
#include "Notice6UVME.h"

ClassImp(NK6UVME)

NK6UVME::NK6UVME() {}

NK6UVME::~NK6UVME() {}

int NK6UVME::VMEopen(void)
{return ::VMEopen();}

void NK6UVME::VMEclose(void)
{::VMEclose();}

int NK6UVME::VMEwrite(unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
{return ::VMEwrite(am, tout, address, data);}

unsigned long NK6UVME::VMEread(unsigned short am, unsigned short tout, unsigned long address)
{return ::VMEread(am, tout, address);}

int NK6UVME::VMEblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data)
{return ::VMEblockread(am, tout, address, count, data);}

