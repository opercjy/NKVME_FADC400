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
#include "TGraph.h"
#include "TString.h"
#include "TPad.h"

#include "Pmt.hh"
#include "RunInfo.hh"

void offline_charge(const char* inputFile = "") {
    TString filename(inputFile);

    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] General Charge Spectrum Analysis\033[0m\n";
        std::cout << "  root -l 'offline_charge.cpp(\"your_data_prod.root\")'\n\n";
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
    if (!tree) {
        std::cerr << "\033[1;31m[ERROR]\033[0m 'PROD' tree not found in the file!\n";
        return;
    }

    TTreeReader reader("PROD", f.get());
    TTreeReaderValue<TClonesArray> pmtArray(reader, "Pmt");

    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wTimeMap;
    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wDropMap;
    
    // [버그 픽스] 최대 32채널까지 브랜치를 동적으로 탐색합니다
    for (int i = 0; i < 32; ++i) {
        if (tree->GetBranch(Form("wTime_Ch%d", i))) {
            wTimeMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wTime_Ch%d", i));
            wDropMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wDrop_Ch%d", i));
        }
    }

    std::map<int, TH1F*> hQtotMap;               
    std::map<int, std::vector<double>> ev0Time;  
    std::map<int, std::vector<double>> ev0Drop;  

    int eventCount = 0;

    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            if (hQtotMap.find(chId) == hQtotMap.end()) {
                hQtotMap[chId] = new TH1F(Form("hQtot_Ch%d", chId), 
                                          Form("General Charge Spectrum (Ch%d);Charge (ADC int);Counts", chId), 
                                          500, 0, 10000);
                
                hQtotMap[chId]->SetMinimum(0.5); 
                
                int colors[] = {kAzure+1, kRed-4, kGreen+2, kOrange+1};
                hQtotMap[chId]->SetFillColor(colors[chId % 4]);
            }
            hQtotMap[chId]->Fill(pmt->GetQtot());
        }

        if (eventCount == 0 && !wTimeMap.empty()) {
            for (auto const& [ch, wTimeVal] : wTimeMap) {
                ev0Time[ch] = **wTimeVal;
                ev0Drop[ch] = **wDropMap[ch];
            }
        }
        eventCount++;
    }

    std::cout << "\033[1;32m[INFO]\033[0m Successfully analyzed " << eventCount << " events.\n";

    int nActiveCh = hQtotMap.size();
    if (nActiveCh == 0) return;

    auto c1 = new TCanvas("c1_charge", "Charge Spectrum Viewer", 1200, 400 * nActiveCh);
    c1->Divide(2, nActiveCh); 

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        c1->cd(row * 2 + 1);
        if (ev0Time.find(chId) != ev0Time.end() && !ev0Time[chId].empty()) {
            auto grWave = new TGraph(ev0Time[chId].size(), ev0Time[chId].data(), ev0Drop[chId].data());
            grWave->SetTitle(Form("Event 0 Waveform (Ch%d);Time (ns);Voltage Drop (ADC)", chId));
            int colors[] = {kBlue, kRed, kGreen+2, kOrange+2};
            grWave->SetLineColor(colors[chId % 4]);
            grWave->SetLineWidth(2);
            grWave->Draw("AL");
        }

        c1->cd(row * 2 + 2);
        gPad->SetLogy(); 
        hist->Draw("HIST");

        row++;
    }
    c1->Update();
}
