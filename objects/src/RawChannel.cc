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
}

void RawChannel::ReserveBulk(int numEvents, int dataPoints) {
    _numEvents = numEvents;
    _dataPoints = dataPoints;
    
    int reqBulk = numEvents * dataPoints * 2; 
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
    return ((_bulkData[offset + 1] << 8) | _bulkData[offset]) & 0x0FFF;
}