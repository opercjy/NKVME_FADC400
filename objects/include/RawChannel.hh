#ifndef RawChannel_hh
#define RawChannel_hh

#include "TObject.h"
#include <vector>

class RawChannel : public TObject {
public:
    RawChannel();
    RawChannel(int chId, int nPoints = 512);
    virtual ~RawChannel();

    void SetChId(int id) { _chId = id; }
    int GetChId() const { return _chId; }

    // [핵심] 메모리 재할당 방지를 위한 사전 공간 예약
    void Reserve(int nPoints) { _samples.reserve(nPoints); }

    void AddSample(unsigned short val) { _samples.push_back(val); }
    unsigned short GetSample(int i) const { return _samples[i]; }
    const std::vector<unsigned short>& GetSamples() const { return _samples; }
    int GetNPoints() const { return _samples.size(); }

    virtual void Clear(Option_t* opt = "") override;

private:
    int _chId;
    std::vector<unsigned short> _samples;

    ClassDefOverride(RawChannel, 1)
};
#endif
