// nfadc400_test.cc
// Legacy Test Code ported for Standalone Compilation

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// ROOT Headers
#include "TROOT.h"
#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TH1F.h"
#include "TCanvas.h"
#include "TApplication.h"

// Notice Headers
#include "NoticeNFADC400.h"
#include "Notice6UVME.h"

// Local Header
#include "nfadc400_test.h"

using namespace std;

int main(int argc, char **argv) {
  // 이벤트 수 인자 처리 (기본값 1000)
  int Nevent = 1000;
  if (argc > 1) Nevent = atoi(argv[1]);

  printf("================================================\n");
  printf(" Notice NFADC400 Legacy Hardware Test\n");
  printf("================================================\n");
  printf(" Total evt number   = %d\n", Nevent);
  printf(" FADC channel number= %d\n", NCH);
  printf(" Total data point   = %d\n", DATA_POINT);
  printf("================================================\n");

  // ROOT Application (Canvas 그리기 위해 필요하지만, 여기서는 배치 모드처럼 작동)
  // TApplication app("app", &argc, argv); 

  const float bintons = 1.0; 
  int record_length = DATA_POINT / 0x80;
  int event_number = 0x80000 / DATA_POINT;
  if(event_number > 512) event_number = 512; 

  int mid = 0; // [중요] 박사님 환경: MID = 0 (스위치 모두 내림)
  int nkusb = 0;

  // Class Instance
  NKNFADC400 kadc;
  
  // 1. VME Open
  printf(">>> Opening VME Controller...\n");
  if (kadc.VMEopen(nkusb) < 0) {
      printf(" [ERROR] VME Open Failed. Check USB Connection.\n");
      return -1;
  }
  printf(" -> VME Open Success.\n");

  // 2. FADC Open & Check
  printf(">>> Opening FADC [MID=%d]...\n", mid);
  kadc.NFADC400open(nkusb, mid);
  
  unsigned long status = kadc.NFADC400read_STAT(nkusb, mid);
  printf(" -> FADC Status = 0x%lX ", status);
  if (status == 0xFADC) printf("(OK!)\n");
  else {
      printf("(ERROR! Expected 0xFADC)\n");
      printf(" [CRITICAL] Board not responding correctly. Exiting.\n");
      return -1;
  }

  // --- Data Structures ---
  fadc fadc1;
  head head1;
  
  TFile *hfile = new TFile("legacy_test.root", "RECREATE");
  TTree *fadc400 = new TTree("nfadc400", "nfadc400 file");
  TTree *head = new TTree("head", "Head of Run");

  // Branch 설정
  char branch_desc[1024];
  sprintf(branch_desc, "trigger[2]/b:tag[6]/b:FADC[%d]/F", NDATA);
  fadc400->Branch("fadc", &fadc1, branch_desc);
  
  // Head Branch (복잡해서 간략화 또는 유지)
  head->Branch("head1", &head1, "rl/I"); // 간략히 테스트

  // --- Initialization ---
  printf(">>> Initializing Registers...\n");
  kadc.NFADC400write_RM(nkusb, mid, 0, 1, 0);
  kadc.NFADC400reset(nkusb, mid);
  kadc.NFADC400write_RL(nkusb, mid, record_length);
  
  // Basic Settings (1 Channel Test)
  kadc.NFADC400write_DACOFF(nkusb, mid, 0, 3400); // Baseline
  kadc.NFADC400write_DACGAIN(nkusb, mid, 0, 1000);
  kadc.NFADC400write_DLY(nkusb, mid, 0, 1000);
  kadc.NFADC400write_TLT(nkusb, mid, 0xFFFE); // OR Trigger
  kadc.NFADC400write_TOW(nkusb, mid, 1000);
  kadc.NFADC400enable_DCE(nkusb, mid);
  kadc.NFADC400write_POL(nkusb, mid, 0, 1); // Negative logic
  kadc.NFADC400write_THR(nkusb, mid, 0, 50); // Threshold

  // Trigger Mode
  kadc.NFADC400write_TM(nkusb, mid, 0, 0, 0); // Normal Mode

  // Start DAQ
  printf(">>> Starting DAQ loop for %d events...\n", Nevent);
  kadc.NFADC400startL(nkusb, mid); 
  kadc.NFADC400startH(nkusb, mid);

  int bufnum = 0;
  int evt = 0;
  unsigned short data[NDATA];
  char tag[6];
  
  while(evt < Nevent) {
    int IsFill = 1;
    // Wait for buffer full
    while(1) {
      if(bufnum == 0) IsFill = !(kadc.NFADC400read_RunL(nkusb, mid));
      else            IsFill = !(kadc.NFADC400read_RunH(nkusb, mid));
      if(IsFill) break;
      // usleep(100); // CPU 점유율 낮추기
    }

    // Read Events in Buffer
    for(int ii=0; ii<event_number; ii++) {
       if (evt >= Nevent) break;

       kadc.NFADC400read_TTIME(nkusb, mid, 1, bufnum * event_number + ii, tag);
       unsigned long trg = kadc.NFADC400read_TPattern(nkusb, mid, 1, bufnum * event_number + ii);
       
       // Fill Structure
       fadc1.trigger[0] = (trg >> 8) & 0xFF;
       fadc1.trigger[1] = trg & 0xFF;
       for(int k=0; k<6; k++) fadc1.tag[k] = tag[k];

       // Read Waveform
       for(int ch=0; ch<NCH; ch++) {
           kadc.NFADC400read_BUFFER(nkusb, mid, ch+1, record_length, event_number*bufnum + ii, data + ch*DATA_POINT);
           for(int j=0; j<DATA_POINT; j++) {
               fadc1.FADC[ch*DATA_POINT + j] = (float)(data[ch*DATA_POINT + j] & 0x3FF);
           }
       }
       fadc400->Fill();
       evt++;
       if (evt % 100 == 0) { printf("\r Events: %d / %d", evt, Nevent); fflush(stdout); }
    }

    // Toggle Buffer
    if(bufnum == 0) { 
        bufnum = 1; kadc.NFADC400startL(nkusb, mid); 
    } else { 
        bufnum = 0; kadc.NFADC400startH(nkusb, mid);
    }
  }

  printf("\n>>> Finished!\n");
  hfile->Write();
  hfile->Close();
  return 0;
}