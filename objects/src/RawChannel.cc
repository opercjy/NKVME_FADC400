#include "RawChannel.hh"

ClassImp(RawChannel)

RawChannel::RawChannel() : _chId(-1) {}

RawChannel::RawChannel(int chId, int nPoints) : _chId(chId) {
    _samples.reserve(nPoints);
}

RawChannel::~RawChannel() {
    Clear();
}

void RawChannel::Clear(Option_t* opt) {
    TObject::Clear(opt);
    _chId = -1;
    // capacity(할당된 메모리)는 유지하고 size만 0으로 초기화하여 극도의 속도 확보
    _samples.clear(); 
}
