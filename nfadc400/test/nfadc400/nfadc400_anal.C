{
  // an example program of simple analysis program data taken by nfadc400
  // Finding peak, FADC sum and ped, pedrms given range
  // 
  //                            by  H.J.Kim hongjoo@knu.ac.kr Oct.2006 
  //
  //include file 
  gROOT->LoadMacro("nfadc400_test.h");
  // range selection   start-margin< :ped, start-end : peak and FADC sum calc.
  // start by FADC400write_DLY routine
  Int_t start = 4000;
  Int_t margin = 200;  //margin for trigger 
  Int_t end   = start + 2000; // *15.625ns 
  //input file
  TFile *a=new TFile("nfadc400.root");
  TTree *fadc400 = (TTree*)a->Get("nfadc400");
  const Int_t nevent=fadc400->GetEntries();
  printf(" Total event %d \n", nevent);
  TH1F *hoscd1 = new TH1F("hoscd1", "signal", DATA_POINT, 0, DATA_POINT*2.5);
  TH1F *hoscd2 = new TH1F("hoscd2", "pedestal", 1024, 0, 1024);
  TH1F *hoscd3 = new TH1F("hoscd3", "FADC sum", 500, 0, 40000);
  TH1F *hoscd4 = new TH1F("hoscd4", "peak",  1024, 0, 1024);
  ntuple = new TNtuple("ntuple","fadc400 anal",
  "evt:pedmean:pedrms:qtot:ttot:max:maxx");
  TCanvas *c1 = new TCanvas("c1", "KIMS", 1000, 800);
  c1->Divide(2,2);
  fadc fadc1;
  fadc400->SetBranchAddress("fadc",&fadc1);
  for(int j=0;j<nevent;j++){
    fadc400->GetEvent(j);
    hoscd1->Reset(); 
    //    hos1->Reset(); hos2->Reset();
    Float_t ymax = 9999.0;
    Float_t ymaxx = 0;
    Int_t pednum =0 ;
    Float_t pedmean =0.0;
    Float_t pedrms =0.0;
    Float_t qTot =0.0;
    Float_t tTot =0.0;
    for(int i=0;i<DATA_POINT;i++){
       hoscd1->Fill(i*2.5,fadc1.FADC[i]);
       //pedestal 
       if(i<(start-margin)) {
         pednum++;
         pedmean += fadc1.FADC[i];
         pedrms +=  fadc1.FADC[i]*fadc1.FADC[i];       
       }
       else if(i==(start-margin)){ 
         if(pednum>0) {
           pedmean = pedmean/pednum;
           pedrms = pedrms/pednum - pedmean*pedmean;
           if(pedrms > 0) pedrms  = sqrt(pedrms); 
         }
       }
       else if(i>start && i<end){
         qTot += pedmean - fadc1.FADC[i];
         tTot += (pedmean - fadc1.FADC[i])*(i-start);
         if((fadc1.FADC[i]) < ymax) {
           ymax = fadc1.FADC[i];
           ymaxx = i;
         }
       }
    }
    if(qTot !=0) tTot = tTot/qTot;
    ymax = pedmean - ymax;  //ped subtraction
    hoscd2->Fill(pedmean);
    hoscd3->Fill(qTot);
    hoscd4->Fill(ymax);
    ntuple->Fill((float)j,pedmean,pedrms,qTot,tTot,ymax,ymaxx);
    //hist
    //    if(!(j%100)) {
      c1->cd(1);
      hoscd1->Draw();
      c1->cd(2);
      hoscd2->Draw();
      c1->cd(3);
      hoscd3->Draw();
      c1->cd(4);
      hoscd4->Draw();
      c1->Modified();
      c1->Update();
      printf("%dth events passed!\n",j);
    //    }
    //getchar();
  }
  TFile *g = new TFile("fadc400_ntp.root","RECREATE");  
  ntuple->Write();
  hoscd1->Write();
  hoscd2->Write();
  g->Close();
}
