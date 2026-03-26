#include "RawData.hh"

ClassImp(RawData)

RawData::RawData() : TObject() {
    _channels = new TClonesArray("RawChannel", 100);
    _channels->BypassStreamer(kFALSE); 
}

RawData::~RawData() {
    if (_channels) { _channels->Clear("C"); delete _channels; _channels = nullptr; }
}

void RawData::Clear(Option_t * option) {
    TObject::Clear(option);
    if (_channels) _channels->Clear("C"); 
}

RawChannel* RawData::AddChannel(int chId) { // 💡 파라미터 축소
    int n = _channels->GetEntriesFast();
    RawChannel* ch = (RawChannel*)_channels->ConstructedAt(n);
    ch->SetChId(chId);
    return ch;
}

int RawData::GetNChannels() const { return _channels ? _channels->GetEntriesFast() : 0; }

RawChannel* RawData::GetChannel(int i) const {
    if (!_channels || i < 0 || i >= _channels->GetEntriesFast()) return nullptr;
    return (RawChannel*)_channels->At(i);
}