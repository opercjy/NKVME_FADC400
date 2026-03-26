#include "Pmt.hh"
#include <cstring>

ClassImp(Pmt)

Pmt::Pmt() : TObject(), 
    _id(0), _ndp(0), _wave(nullptr),
    _pedmean(0), _pedrms(0), _qtot(-999.), _qmax(-999.), _fmax(-999.) 
{
}

Pmt::Pmt(int id, int ndp) : TObject(), 
    _id(id), _ndp(ndp), _wave(nullptr),
    _pedmean(0), _pedrms(0), _qtot(-999.), _qmax(-999.), _fmax(-999.) 
{
    if(_ndp > 0) {
        _wave = new float[_ndp];
        std::memset(_wave, 0, _ndp * sizeof(float));
    }
}

Pmt::~Pmt() {
    if(_wave != nullptr) {
        delete [] _wave;
        _wave = nullptr;
    }
}

void Pmt::Clear(Option_t* opt) {
    TObject::Clear(opt);
    _pedmean = 0;
    _pedrms  = 0;
    _qtot    = -999.;
    _qmax    = -999.;
    _fmax    = -999.;
    
    // [최적화 핵심] 이벤트 단위 변환 루프가 돌아도 _wave 배열을 해제하지 않습니다.
    // 오직 저수준 memset을 통해 메모리를 0으로 초기화만 수행하여 CPU 사이클을 아낍니다.
    if (_wave != nullptr && _ndp > 0) {
        std::memset(_wave, 0, _ndp * sizeof(float));
    }
}

void Pmt::AllocateWaveform(int ndp) {
    // [최적화 핵심] 요구되는 배열 크기(ndp)가 기존과 같다면 재할당을 전면 스킵합니다.
    // DAQ 수집 특성상 단일 런(Run) 내에서 샘플링 포인트는 불변이므로 완벽한 재활용이 이루어집니다.
    if (_ndp == ndp && _wave != nullptr) {
        return; 
    }
    
    if (_wave != nullptr) {
        delete [] _wave;
        _wave = nullptr;
    }
    
    _ndp = ndp;
    
    if (_ndp > 0) {
        _wave = new float[_ndp];
        std::memset(_wave, 0, _ndp * sizeof(float));
    }
}

void Pmt::SetWaveform(float * wave) {
    if(_ndp == 0 || _wave == nullptr || wave == nullptr) return;
    
    // [최적화 핵심] std::copy 대신 C 스타일의 고속 memcpy를 사용하여
    // 블록 단위로 데이터를 복사(Block Transfer)함으로써 직렬화 속도를 올립니다.
    std::memcpy(_wave, wave, _ndp * sizeof(float));
}