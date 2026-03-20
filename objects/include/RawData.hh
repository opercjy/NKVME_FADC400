#ifndef RawData_hh
#define RawData_hh

#include "TObject.h"
#include "TClonesArray.h"
#include "RawChannel.hh"

class RawData : public TObject {
public:
    RawData();
    virtual ~RawData();

    virtual void Clear(Option_t * option = "") override;

    // 객체 풀링(ConstructedAt)을 통해 새 객체를 힙 메모리 유실 없이 재활용
    RawChannel* AddChannel(int chId, int nPoints);
    
    int GetNChannels() const;
    RawChannel* GetChannel(int i) const;
    TClonesArray* GetArray() const { return _channels; }

private:
    TClonesArray * _channels; //-> ROOT Splitting을 위한 주석 마커

    ClassDefOverride(RawData, 1)
};
#endif
