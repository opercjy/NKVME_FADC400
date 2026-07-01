#ifndef Pmt_hh
#define Pmt_hh

#include "TObject.h"
#include <cstdint>

class Pmt : public TObject {
public:
    Pmt();
    Pmt(int id, int ndp);
    virtual ~Pmt();

    virtual void Clear(Option_t* opt = "") override;

    void SetId(int id) { _id = id; }
    void AllocateWaveform(int ndp); // 💡 [핵심] TClonesArray 재사용 시 동적 배열 안전 할당
    void SetWaveform(float * wave);

    void SetPedMean(double val) { _pedmean = val; }
    void SetPedRms (double val) { _pedrms  = val; }
    void SetQtot   (double val) { _qtot    = val; }
    void SetQmax   (double val) { _qmax    = val; }
    void SetFmax   (double val) { _fmax    = val; }
    void SetHwTime(uint64_t val)     { _hwTime = val; }
    void SetTrgPattern(uint16_t val) { _trgPattern = val; }
  
    int    Id()       const { return _id     ; }
    int    Ndp()      const { return _ndp    ; }
    double PedMean()  const { return _pedmean; }
    double PedRms()   const { return _pedrms ; }
    double Qtot()     const { return _qtot   ; }
    double Qmax()     const { return _qmax   ; }
    double Fmax()     const { return _fmax   ; }
    uint64_t HwTime() const     { return _hwTime; }
    uint16_t TrgPattern() const { return _trgPattern; }

    float * Waveform() const { return _wave; }

private:
    int _id ;
    int _ndp;

    float * _wave; //[_ndp]  // 💡 ROOT 트리 직렬화를 위한 가변 배열 마커

    double _pedmean;
    double _pedrms ;
    double _qtot   ;
    double _qmax   ;
    double _fmax   ;

    uint64_t _hwTime;       // 하드웨어 절대 시간 변수
    uint16_t _trgPattern;   // 동시계수 확인용 트리거 패턴 변수

    ClassDefOverride(Pmt, 2);
};
#endif
