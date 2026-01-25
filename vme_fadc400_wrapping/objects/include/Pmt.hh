#ifndef Pmt_hh
#define Pmt_hh
#include "TObject.h"
class Pmt : public TObject {
public:
  Pmt();
  Pmt(int id, double signal);
  virtual ~Pmt();
  void SetId(int id) { _id = id; }
  void SetSignal(double sig) { _signal = sig; }
  int GetId() const { return _id; }
  double GetSignal() const { return _signal; }
private:
  int _id;
  double _signal;
  ClassDef(Pmt, 1)
};
#endif