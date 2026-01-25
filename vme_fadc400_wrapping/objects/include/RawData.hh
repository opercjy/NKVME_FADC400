#ifndef RawData_hh
#define RawData_hh
#include "TObject.h"
#include "TClonesArray.h"
#include "RawChannel.hh"

class RawData : public TObject {
public:
  RawData();
  virtual ~RawData();
  void Clear(Option_t * option = "");
  RawChannel* AddChannel(int id, int ndp);
  int GetNChannels() const;
  RawChannel* GetChannel(int i) const;
private:
  TClonesArray * _channels;
  ClassDef(RawData, 1)
};
#endif