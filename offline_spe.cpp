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

// 환경 변수(setup.sh) 설정 덕분에 절대 경로 없이 깔끔하게 Include 가능
#include "Pmt.hh"
#include "RunInfo.hh"

// [주의] ROOT 매크로의 특성상 파일명(offline_spe)과 함수명이 반드시 일치해야 합니다.
void offline_spe(const char* inputFile = "") {
    TString filename(inputFile);

    // =========================================================
    // 0. 입력 파라미터 검증 및 사용법 안내
    // =========================================================
    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] PMT Single Photo-electron (SPE) Calibration\033[0m\n";
        std::cout << "  root -l 'offline_spe.cpp(\"your_data_prod.root\")'\n\n";
        return;
    }

    // =========================================================
    // 1. 필수 공유 라이브러리 로드 (.so)
    // =========================================================
    if (gSystem->Load("libNKVME_FADC400.so") < 0) {
        std::cerr << "\033[1;31m[ERROR]\033[0m Cannot load libNKVME_FADC400.so!\n";
        std::cerr << "        Did you forget to run 'source setup.sh' ?\n";
        return;
    }

    // =========================================================
    // 2. 스마트 포인터로 파일 열기 및 메타데이터 파싱
    // =========================================================
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

    // =========================================================
    // 3. 동적 채널 바인딩 (TTreeReader 활용)
    // =========================================================
    TTreeReader reader("PROD", f.get());
    TTreeReaderValue<TClonesArray> pmtArray(reader, "Pmt");

    std::map<int, TH1F*> hQtotMap; // 채널별 전하량 히스토그램
    int eventCount = 0;

    // =========================================================
    // 4. 이벤트 추출 루프 (Data Extraction)
    // =========================================================
    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            if (hQtotMap.find(chId) == hQtotMap.end()) {
                // 🔥 SPE 분석 핵심: 첫 번째 피크(1 p.e.)를 세밀하게 보기 위해 X축을 0~2000으로 좁힘
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

    // =========================================================
    // 5. 캔버스 렌더링 및 교과서적 SPE 캘리브레이션 (Multi-Gaussian)
    // =========================================================
    int nActiveCh = hQtotMap.size();
    if (nActiveCh == 0) return;

    gStyle->SetOptFit(1111); // 피팅 파라미터 결과창 활성화
    
    // 파형 없이 스펙트럼 피팅에만 집중하므로 1열로 캔버스 분할
    auto c1 = new TCanvas("c1_spe", "PMT SPE Calibration Viewer", 800, 400 * nActiveCh);
    c1->Divide(1, nActiveCh); 

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        c1->cd(row + 1);
        gPad->SetLogy(); // 스펙트럼 꼬리 관찰을 위한 Log-Y 적용
        hist->Draw("HIST");

        std::cout << "\n\033[1;35m[Fit]\033[0m Attempting Textbook SPE Fit for Channel " << chId << "...\n";
        
        // ---------------------------------------------------------
        // 💡 교과서적 PMT 피팅 모델 (가우시안 3개의 합성)
        // [0~2]: Pedestal (전자 노이즈)
        // [3~5]: 1st p.e. (단일 광전자 피크)
        // [6~8]: 2nd p.e. (이중 광전자 피크)
        // ---------------------------------------------------------
        TF1 *pmtFit = new TF1(Form("pmtFit_Ch%d", chId), "gaus(0) + gaus(3) + gaus(6)", 0, 1500);
        pmtFit->SetLineColor(kMagenta);
        pmtFit->SetLineWidth(2);
        
        // 각 파라미터에 명시적인 이름 부여 (결과창 가독성 극대화)
        pmtFit->SetParNames("Ped_Amp", "Ped_Mean", "Ped_Sigma", 
                            "1PE_Amp", "1PE_Mean", "1PE_Sigma", 
                            "2PE_Amp", "2PE_Mean", "2PE_Sigma");

        // 💡 초기값(Initial Guess) 세팅
        // (주의: 사용하는 PMT 모델과 인가된 HV에 따라 수정이 필요할 수 있습니다.)
        double maxBinVal = hist->GetMaximum();
        pmtFit->SetParameters(maxBinVal, 50, 20,       // Pedestal 추정
                              maxBinVal/10, 300, 50,   // 1 p.e. 추정
                              maxBinVal/50, 600, 70);  // 2 p.e. 추정

        // ROOT 피팅 엔진 이탈 방지를 위한 물리적 제약조건 (Constraints)
        pmtFit->SetParLimits(1, 0, 150);     // 페데스탈 중심은 낮은 곳에 존재
        pmtFit->SetParLimits(4, 150, 800);   // 1 p.e.는 150~800 구간 어딘가에 존재
        pmtFit->SetParLimits(7, 300, 1600);  // 2 p.e.는 1 p.e.의 약 두 배 위치에 존재

        // 피팅 수행 (Q: Quiet, R: Range 사용, +: 기존 히스토그램에 추가)
        hist->Fit(pmtFit, "QR+"); 

        // ---------------------------------------------------------
        // 합성된 분포 내부의 개별 물리적 구성요소(Component) 해부 및 시각화
        // ---------------------------------------------------------
        TF1 *fPed = new TF1(Form("fPed_Ch%d", chId), "gaus", 0, 1500);
        fPed->SetParameters(pmtFit->GetParameter(0), pmtFit->GetParameter(1), pmtFit->GetParameter(2));
        fPed->SetLineColor(kBlack); fPed->SetLineStyle(2); fPed->Draw("SAME"); // 페데스탈 (검정 점선)

        TF1 *f1pe = new TF1(Form("f1pe_Ch%d", chId), "gaus", 0, 1500);
        f1pe->SetParameters(pmtFit->GetParameter(3), pmtFit->GetParameter(4), pmtFit->GetParameter(5));
        f1pe->SetLineColor(kRed); f1pe->SetLineStyle(2); f1pe->Draw("SAME");   // 1 p.e. (빨강 점선)

        TF1 *f2pe = new TF1(Form("f2pe_Ch%d", chId), "gaus", 0, 1500);
        f2pe->SetParameters(pmtFit->GetParameter(6), pmtFit->GetParameter(7), pmtFit->GetParameter(8));
        f2pe->SetLineColor(kGreen+2); f2pe->SetLineStyle(2); f2pe->Draw("SAME"); // 2 p.e. (초록 점선)

        // ---------------------------------------------------------
        // 최종 산출물: 절대 이득 (Absolute Gain) 계산 및 터미널 출력
        // ---------------------------------------------------------
        double speGain = pmtFit->GetParameter(4) - pmtFit->GetParameter(1); // 1PE Mean - Pedestal Mean
        printf("  => \033[1;36mCh%d Absolute PMT Gain (1PE - Pedestal) = %.1f ADC counts\033[0m\n", chId, speGain);

        row++;
    }
    c1->Update();
}
