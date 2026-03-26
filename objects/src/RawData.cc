#include "RawData.hh"

ClassImp(RawData)

RawData::RawData() : TObject() {
    // 초기 배열 100개를 여유 있게 확보. 이후 TClonesArray가 동적 할당을 관리합니다.
    _channels = new TClonesArray("RawChannel", 100);
    _channels->BypassStreamer(kFALSE); 
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
    
    // [최적화 핵심] "C" 옵션을 주어 내부 객체들의 Clear()만 호출하게 만듭니다.
    // 객체 자체를 소멸(Destruct)시키지 않으므로, 다음 트리거 시 즉시 재활용됩니다. (Zero-Allocation)
    if (_channels) _channels->Clear("C"); 
}

RawChannel* RawData::AddChannel(int chId) {
    int n = _channels->GetEntriesFast();
    
    // [최적화 핵심] Placement New (ConstructedAt 사용)
    // 메모리에 새로 객체를 new 하는 것이 아니라, 
    // 미리 확보된 메모리 주소(n번째 위치) 위에 껍데기만 씌워 초기화 속도를 극한으로 끌어올립니다.
    RawChannel* ch = (RawChannel*)_channels->ConstructedAt(n);
    ch->SetChId(chId);
    
    return ch;
}

int RawData::GetNChannels() const { 
    return _channels ? _channels->GetEntriesFast() : 0; 
}

RawChannel* RawData::GetChannel(int i) const {
    if (!_channels || i < 0 || i >= _channels->GetEntriesFast()) return nullptr;
    return (RawChannel*)_channels->At(i);
}