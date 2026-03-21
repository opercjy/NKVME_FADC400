// Test program of NFADC400 
// Usgage :  ROOT> .L nfadc400_test.C
//           ROOT> run_nfadc400(1000) //for 1000 events data case

#include "nfadc400_test.h"

int run_nfadc400(int Nevent = 1000) {

  // check setting variable
  cout << "Total evt number for data taking= "<< Nevent <<endl;
  cout << "FADC channel number = "<< NCH <<endl;
  cout << "Total data point = "<<DATA_POINT <<endl;
  cout << "FADC array size = "<<NDATA <<endl;

  //calculation of recording length from DATA_POINT
  const float bintons = 2.5; //2.5 for 400Mhz
  
  int record_length=DATA_POINT/0x80;
  int event_number =0x80000/DATA_POINT;
  
  if(event_number > 512) event_number = 512;  //Refer Manual

  int record_time = int(DATA_POINT * bintons); 
  cout << "Total recording time in ns  = "<<record_time <<endl;
 
  // get NKHOME enviernment
//  TString mypath = gSystem->Getenv("NKHOME");
//  cout<<"NKHOME pass : "<<mypath<<endl;
//  TString myvme  = mypath + TString("/lib/libNotice6UVMEROOT.so");  
//  TString myfadc = mypath + TString("/lib/libNoticeNFADC400ROOT.so");
  
  // Loading VME & FADC lib.
//  gSystem->Load(myvme);
//  gSystem->Load(myfadc);
  R__LOAD_LIBRARY(libNotice6UVMEROOT.so);		// load usb3 library
  R__LOAD_LIBRARY(libNoticeNFADC400ROOT.so);// load nkfadc500 library
   
  //local variable 
  Int_t i;  
  ULong_t NC = NCH;
  ULong_t ND = NDATA;
  Char_t tag[6];
  ULong_t trigger;
  unsigned short  data[NDATA];

  int mid = 0x0;    // Module ID --> Set by DIP switch on the board

  // Class
  NK6UVME kvme;
  NKNFADC400 kadc;
  
  //VME open  
  kvme.VMEopen();
  
  //tree class
  fadc fadc1;
  head head1;

  TCanvas *c1 = new TCanvas("c1", "NFADC400", 800, 800);
  c1->Divide(1,2);
  
  TFile *hfile=new TFile("nfadc400.root","RECREATE");
  TTree *fadc400=new TTree("nfadc400","nfadc400 file");
  TTree *head=new TTree("head","Head of Run");
  TString bfadc = TString("trigger[2]/b:tag[6]/b:FADC[") + ND + TString("]/F");  
  cout<<"bfadc = "<<bfadc<<endl;  
  fadc400->Branch("fadc",&fadc1,bfadc);
  TString bhead = TString("rl/I:offset[") + NC + TString("]/I:gain[") + NC + TString("]/I:ped[") + NC 
    + TString("]/I:delay[") + NC + TString("]/I:tlt/I:tow/I:dce/I:pol[") + NC + TString("]/I:thr[")+ NC 
    + TString("]/I:dt[") + NC + TString("]/I:cw[") + NC + TString("]/I:tm[") + NC 
    + TString("]/I:pct[") + NC + TString("]/I:pci[") + NC + TString("]/I:pwt[") + NC + TString("]/F");
  cout<<"head = "<<bhead<<endl;  
  head->Branch("head1",&head1,bhead);
  TH1F *hoscd1 = new TH1F("hoscd1", "Channel1", DATA_POINT, 0, DATA_POINT*bintons);
  TH1F *hoscd2 = new TH1F("hoscd2", "Channel2", DATA_POINT, 0, DATA_POINT*bintons);

  // open NFADC400
  kadc.NFADC400open(mid);
  
  // check FADC status
  printf("Status = %lX\n", kadc.NFADC400read_STAT(mid));

  // set reset mode, arguments = (device number, mid, timer reset, event number reset, register reset)
  kadc.NFADC400write_RM(mid, 0, 1, 0);

  // reset FADC
  kadc.NFADC400reset(mid);

  // set recording length
  kadc.NFADC400write_RL(mid, record_length);    
  head1.rl =  kadc.NFADC400read_RL(mid); 
      
  // set offset to 3200(range = 0~4095)
  kadc.NFADC400write_DACOFF(mid, 0, 3200);
  for (i = 0; i < NCH; i++)
    head1.off[i] =  kadc.NFADC400read_DACOFF(mid, i + 1); 

  // set gain to 1000(range = 0~4095)
  kadc.NFADC400write_DACGAIN(mid, 0, 1000);
  for (i = 0; i < NCH; i++)
    head1.gain[i] =  kadc.NFADC400read_DACGAIN(mid, i + 1); 

  // measure and show pedestal
  kadc.NFADC400measure_PED(mid, 0);
  for (i = 1; i <= 4; i++)
    printf("pedestal @ch%d = %ld\n", i, kadc.NFADC400read_PED(mid, i));
  for (i = 0; i < NCH; i++)
    head1.ped[i] =  kadc.NFADC400read_PED(mid, i + 1); 

  // set input delay to 12000ns
  kadc.NFADC400write_DLY(mid, 0, 12000);  
  for (i = 0; i < NCH; i++)
    head1.dly[i] =  kadc.NFADC400read_DLY(mid, i + 1); 

  // select Trigger Lookup Table to be used
  // 0xFFFE for all ORed, 0x8000 for all ANDed
  kadc.NFADC400write_TLT(mid, 0xFFFE);
  head1.tlt =  kadc.NFADC400read_TLT(mid); 

  // set Trigger output(@ TRIG OUT connector) width to 1000ns
  kadc.NFADC400write_TOW(mid, 1000);
  head1.tow =  kadc.NFADC400read_TOW(mid); 

  // enable Trigger Daisy chain (use NFADC400disable_DCE if daisy chain is not used)
  kadc.NFADC400enable_DCE(mid);
  head1.dce =  kadc.NFADC400read_DCE(mid); 

  // set pulse polarity to negative(0 for negative, 1 for positive)
  kadc.NFADC400write_POL(mid, 0, 0);  
  for (i = 0; i < NCH; i++)
    head1.pol[i] =  kadc.NFADC400read_POL(mid, i + 0); 
            
  // set discriminator threshold to 10
  kadc.NFADC400write_THR(mid, 0, 50);   
  for (i = 0; i < NCH; i++)
    head1.thr[i] =  kadc.NFADC400read_THR(mid, i + 1); 

  // set trigger deadtime to 0(in nano seconds)
  kadc.NFADC400write_DT(mid, 0, 0);
  for (i = 0; i < NCH; i++)
    head1.dt[i] =  kadc.NFADC400read_DT(mid, i + 1); 

  // set coincidence width to 1000ns
  kadc.NFADC400write_CW(mid, 0, 1000);
  for (i = 0; i < NCH; i++)
    head1.cw[i] =  kadc.NFADC400read_CW(mid, i + 1); 

  // Trigger mode setting(device, mid, ch, enable pulse width, enable pulse count)
  kadc.NFADC400write_TM(mid, 0, 1, 1);
  for (i = 0; i < NCH; i++)
    head1.tm[i] =  kadc.NFADC400read_TM(mid, i + 1); 

  // set pulse count theshold to 1
  kadc.NFADC400write_PCT(mid, 0, 1);
  for (i = 0; i < NCH; i++)
    head1.pct[i] =  kadc.NFADC400read_PCT(mid, i + 1); 

  // set pulse count interval in nano seconds(if PCT=1, this setting is meaningless)
  kadc.NFADC400write_PCI(mid, 0, 1000);
  for (i = 0; i < NCH; i++)
    head1.pci[i] =  kadc.NFADC400read_PCI(mid, i + 1); 
  
  // set pulse width threshold to 200ns
  kadc.NFADC400write_PWT(mid, 0, 200);
  for (i = 0; i < NCH; i++)
    head1.pwt[i] =  kadc.NFADC400read_PWT(mid, i + 1); 

  head->Fill();

  // Start ADC DAQ ( Start buffer page 0 and 1 )
  kadc.NFADC400startL(mid); 
  kadc.NFADC400startH(mid);

  Int_t bufnum =0;
  Int_t evt = 0;
  Int_t dpoint =0;
  Int_t flag = 1;
  
  while(flag) {
      
    Int_t IsFill=1;

    while(1) {

      if(bufnum == 0) 
      	IsFill = !(kadc.NFADC400read_RunL(mid));
      else if(bufnum ==1)
      	IsFill = !(kadc.NFADC400read_RunH(mid)); // Read event number, Flag =0 :all fill

      if(IsFill) break;
   }

    for(Int_t ii=0;ii<event_number;ii++){ 


      // reset histogram
      if (ii == 0) {
	hoscd1->Reset(); 
	hoscd2->Reset();
      }
      
      kadc.NFADC400read_TTIME(mid, 1, bufnum * event_number + ii, tag);
      trigger = kadc.NFADC400read_TPattern(mid, 1, bufnum * event_number + ii);
	
      //tag index   061121
      fadc1.trigger[0] = (trigger >> 8) & 0xFF; //trigger pattern
      fadc1.trigger[1] = trigger & 0xFF;
      
      for(Int_t kk =0;kk<6;kk++) 
	fadc1.tag[kk]=tag[kk] & 0xFF;

      for(Int_t kk =0;kk<NCH;kk++) {
	dpoint = kk*DATA_POINT;
	  
	kadc.NFADC400read_BUFFER(mid, kk + 1, record_length, event_number*bufnum+ii ,data+dpoint);  //data is unsigned short

	for(int jj=0;jj<DATA_POINT;jj++){
	  fadc1.FADC[jj+dpoint]= float(data[jj+dpoint] & 0x3FF);  //10bit

	  //only plot first events 
	  if(kk==0&&ii==0) hoscd1->Fill((jj+0.5)*bintons, fadc1.FADC[jj]);
	  if(kk==1&&ii==0) hoscd2->Fill((jj+0.5)*bintons, fadc1.FADC[jj+dpoint]);
	}
      }

//      fadc400->Fill();
      evt++;
      if(evt >= Nevent) {
	flag = 0;
	break;
      } 
    }

    c1->cd(1);
    hoscd1->Draw("hist");
    c1->cd(2);
    hoscd2->Draw("hist");
    c1->Modified();
    c1->Update();
    printf("evt number = %d\n",evt);
    gSystem->Exec("date");
    //getchar();

    if (flag) {
      if(bufnum == 0 ) { 
	bufnum = 1;
	kadc.NFADC400startL(mid); 
      }
      else if(bufnum == 1) { 
	bufnum =0; 
	kadc.NFADC400startH(mid);
      }
    }

    fadc400->AutoSave();
    FILE *fp = fopen("KILLME","r");
    if(fp) {
      fclose(fp);
      printf(" KILLME file exist  BYE \n");
      gSystem->Exec("rm -f KILLME");
      break;
    }
  }

  kvme.VMEclose();

  printf("finished!!\n");
  gSystem->Exec("date");
  hfile->Write();
 
  return 0;
}



