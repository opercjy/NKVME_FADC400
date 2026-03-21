
#define NCH 2               // number of fadc channel to be used 
#define DATA_POINT 0x4000   // data size : Refer manual 
#define NDATA (DATA_POINT*NCH)

class fadc{
 public:
  UChar_t trigger[2];//trigger pattern
  UChar_t tag[6];//tagging information
  Float_t FADC[NDATA];  // 
};

class head{
 public:
  Int_t rl;
  Int_t off[NCH];
  Int_t gain[NCH];
  Int_t ped[NCH];
  Int_t dly[NCH];
  Int_t tlt;
  Int_t tow;
  Int_t dce;
  Int_t pol[NCH];
  Int_t thr[NCH];
  Int_t dt[NCH];
  Int_t cw[NCH];
  Int_t tm[NCH];
  Int_t pct[NCH];
  Int_t pci[NCH];
  Float_t pwt[NCH];
};

