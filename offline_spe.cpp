#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>

#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TClonesArray.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TString.h"
#include "TPad.h"
#include "TF1.h"
#include "TStyle.h"

#include "Pmt.hh"
#include "RunInfo.hh"

void offline_spe(const char* inputFile = "") {
    TString filename(inputFile);

    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] PMT Single Photo-electron (SPE) Calibration\033[0m\n";
        std::cout << "  root -l 'offline_spe.cpp(\"your_data_prod.root\")'\n\n";
        return;
    }

    if (gSystem->Load("libNKVME_FADC400.so") < 0) {
        std::cerr << "\033[1;31m[ERROR]\033[0m Cannot load libNKVME_FADC400.so!\n";
        std::cerr << "        Did you forget to run 'source setup.sh' ?\n";
        return;
    }

    std::unique_ptr<TFile> f(TFile::Open(filename.Data(), "READ"));
    if (!f || f->IsZombie()) return;

    auto rInfo = f->Get<RunInfo>("RunInfo");
    if (rInfo) {
        std::cout << "\033[1;36m=== Run Information ===\033[0m\n";
        std::cout << " Run Number : " << rInfo->GetRunNum() << "\n";
        std::cout << " Run Type   : " << rInfo->GetRunType() << "\n";
        std::cout << " File Name  : " << filename << "\n";
        std::cout << "=======================\n";
    }

    auto tree = f->Get<TTree>("PROD");
    if (!tree) return;

    TTreeReader reader("PROD", f.get());
    TTreeReaderValue<TClonesArray> pmtArray(reader, "Pmt");

    std::map<int, TH1F*> hQtotMap; 
    int eventCount = 0;

    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            if (hQtotMap.find(chId) == hQtotMap.end()) {
                hQtotMap[chId] = new TH1F(Form("hQtot_Ch%d", chId), 
                                          Form("PMT SPE Calibration (Ch%d);Charge (ADC int);Counts", chId), 
                                          500, 0, 2000); 
                hQtotMap[chId]->SetMinimum(0.5); 
                int colors[] = {kAzure+1, kRed-4, kGreen+2, kOrange+1};
                hQtotMap[chId]->SetFillColor(colors[chId % 4]);
            }
            hQtotMap[chId]->Fill(pmt->GetQtot());
        }
        eventCount++;
    }

    std::cout << "\033[1;32m[INFO]\033[0m Successfully analyzed " << eventCount << " events for SPE Calibration.\n";

    int nActiveCh = hQtotMap.size();
    if (nActiveCh == 0) return;

    gStyle->SetOptFit(1111); 
    auto c1 = new TCanvas("c1_spe", "PMT SPE Calibration Viewer", 800, 400 * nActiveCh);
    c1->Divide(1, nActiveCh); 

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        c1->cd(row + 1);
        gPad->SetLogy(); 
        hist->Draw("HIST");

        std::cout << "\n\033[1;35m[Fit]\033[0m Attempting Textbook SPE Fit for Channel " << chId << "...\n";
        
        TF1 *pmtFit = new TF1(Form("pmtFit_Ch%d", chId), "gaus(0) + gaus(3) + gaus(6)", 0, 1500);
        pmtFit->SetLineColor(kMagenta);
        pmtFit->SetLineWidth(2);
        
        pmtFit->SetParNames("Ped_Amp", "Ped_Mean", "Ped_Sigma", 
                            "1PE_Amp", "1PE_Mean", "1PE_Sigma", 
                            "2PE_Amp", "2PE_Mean", "2PE_Sigma");

        double maxBinVal = hist->GetMaximum();
        pmtFit->SetParameters(maxBinVal, 50, 20,       
                              maxBinVal/10, 300, 50,   
                              maxBinVal/50, 600, 70);  

        pmtFit->SetParLimits(1, 0, 150);     
        pmtFit->SetParLimits(4, 150, 800);   
        pmtFit->SetParLimits(7, 300, 1600);  

        hist->Fit(pmtFit, "QR+"); 

        TF1 *fPed = new TF1(Form("fPed_Ch%d", chId), "gaus", 0, 1500);
        fPed->SetParameters(pmtFit->GetParameter(0), pmtFit->GetParameter(1), pmtFit->GetParameter(2));
        fPed->SetLineColor(kBlack); fPed->SetLineStyle(2); fPed->Draw("SAME"); 

        TF1 *f1pe = new TF1(Form("f1pe_Ch%d", chId), "gaus", 0, 1500);
        f1pe->SetParameters(pmtFit->GetParameter(3), pmtFit->GetParameter(4), pmtFit->GetParameter(5));
        f1pe->SetLineColor(kRed); f1pe->SetLineStyle(2); f1pe->Draw("SAME");   

        TF1 *f2pe = new TF1(Form("f2pe_Ch%d", chId), "gaus", 0, 1500);
        f2pe->SetParameters(pmtFit->GetParameter(6), pmtFit->GetParameter(7), pmtFit->GetParameter(8));
        f2pe->SetLineColor(kGreen+2); f2pe->SetLineStyle(2); f2pe->Draw("SAME"); 

        double speGain = pmtFit->GetParameter(4) - pmtFit->GetParameter(1); 
        printf("  => \033[1;36mCh%d Absolute PMT Gain (1PE - Pedestal) = %.1f ADC counts\033[0m\n", chId, speGain);

        row++;
    }
    c1->Update();
}
