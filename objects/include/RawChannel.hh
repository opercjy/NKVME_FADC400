#ifndef RawChannel_hh
#define RawChannel_hh

#include "TObject.h"

class RawChannel : public TObject {
public:
    RawChannel();
    RawChannel(int chId);
    virtual ~RawChannel();

    void SetChId(int id) { _chId = id; }
    int GetChId() const { return _chId; }

    void ReserveBulk(int numEvents, int dataPoints);
    unsigned char* GetBulkData() const { return _bulkData; }
    unsigned char* GetTagData() const { return _tagData; } // 💡 [핵심] 하드웨어 락 해제용 TAG 버퍼
    
    int GetNumEvents() const { return _numEvents; }
    int GetDataPoints() const { return _dataPoints; }

    unsigned short GetSample(int evtIdx, int ptIdx) const;

    virtual void Clear(Option_t* opt = "") override;

private:
    int _chId;
    int _numEvents;
    int _dataPoints;
    int _bulkSize;
    unsigned char* _bulkData; //[_bulkSize] 
    int _tagSize;
    unsigned char* _tagData;  //[_tagSize]  

    ClassDefOverride(RawChannel, 3)
};
#endif