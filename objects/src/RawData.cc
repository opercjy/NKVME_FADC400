#include "RawData.hh"

ClassImp(RawData)

RawData::RawData() : TObject() {
    _channels = new TClonesArray("RawChannel", 100);
}

RawData::~RawData() {
    if (_channels) {
        _channels->Clear("C");
        delete _channels;
        _channels = nullptr;
    }
}

void RawData::Clear(Option_t * option) {
    TObject::Clear(option);
    if (_channels) _channels->Clear("C"); 
}

RawChannel* RawData::AddChannel(int chId, int nPoints) {
    int n = _channels->GetEntriesFast();
    // [핵심] Placement New 대신 ConstructedAt을 사용하여 내부 std::vector 메모리 누수 완벽 차단!
    RawChannel* ch = (RawChannel*)_channels->ConstructedAt(n);
    ch->SetChId(chId);
    ch->Reserve(nPoints);
    return ch;
}

int RawData::GetNChannels() const {
    return _channels ? _channels->GetEntriesFast() : 0;
}

RawChannel* RawData::GetChannel(int i) const {
    if (!_channels || i < 0 || i >= _channels->GetEntriesFast()) return nullptr;
    return (RawChannel*)_channels->At(i);
}
