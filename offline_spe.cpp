/*
 * =======================================================================================
 * offline_spe.cpp (Single Photo-electron Calibration & Absolute Gain Extraction)
 * =======================================================================================
 * 🚀 [실행 방법 / How to Run]
 * 터미널을 열고 아래 명령어를 실행하세요. (실행 전 'source setup.sh' 필수)
 * * 1. [순수 SPE 모드] LED/Laser 등 미약한 빛으로 1-PE를 볼 때 (X축 3000 권장)
 * root -l '$NKVME_ROOT/offline_spe.cpp("data/run_101_prod.root", 0, 3000.0)'
 * * 2. [컴프턴 광역대 모드] 방사선원+액체섬광체 등 대량의 광자를 볼 때 (X축 12000 이상 권장)
 * root -l '$NKVME_ROOT/offline_spe.cpp("data/run_101_prod.root", 0, 12000.0)'
 *
 * ---------------------------------------------------------------------------------------
 * 🎓 [교육적 목적 및 물리적 의도]
 * 1. 왜 'Charge Sum(펄스 적분)'인가?
 * - 원자력 공학에서 편의상 쓰는 '펄스 파고(Pulse Height)'는 전자 노이즈나 지터(Jitter)에 취약합니다.
 * 미세한 1-PE의 잔피크(Fine structures)를 명확히 분리하고 압도적인 에너지 분해능을 얻으려면 
 * 파형 전체의 면적을 구하는 '펄스 적분(Charge Sum)'이 입자물리학의 절대적인 정석입니다.
 * * 2. 컴프턴 연속 스펙트럼 (Compton Continuum) 주의사항
 * - 방사선원과 표준 액체섬광체를 사용할 경우, 막대한 광자 플럭스(Photon Flux)가 발생하여
 * 단일 광전자(1-PE, 2-PE) 피크들이 거대한 컴프턴 능선(Compton Continuum)과 후방 엣지(Edge)에 
 * 파묻히게 됩니다. 이 코드는 피팅 결과의 폭(Sigma)을 분석하여 이를 자동으로 감지합니다.
 * =======================================================================================
 */

#include <iostream>
#include <map>
#include "TSystem.h"
#include "TROOT.h"

R__ADD_INCLUDE_PATH($NKVME_ROOT)

#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TH1F.h"
#include "TF1.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TMath.h"
#include "Math/MinimizerOptions.h"

#include "objects/include/Pmt.hh"

// =========================================================================
// [물리 모델] Multi-Gaussian 합성 피팅 함수 (Pedestal + 1-PE + 2-PE)
// =========================================================================
Double_t MultiGaussianFit(Double_t *x, Double_t *par) {
    Double_t val = x[0];
    
    // [0~2]: Pedestal (전자 노이즈, 암전류 등 외부 요인)
    Double_t ped_norm  = par[0];
    Double_t ped_mean  = par[1];
    Double_t ped_sigma = par[2];
    
    // [3~5]: 1st p.e. (단일 광전자 피크, PMT Gain의 기준점)
    Double_t spe_norm  = par[3];
    Double_t spe_mean  = par[4]; 
    Double_t spe_sigma = par[5]; 
    
    // [6]: 2nd p.e. (이중 광전자 피크)
    // [핵심 제약] 2-PE 피크의 위치와 폭은 독립 변수가 아니라 1-PE에 종속된 물리적 파생 변수입니다.
    Double_t dpe_norm  = par[6];
    Double_t dpe_mean  = ped_mean + 2.0 * (spe_mean - ped_mean); 
    Double_t dpe_sigma = spe_sigma * TMath::Sqrt(2.0);           

    Double_t pedestal = ped_norm * TMath::Gaus(val, ped_mean, ped_sigma);
    Double_t single_pe = spe_norm * TMath::Gaus(val, spe_mean, spe_sigma);
    Double_t double_pe = dpe_norm * TMath::Gaus(val, dpe_mean, dpe_sigma);

    return pedestal + single_pe + double_pe;
}

// 💡 [수정] 광역대를 자유롭게 조절할 수 있도록 x_max 파라미터 추가 (기본값 12000.0)
void offline_spe(const char* filename = "data/run_101_prod.root", int target_ch = 0, double x_max = 12000.0) {
    TString libPath = Form("%s/lib/libNKVME_FADC400.so", gSystem->Getenv("NKVME_ROOT"));
    if (gSystem->Load(libPath) < 0) {
        std::cerr << "\033[1;31m[오류]\033[0m 라이브러리 로드 실패! 'source setup.sh'를 실행했는지 확인하세요.\n";
        return;
    }

    // 💡 [UI 픽스] 통계 박스(Legend)가 그래프와 겹치지 않도록 폰트 축소 및 여백 조정
    gStyle->SetOptFit(1111); 
    gStyle->SetStatW(0.25); // 박스 너비 축소
    gStyle->SetStatH(0.25); // 박스 높이 축소
    gStyle->SetStatX(0.95); // 우측 마진
    gStyle->SetStatY(0.95); // 상단 마진
    gStyle->SetStatFontSize(0.03); // 폰트 크기 축소
    
    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) return;
    
    TTree* tree = (TTree*)file->Get("PROD");
    if (!tree) return;

    // 분할(Split) 배열 버그를 피하는 클래식 TClonesArray 포인터 맵핑
    TClonesArray* pmtArray = new TClonesArray("Pmt");
    tree->SetBranchAddress("Pmt", &pmtArray);

    // X축 범위를 x_max 변수로 받아 동적 생성
    TH1F* h_spe = new TH1F("h_spe", Form("Ch %d SPE Calibration (Charge Sum); Integrated Charge (ADC #times ns); Counts / Bin", target_ch), 500, -500, x_max);
    h_spe->SetLineColor(kAzure+1);
    h_spe->SetFillColor(kAzure+1);
    h_spe->SetFillStyle(3004);
    
    Long64_t nEntries = tree->GetEntries();
    std::cout << "[분석 시작] 총 이벤트 수: " << nEntries << std::endl;
    int filled_events = 0;

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        int nPmts = pmtArray->GetEntriesFast();
        
        for (int j = 0; j < nPmts; ++j) {
            Pmt* pmt = (Pmt*)pmtArray->At(j);
            if (pmt->Id() != target_ch) continue;
            
            h_spe->Fill(pmt->Qtot()); 
            filled_events++;
        }
    }

    if (filled_events == 0) return;

    TCanvas* c1 = new TCanvas("c1_spe", "PMT SPE Calibration Viewer", 1000, 700);
    c1->cd();
    gPad->SetLogy(); 
    gPad->SetGrid();

    // 💡 [UI 픽스] Log-Y 스케일에서 꼭대기 데이터가 통계 박스와 겹치지 않도록 Y축 최대치 10배 펌핑
    h_spe->SetMaximum(h_spe->GetMaximum() * 10.0);

    // 강력한 Minuit2 엔진 사용
    ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2"); 
    ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(30000); 
    ROOT::Math::MinimizerOptions::SetDefaultTolerance(0.001);        

    double ped_peak = h_spe->GetMaximum();
    double ped_pos = h_spe->GetBinCenter(h_spe->GetMaximumBin());
    
    TF1* fitFunc = new TF1("fitFunc", MultiGaussianFit, -200, x_max, 7);
    fitFunc->SetParNames("Ped_Norm", "Ped_Mean", "Ped_Sigma", "1PE_Norm", "1PE_Mean", "1PE_Sigma", "2PE_Norm");
    fitFunc->SetLineColor(kMagenta);
    fitFunc->SetLineWidth(2);
    
    fitFunc->SetParameter(0, ped_peak);
    fitFunc->SetParameter(1, ped_pos);
    fitFunc->SetParameter(2, 50.0);
    fitFunc->SetParameter(3, ped_peak * 0.1);  
    fitFunc->SetParameter(4, ped_pos + (x_max * 0.1)); // 탐색 범위에 비례하여 1-PE 초기값 동적 세팅
    fitFunc->SetParameter(5, 100.0);
    fitFunc->SetParameter(6, ped_peak * 0.01); 

    fitFunc->SetParLimits(1, ped_pos - 100, ped_pos + 100);   
    fitFunc->SetParLimits(4, ped_pos + 50, x_max * 0.8); // 최대 x_max의 80% 지점까지 탐색 허용

    h_spe->Draw("HIST");
    h_spe->Fit("fitFunc", "QR+"); 
    
    TF1 *fPed = new TF1("fPed", "gaus", -200, x_max);
    fPed->SetParameters(fitFunc->GetParameter(0), fitFunc->GetParameter(1), fitFunc->GetParameter(2));
    fPed->SetLineColor(kBlack); fPed->SetLineStyle(2); fPed->Draw("SAME");

    TF1 *f1pe = new TF1("f1pe", "gaus", -200, x_max);
    f1pe->SetParameters(fitFunc->GetParameter(3), fitFunc->GetParameter(4), fitFunc->GetParameter(5));
    f1pe->SetLineColor(kRed); f1pe->SetLineStyle(2); f1pe->Draw("SAME");   

    TF1 *f2pe = new TF1("f2pe", "gaus", -200, x_max);
    f2pe->SetParameters(fitFunc->GetParameter(6), 
                        fitFunc->GetParameter(1) + 2.0 * (fitFunc->GetParameter(4) - fitFunc->GetParameter(1)), 
                        fitFunc->GetParameter(5) * TMath::Sqrt(2.0));
    f2pe->SetLineColor(kGreen+2); f2pe->SetLineStyle(2); f2pe->Draw("SAME"); 

    // 절대 이득 (Absolute Gain) 환산
    Double_t mean_ped = fitFunc->GetParameter(1);
    Double_t mean_1pe = fitFunc->GetParameter(4);
    Double_t sigma_1pe = fitFunc->GetParameter(5); // 1-PE의 폭(Sigma)
    Double_t spe_charge_adc = mean_1pe - mean_ped; 

    // FADC400 하드웨어 스펙 기반 정밀 환산 (2.0Vpp, 10-bit(1024), 2.5ns sampling, 50 Ohm 입력 저항)
    Double_t adc_to_pc = (2.0 / 1024.0) * 2.5 / 50.0 * 1000.0; 
    Double_t q_pc = spe_charge_adc * adc_to_pc;
    Double_t gain = q_pc / 1.602e-7; 

    std::cout << "\n========================================================" << std::endl;
    std::cout << " [SPE 캘리브레이션 결과 요약 | Channel " << target_ch << "]" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    
    // 💡 [우아한 예외 처리] 피팅된 1-PE의 폭(Sigma)이 비정상적으로 크면 컴프턴 엣지로 판별
    if (sigma_1pe > 500.0) {
        std::cout << "\033[1;33m [주의] 물리적 예외 감지! (Compton Continuum Detected)\033[0m\n";
        std::cout << "  - 현재 피팅된 거대한 언덕(Sigma: " << sigma_1pe << ")은 단일 광전자(1-PE)가 아닙니다.\n";
        std::cout << "  - 방사선원과 액체섬광체 반응에 의한 전형적인 '컴프턴 능선과 후방 엣지'입니다.\n";
        std::cout << "  - 순수 1-PE를 관찰하려면 LED나 정밀 레이저로 광자 플럭스를 낮추고,\n";
        std::cout << "  - x_max 범위를 3000 정도로 좁혀서 다시 실행하십시오.\n";
        std::cout << "--------------------------------------------------------" << std::endl;
    }

    std::cout << " 1. 페데스탈 위치 (Pedestal)    : " << mean_ped << std::endl;
    std::cout << " 2. 순수 1-PE 전하량 (ADC x ns) : " << spe_charge_adc << std::endl;
    std::cout << " 3. 물리적 단위 환산 (pC)       : " << q_pc << " pC" << std::endl;
    std::cout << " 4. PMT 절대 이득 (Absolute Gain) : \033[1;36m" << std::scientific << gain << "\033[0m" << std::fixed << std::endl;
    std::cout << "========================================================" << std::endl;
}