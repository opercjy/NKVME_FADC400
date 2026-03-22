// =========================================================================
// PMT 단일 광전자 캘리브레이션 매크로 (Single Photo-electron Calibration)
// 작성 목적: PMT의 절대 이득(Absolute Gain)을 추출하기 위해
//           Multi-Gaussian (Pedestal + 1PE + 2PE) 피팅의 정석을 학습합니다.
//           또한, 최신 ROOT 피팅 엔진인 Minuit2의 사용법을 익힙니다.
// =========================================================================

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
#include "Math/MinimizerOptions.h" // [교육] Minuit2 세밀 제어를 위한 핵심 헤더

#include "Pmt.hh"
#include "RunInfo.hh"

void offline_spe(const char* inputFile = "") {
    TString filename(inputFile);

    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] 사용법:\033[0m\n";
        std::cout << "  root -l 'offline_spe.cpp(\"your_data_prod.root\")'\n\n";
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
    int eventCount = 0;

    // 데이터 추출 루프
    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            if (hQtotMap.find(chId) == hQtotMap.end()) {
                // [물리 학습 1] SPE 분석은 미약한 신호를 관찰하므로 X축(전하량)을 0~2000으로 좁힙니다.
                // 만약 PMT Gain이 너무 높거나 낮으면 이 범위를 직접 튜닝해야 합니다.
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

    gStyle->SetOptFit(1111); // 캔버스 우측 상단에 피팅 파라미터 결과창 표시 켬
    
    auto c1 = new TCanvas("c1_spe", "PMT SPE Calibration Viewer", 800, 400 * nActiveCh);
    c1->Divide(1, nActiveCh); // 파형 그래프는 생략하고 캘리브레이션에 집중 (1열 렌더링)

    // =========================================================================
    // [심화 학습: Minuit2 피팅 엔진 명시적 사용 및 제어법]
    // ROOT의 구형 Fit()은 초기값이 조금만 틀어져도 수렴(Convergence)에 실패하기 쉽습니다.
    // 입자물리학 데이터 분석의 표준인 강력한 'Minuit2' 엔진을 호출하고 
    // 정밀도를 높이려면 아래 주석을 해제(Uncomment)하여 사용하십시오.
    // =========================================================================
    /*
    ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2"); // 최신 엔진 강제
    ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(20000); // 끈질기게(반복횟수↑) 답을 찾음
    ROOT::Math::MinimizerOptions::SetDefaultTolerance(0.001);        // 오차 허용치(정밀도) 상향
    ROOT::Math::MinimizerOptions::SetDefaultPrintLevel(1);           // 터미널에 중간 피팅 과정 출력
    std::cout << "\n\033[1;36m[System]\033[0m Default Fitter set to \033[1;33mMinuit2\033[0m.\n";
    */

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        c1->cd(row + 1);
        gPad->SetLogy(); // 고에너지 꼬리를 함께 보기 위한 Log-Y 적용
        hist->Draw("HIST");

        std::cout << "\n\033[1;35m[Fit]\033[0m Attempting Textbook SPE Fit for Channel " << chId << "...\n";
        
        // ---------------------------------------------------------
        // [물리 학습 2: 교과서적 물리 모델] Multi-Gaussian 합성 함수
        // [0~2]: Pedestal (전자 노이즈, 암전류 등 외부 요인)
        // [3~5]: 1st p.e. (단일 광전자 피크, PMT Gain의 기준점)
        // [6~8]: 2nd p.e. (이중 광전자 피크, 1st p.e.의 약 2배 위치)
        // ---------------------------------------------------------
        TF1 *pmtFit = new TF1(Form("pmtFit_Ch%d", chId), "gaus(0) + gaus(3) + gaus(6)", 0, 1500);
        pmtFit->SetLineColor(kMagenta);
        pmtFit->SetLineWidth(2);
        
        // 학생들이 결과창을 보고 직관적으로 이해할 수 있도록 파라미터 이름 명시
        pmtFit->SetParNames("Ped_Amp", "Ped_Mean", "Ped_Sigma", 
                            "1PE_Amp", "1PE_Mean", "1PE_Sigma", 
                            "2PE_Amp", "2PE_Mean", "2PE_Sigma");

        // [핵심] 초기값(Initial Guess) 세팅
        // (수렴 확률을 높이기 위해 데이터의 Maximum Bin을 기준으로 대략적인 위치를 찍어줍니다.)
        double maxBinVal = hist->GetMaximum();
        pmtFit->SetParameters(maxBinVal, 50, 20,       // Pedestal 추정치
                              maxBinVal/10, 300, 50,   // 1 p.e. 추정치
                              maxBinVal/50, 600, 70);  // 2 p.e. 추정치

        // [핵심] 물리적 제약 조건 (Boundary Constraints)
        // ROOT 피팅 엔진이 "1 p.e.를 Pedestal 위치에서 찾는" 멍청한 짓을 막습니다.
        pmtFit->SetParLimits(1, 0, 150);     // 페데스탈은 무조건 0~150 사이 좁은 구간에 존재함을 강제
        pmtFit->SetParLimits(4, 150, 800);   // 1 p.e.는 150~800 구간 어딘가에 존재함을 강제
        pmtFit->SetParLimits(7, 300, 1600);  // 2 p.e.는 1 p.e.의 대략 두 배 위치에 존재함을 강제

        // 피팅 수행 (Q: 조용히, R: 지정된 Range(0~1500) 사용, +: 기존 캔버스에 덧그리기)
        hist->Fit(pmtFit, "QR+"); 

        // ---------------------------------------------------------
        // [물리 학습 3] 합성 함수 내부 구성 요소(Component) 해부 시각화
        // 보라색 실선(전체 함수) 안에 숨겨진 각각의 가우시안을 점선으로 덧그려줍니다.
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
        // [최종 산출물] 절대 이득 (Absolute Gain) 계산 및 터미널 출력
        // PMT Gain = (1PE 평균값) - (Pedestal 평균값)
        // ---------------------------------------------------------
        double speGain = pmtFit->GetParameter(4) - pmtFit->GetParameter(1); 
        printf("  => \033[1;36mCh%d Absolute PMT Gain (1PE - Pedestal) = %.1f ADC counts\033[0m\n", chId, speGain);

        row++;
    }
    c1->Update();
}
