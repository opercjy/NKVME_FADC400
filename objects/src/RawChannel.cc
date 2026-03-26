#include "RawChannel.hh"
#include <cstring>

ClassImp(RawChannel)

RawChannel::RawChannel() : _chId(-1), _numEvents(0), _dataPoints(0), _bulkSize(0), _bulkData(nullptr), _tagSize(0), _tagData(nullptr) {}

RawChannel::RawChannel(int chId) : _chId(chId), _numEvents(0), _dataPoints(0), _bulkSize(0), _bulkData(nullptr), _tagSize(0), _tagData(nullptr) {}

RawChannel::~RawChannel() {
    if (_bulkData) { delete[] _bulkData; _bulkData = nullptr; }
    if (_tagData) { delete[] _tagData; _tagData = nullptr; }
}

void RawChannel::Clear(Option_t* opt) {
    TObject::Clear(opt);
    _chId = -1;
    _numEvents = 0;
    _dataPoints = 0;
    
    // [최적화 핵심] 버퍼 메모리(_bulkData, _tagData)는 절대로 해제(delete)하지 않습니다.
    // 다음 이벤트에서 VMEblockread가 이 주소 위에 바로 바이너리 데이터를 덮어쓰도록 유도하여 
    // 메모리 단편화(Fragmentation)와 할당 오버헤드를 완벽히 제거했습니다.
}

void RawChannel::ReserveBulk(int numEvents, int dataPoints) {
    _numEvents = numEvents;
    _dataPoints = dataPoints;
    
    int reqBulk = numEvents * dataPoints * 2; 
    
    // [최적화 핵심] 요구되는 메모리량이 현재 캐시된 용량(_bulkSize)보다 클 때만 확장을 수행합니다.
    if (reqBulk > _bulkSize) {
        if (_bulkData) delete[] _bulkData;
        _bulkSize = reqBulk;
        _bulkData = new unsigned char[_bulkSize];
    }
    
    int reqTag = numEvents * 8; // Tag is 8 bytes per event
    
    if (reqTag > _tagSize) {
        if (_tagData) delete[] _tagData;
        _tagSize = reqTag;
        _tagData = new unsigned char[_tagSize];
    }
}

unsigned short RawChannel::GetSample(int evtIdx, int ptIdx) const {
    if (!_bulkData) return 0;
    int offset = (evtIdx * _dataPoints + ptIdx) * 2;
    // Little-endian 디코딩 (Notice VME 버스 통신 규격 적용)
    return ((_bulkData[offset + 1] << 8) | _bulkData[offset]) & 0x0FFF;
}