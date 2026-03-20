#include "Pmt.hh"
#include <cstring>

ClassImp(Pmt)

Pmt::Pmt() : TObject(), 
    _id(0), _ndp(0), _wave(nullptr),
    _pedmean(0), _pedrms(0), _qtot(-999.), _qmax(-999.), _fmax(-999.) 
{
}

Pmt::Pmt(int id, int ndp) : TObject(), 
    _id(id), _ndp(ndp), 
    _pedmean(0), _pedrms(0), _qtot(-999.), _qmax(-999.), _fmax(-999.) 
{
    if(_ndp > 0)
        _wave = new float[_ndp];
    else
        _wave = nullptr;
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
    if (_wave != nullptr && _ndp > 0) {
        memset(_wave, 0, _ndp * sizeof(float));
    }
}

void Pmt::SetWaveform(float * wave) {
    if(_ndp == 0 || _wave == nullptr || wave == nullptr) return;
    
    size_t n = _ndp * sizeof(float);
    memcpy(_wave, wave, n);
}
