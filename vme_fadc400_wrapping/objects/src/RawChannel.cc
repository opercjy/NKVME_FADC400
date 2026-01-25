#include "RawChannel.hh"

ClassImp(RawChannel)

RawChannel::RawChannel()
: TObject(), _cid(-1)
{
  _wave.clear();
}

RawChannel::RawChannel(int cid, int ndp)
: TObject(), _cid(cid)
{
  // Reserve memory to optimize performance
  // ndp = Number of Data Points
  _wave.reserve(ndp);
}

RawChannel::~RawChannel()
{
  _wave.clear();
}