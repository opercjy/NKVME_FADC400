#ifndef NFADC400_TEST_H
#define NFADC400_TEST_H

// ROOT 자료형 정의 포함
#include "Rtypes.h"

// [설정] 변경 가능
#define NCH 1               // 테스트할 채널 수 (박사님 환경: 1채널)
#define DATA_POINT 512      // 데이터 포인트 (config와 동일하게 512 권장)
#define NDATA (DATA_POINT*NCH)

// 데이터 구조체
class fadc {
 public:
  UChar_t trigger[2]; // trigger pattern
  UChar_t tag[6];     // tagging information
  Float_t FADC[NDATA]; 
};

// 헤더 정보 구조체
class head {
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

#endif