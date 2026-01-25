#include "RawData.hh"

ClassImp(RawData)

RawData::RawData()
: TObject()
{
  // Initialize TClonesArray for RawChannel objects
  _channels = new TClonesArray("RawChannel", 100);
}

RawData::~RawData()
{
  if(_channels) {
      _channels->Delete();
      delete _channels;
  }
}

void RawData::Clear(Option_t * option)
{
  _channels->Clear(option);
}

RawChannel* RawData::AddChannel(int id, int ndp)
{
  int n = _channels->GetEntries();
  // Construct new RawChannel(id, ndp) at index n using placement new
  return new ((*_channels)[n]) RawChannel(id, ndp);
}

int RawData::GetNChannels() const
{
  return _channels->GetEntries();
}

RawChannel* RawData::GetChannel(int i) const
{
  if (i < 0 || i >= _channels->GetEntries()) return NULL;
  return (RawChannel*)_channels->At(i);
}