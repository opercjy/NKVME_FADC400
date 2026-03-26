/*
 * =======================================================================================
 * offline_compton_edge.cpp (Compton Edge & Inverse Problem Solver)
 * =======================================================================================
 * 🚀 [실행 방법]
 * root -l '$NKVME_ROOT/offline_compton_edge.cpp("data/run_101_prod.root", 0, 15000.0)'
 * * 🎓 [물리적 목적]
 * 방사선원(Radioactive Source) 측정 시 발생하는 컴프턴 연속 스펙트럼(Compton Continuum)에서, 
 * 가우시안 분해능 커널에 의해 뭉개진(Smeared) 데이터로부터 실제 컴프턴 엣지(Compton Edge)의 
 * 정확한 ADC 위치를 역산(Inverse Problem)하여 찾아냅니다. 
 * 이를 위해 상보오차함수(Complementary Error Function, ERFC) 피팅을 수행합니다.
 * =======================================================================================
 */

#include <iostream>
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
#include "TLine.h"
#include "TArrow.h"
#include "TLatex.h"
#include "Math/MinimizerOptions.h"

#include "objects/include/Pmt.hh"

// =========================================================================
// [역문제 해결 모델] Smeared Compton Edge Fitting Function
// -------------------------------------------------------------------------
// 계단 함수(Step Function)와 가우시안 커널(Gaussian Kernel)의 컨볼루션 결과인
// 상보오차함수(ERFC)를 사용하여 컴프턴 능선의 절반(Half-maximum) 지점을 찾습니다.
// par[0] : 능선의 진폭 (Amplitude)
// par[1] : 컴프턴 엣지 위치 (Compton Edge ADC) -> 우리가 찾고자 하는 역문제의 해(Solution)
// par[2] : 검출기의 에너지 분해능 (Sigma)
// par[3] : 백그라운드 상수 (Background)
// =========================================================================
Double_t SmearedComptonEdge(Double_t *x, Double_t *par) {
    Double_t val = x[0];
    Double_t amp = par[0];
    Double_t edge_pos = par[1];
    Double_t sigma = par[2];
    Double_t bg = par[3];
    
    // TMath::Erfc는 인자가 0일 때 정확히 절반의 높이(amp/2)를 지납니다.
    // 즉, val == edge_pos 일 때가 능선의 반절 지점이 됩니다.
    return amp * TMath::Erfc((val - edge_pos) / (TMath::Sqrt(2.0) * sigma)) + bg;
}

void offline_compton_edge(const char* filename = "data/run_101_prod.root", int target_ch = 0, double x_max = 15000.0) {
    TString libPath = Form("%s/lib/libNKVME_FADC400.so", gSystem->Getenv("NKVME_ROOT"));
    if (gSystem->Load(libPath) < 0) {
        std::cerr << "\033[1;31m[오류]\033[0m 라이브러리 로드 실패!\n";
        return;
    }

    gStyle->SetOptFit(1111); 
    gStyle->SetStatW(0.25); 
    gStyle->SetStatH(0.20); 
    gStyle->SetStatX(0.95); 
    gStyle->SetStatY(0.95); 
    gStyle->SetStatFontSize(0.035); 
    
    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) return;
    
    TTree* tree = (TTree*)file->Get("PROD");
    if (!tree) return;

    TClonesArray* pmtArray = new TClonesArray("Pmt");
    tree->SetBranchAddress("Pmt", &pmtArray);

    TH1F* h_compton = new TH1F("h_compton", Form("Ch %d Compton Edge Inverse Problem (ERFC Fit); Integrated Charge (ADC #times ns); Counts / Bin", target_ch), 400, 0, x_max);
    h_compton->SetLineColor(kAzure+2);
    h_compton->SetFillColor(kAzure+1);
    h_compton->SetFillStyle(3004);
    
    Long64_t nEntries = tree->GetEntries();
    std::cout << "[분석 시작] 총 이벤트 수: " << nEntries << std::endl;

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        int nPmts = pmtArray->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            Pmt* pmt = (Pmt*)pmtArray->At(j);
            if (pmt->Id() == target_ch) h_compton->Fill(pmt->Qtot()); 
        }
    }

    TCanvas* c1 = new TCanvas("c1_compton", "Compton Edge Solver", 1000, 700);
    c1->cd();
    gPad->SetLogy(0); 
    gPad->SetGrid();

    // 상단 마진 확보
    h_compton->SetMaximum(h_compton->GetMaximum() * 1.3);

    ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2"); 
    ROOT::Math::MinimizerOptions::SetDefaultMaxFunctionCalls(30000); 

    // =========================================================================
    // 💡 데이터로부터 엣지 위치를 찾는 우아한 알고리즘 (Right-to-Left Scan)
    // 저에너지 영역(가장 높은 피크)의 방해를 피하기 위해, 고에너지 대역부터 거꾸로 스캔합니다.
    // =========================================================================
    
    // 1. 저에너지 노이즈를 피하기 위한 탐색 시작점 (약 2000 ADC 이상부터 진짜 능선으로 취급)
    int startBin = h_compton->FindBin(2000.0); 
    int lastBin = h_compton->GetNbinsX();
    
    // 2. 고에너지 연속 스펙트럼(Continuum) 대역에서의 평균적인 '능선 높이(Amplitude)' 탐색
    double continuumAmp = 0;
    for (int b = startBin; b <= lastBin; ++b) {
        if (h_compton->GetBinContent(b) > continuumAmp) {
            continuumAmp = h_compton->GetBinContent(b);
        }
    }
    
    // 3. 우측 끝(고에너지)에서 좌측으로 스캔하며, 카운트가 능선 높이의 절반(Half-Max)으로 솟아오르는 지점을 찾음
    double guessEdgePos = 0;
    for (int b = lastBin; b >= startBin; --b) {
        if (h_compton->GetBinContent(b) >= continuumAmp * 0.5) {
            guessEdgePos = h_compton->GetBinCenter(b);
            break;
        }
    }

    // 만약 예외적으로 엣지를 못 찾았다면 안전장치로 중앙값을 대입
    if (guessEdgePos == 0) guessEdgePos = x_max * 0.5;

    // 피팅 구간은 추정된 엣지의 주변 대역으로 한정합니다 (예: 엣지의 50% ~ 130% 대역)
    TF1* fitEdge = new TF1("fitEdge", SmearedComptonEdge, guessEdgePos * 0.5, guessEdgePos * 1.3, 4);
    fitEdge->SetParNames("Amplitude", "Compton_Edge_ADC", "Resolution_Sigma", "Background");
    fitEdge->SetLineColor(kRed);
    fitEdge->SetLineWidth(3);
    
    // 역방향 스캔으로 찾은 강력한 초기값을 피팅 엔진에 제공
    fitEdge->SetParameter(0, continuumAmp);         // 컴프턴 능선의 높이
    fitEdge->SetParameter(1, guessEdgePos);         // 우리가 역으로 추적한 엣지의 위치
    fitEdge->SetParameter(2, guessEdgePos * 0.1);   // 에너지 분해능은 보통 위치의 10~15% 내외
    fitEdge->SetParameter(3, 10.0);                 // 백그라운드 상수

    // 파라미터 경계 제약 (엣지 위치가 엉뚱한 곳으로 튀지 않도록 가두기)
    fitEdge->SetParLimits(1, guessEdgePos * 0.7, guessEdgePos * 1.2); 

    h_compton->Draw("HIST");
    h_compton->Fit("fitEdge", "QR+"); 

    // =========================================================================
    // 💡 [우아한 시각화 개선] 짧은 화살표와 수식(TLatex) 라벨 추가
    // =========================================================================
    double edgeADC = fitEdge->GetParameter(1);
    double y_edge = fitEdge->Eval(edgeADC);

    // 짧고 세련된 화살표 세팅 (엣지에서 우측 상단으로 살짝 비스듬히 뻗은 형태)
    double arrowStartX = edgeADC + (x_max * 0.05);  
    double arrowStartY = y_edge + (continuumAmp * 0.25); 

    // "|>" 옵션을 사용하여 속이 채워진 예쁜 삼각형 화살촉 생성
    TArrow *arrEdge = new TArrow(arrowStartX, arrowStartY, edgeADC, y_edge, 0.015, "|>");
    arrEdge->SetLineColor(kMagenta);
    arrEdge->SetLineWidth(2);
    arrEdge->SetFillColor(kMagenta);
    arrEdge->Draw();

    // ROOT의 TLatex를 사용하여 수식 형태(E_{Compton Edge})로 우아하게 텍스트 배치
    // 화살표 꼬리 바로 우측에 정렬
    TLatex *latEdge = new TLatex(arrowStartX + (x_max * 0.01), arrowStartY, "E_{Compton Edge}");
    latEdge->SetTextSize(0.035);
    latEdge->SetTextColor(kMagenta);
    latEdge->SetTextAlign(12); // 12: Left-Center 정렬 (화살표 꼬리 바로 옆에 수직 중앙 정렬)
    latEdge->Draw();

    std::cout << "\n========================================================" << std::endl;
    std::cout << " [컴프턴 엣지 역문제 해석 결과 | Channel " << target_ch << "]" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << " 1. 연속 스펙트럼(능선) 높이 : " << continuumAmp << " Counts" << std::endl;
    std::cout << " 2. 컴프턴 능선 절반 위치 (Compton Edge) : \033[1;36m" << edgeADC << " ADC\033[0m" << std::endl;
    std::cout << " 3. 검출기 에너지 분해능 (Sigma) : " << fitEdge->GetParameter(2) << " ADC" << std::endl;
    std::cout << "--------------------------------------------------------" << std::endl;
    std::cout << " 🎓 [물리적 의미]" << std::endl;
    std::cout << " 피팅된 엣지 위치(" << edgeADC << " ADC)는 감마선이 전자와 180도" << std::endl;
    std::cout << " 후방 산란할 때 전달하는 최대 에너지(E_c)에 대응하는 채널입니다." << std::endl;
    std::cout << " 가우시안 커널 스미어링을 걷어낸 진정한 역문제의 해(Solution)입니다." << std::endl;
    std::cout << "========================================================" << std::endl;
}