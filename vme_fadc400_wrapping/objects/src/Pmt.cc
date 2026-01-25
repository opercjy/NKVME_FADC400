#include "Pmt.hh"

ClassImp(Pmt)

Pmt::Pmt()
: TObject(), _id(0), _signal(0.0)
{
}

Pmt::Pmt(int id, double signal)
: TObject(), _id(id), _signal(signal)
{
}

Pmt::~Pmt()
{
}