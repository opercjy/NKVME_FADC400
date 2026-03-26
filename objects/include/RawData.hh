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
    RawChannel* AddChannel(int chId); // 💡 인자 단순화
    
    int GetNChannels() const;
    RawChannel* GetChannel(int i) const;
    TClonesArray* GetArray() const { return _channels; }

private:
    TClonesArray * _channels; //->

    ClassDefOverride(RawData, 1)
};
#endif