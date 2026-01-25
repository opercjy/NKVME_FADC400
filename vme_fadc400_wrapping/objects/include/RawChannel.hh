#ifndef RawChannel_hh
#define RawChannel_hh
#include "TObject.h"
#include <vector>

class RawChannel : public TObject {
public:
  RawChannel();
  RawChannel(int cid, int ndp);
  virtual ~RawChannel();

  void SetChannelId(int cid) { _cid = cid; }
  void AddSample(unsigned short sample) { _wave.push_back(sample); }

  int ChannelId() const { return _cid; }
  int GetNPoints() const { return _wave.size(); }
  const std::vector<unsigned short>& GetWaveform() const { return _wave; }

private:
  int _cid;
  std::vector<unsigned short> _wave; // 16-bit storage
  ClassDef(RawChannel, 2)
};
#endif