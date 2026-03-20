#ifndef Pmt_hh
#define Pmt_hh

#include "TObject.h"

class Pmt : public TObject {
public:
    Pmt();
    Pmt(int id, int ndp);
    virtual ~Pmt();

    // [추가] 객체 재사용 시 메모리 누수를 막기 위한 초기화 함수
    virtual void Clear(Option_t* opt = "") override;

    void SetId(int id) { _id = id; }
    void SetWaveform(float * wave);

    void SetPedMean(double val) { _pedmean = val; }
    void SetPedRms (double val) { _pedrms  = val; }
    void SetQtot   (double val) { _qtot    = val; }
    void SetQmax   (double val) { _qmax    = val; }
    void SetFmax   (double val) { _fmax    = val; }
  
    int    Id()       const { return _id     ; }
    int    Ndp()      const { return _ndp    ; }
    double PedMean()  const { return _pedmean; }
    double PedRms()   const { return _pedrms ; }
    double Qtot()     const { return _qtot   ; }
    double Qmax()     const { return _qmax   ; }
    double Fmax()     const { return _fmax   ; }

    float * Waveform() const { return _wave; }

private:
    int _id ;
    int _ndp;

    // [수정] ROOT Tree 직렬화를 위해 반드시 주석이 //[size] 형태여야 합니다.
    float * _wave; //[_ndp] 

    double _pedmean;
    double _pedrms ;
    double _qtot   ;
    double _qmax   ;
    double _fmax   ;

    ClassDefOverride(Pmt, 1);
};
#endif
