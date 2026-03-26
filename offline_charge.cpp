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
        std::cout << "\n\033[1;33m[USAGE] 사용법:\033[0m\n";
        std::cout << "  root -l 'offline_charge.cpp(\"your_data_prod.root\")'\n\n";
        return;
    }

    if (gSystem->Load("libNKVME_FADC400.so") < 0) {
        std::cerr << "\033[1;31m[ERROR]\033[0m 라이브러리 로드 실패! 'source setup.sh'를 실행했는지 확인하세요.\n";
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
            hQtotMap[chId]->Fill(pmt->Qtot()); 

            // 💡 [핵심] 외부 배열 없이 Pmt 내부에 아름답게 캡슐화된 파형 포인터(Waveform())를 직접 읽어옵니다!
            if (eventCount == 0 && pmt->Ndp() > 0 && pmt->Waveform() != nullptr) {
                for (int pt = 0; pt < pmt->Ndp(); pt++) {
                    ev0Time[chId].push_back(pt * 2.5);          // Index를 시간으로 환산 (2.5ns/pt)
                    ev0Drop[chId].push_back(pmt->Waveform()[pt]); // Voltage Drop 
                }
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
            grWave->Draw("AL");
        }

        c1->cd(row * 2 + 2); 
        gPad->SetLogy();     
        hist->Draw("HIST");

        row++;
    }
    c1->Update();
}