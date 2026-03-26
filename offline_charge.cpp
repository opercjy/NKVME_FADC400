/*
 * offline_charge.cpp (Educational Analysis Dashboard)
 * * [목적]
 * PMT 신호의 파형(Waveform) 시계열 누적 렌더링, 진폭-전하량 상관관계 분석,
 * 그리고 물리적 Cut 적용이 스펙트럼에 미치는 영향을 4분할 캔버스로 교육하는 매크로입니다.
 * * * [교육적 핵심]
 * 아날로그 회로 구성의 편의성 때문에 종종 오용되는 '펄스 파고(Pulse Height/Amplitude)' 방법론의 한계와,
 * 파형 전체 면적을 구하여 노이즈를 상쇄하고 잔피크(Fine structures) 및 뛰어난 에너지 분해능을 보여주는 
 * '차지섬(Charge Sum / 펄스 적분)' 방법론의 우수성을 직관적으로 비교할 수 있도록 설계되었습니다.
 * [실행]
 * root -l 'offline_charge.cpp("dir/run_101_prod.root", 0, 0.0)'
 */

#include <iostream>
#include "TSystem.h"
#include "TROOT.h"

// 환경 변수($NKVME_ROOT)를 활용한 동적 라이브러리 및 헤더 로드
R__ADD_INCLUDE_PATH($NKVME_ROOT)

#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLine.h"
#include "TLegend.h"

#include "objects/include/Pmt.hh"

void offline_charge(const char* filename = "data/run_101_prod.root", int target_ch = 0, float amp_cut = 5.0) {
    // 라이브러리 동적 로드
    TString libPath = Form("%s/lib/libNKVME_FADC400.so", gSystem->Getenv("NKVME_ROOT"));
    gSystem->Load(libPath);

    // [1. 시각화 스타일 셋업]
    gStyle->SetOptStat(0);             // 통계 박스(Mean, RMS 등)를 숨겨 그래프에 집중하게 함
    gStyle->SetPalette(kBird);         // 2D 산점도의 밀도를 명확히 보여주는 온도 컬러맵 적용
    gStyle->SetNumberContours(256);    // 컬러맵의 그라데이션을 부드럽게 처리
    
    // [2. 파일 및 트리 연결]
    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "[오류] 파일을 열 수 없습니다: " << filename << std::endl;
        return;
    }
    
    TTree* tree = (TTree*)file->Get("PROD");
    if (!tree) {
        std::cerr << "[오류] PROD 트리를 찾을 수 없습니다." << std::endl;
        return;
    }

    // 💡 [핵심 해결책] 분할(Split) 배열을 가장 완벽하게 읽어내는 클래식 포인터 방식
    TClonesArray* pmtArray = new TClonesArray("Pmt");
    tree->SetBranchAddress("Pmt", &pmtArray);

    // [3. 교육용 히스토그램 정의]
    // X축 범위를 0 ~ 1280ns (512포인트 * 2.5ns)로 설정
    TH2F* h_wave_persist = new TH2F("h_wave_persist", Form("Ch %d Waveform Persistence; Time (ns); Voltage Drop (ADC)", target_ch), 512, 0, 1280, 200, -50, 1000);
    TH2F* h_amp_charge = new TH2F("h_amp_charge", Form("Ch %d Correlation: Amplitude vs Charge; Integrated Charge (ADC #times ns); Peak Amplitude (ADC)", target_ch), 300, -500, 30000, 200, -50, 1000);
    
    // 💡 교육적 목적 달성을 위한 명시적인 타이틀 설정 (적분 vs 파고 비교)
    TH1F* h_charge_raw = new TH1F("h_charge_raw", Form("Ch %d Charge Spectrum (Pulse Integration); Integrated Charge (ADC #times ns); Counts / Bin", target_ch), 300, -500, 30000);
    TH1F* h_charge_cut = new TH1F("h_charge_cut", Form("Ch %d Charge Spectrum (Amp > %.1f); Integrated Charge (ADC #times ns); Counts / Bin", target_ch, amp_cut), 300, -500, 30000);
    TH1F* h_amp_raw = new TH1F("h_amp_raw", Form("Ch %d Amplitude Spectrum (Pulse Height); Peak Amplitude (ADC); Counts / Bin", target_ch), 200, -50, 1000);
    TH1F* h_amp_cut = new TH1F("h_amp_cut", Form("Ch %d Amplitude Cut (Amp > %.1f); Peak Amplitude (ADC); Counts / Bin", target_ch, amp_cut), 200, -50, 1000);

    h_charge_raw->SetLineColor(kBlack);
    h_charge_cut->SetLineColor(kRed); h_charge_cut->SetFillColor(kRed); h_charge_cut->SetFillStyle(3004);
    
    h_amp_raw->SetLineColor(kBlack);
    h_amp_cut->SetLineColor(kBlue); h_amp_cut->SetFillColor(kBlue); h_amp_cut->SetFillStyle(3004);

    // [4. 이벤트 루프 및 데이터 파싱]
    Long64_t nEntries = tree->GetEntries();
    std::cout << "[분석 시작] 총 이벤트 수: " << nEntries << std::endl;
    int filled_events = 0;

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        
        int nPmts = pmtArray->GetEntriesFast();
        
        // 디버그 출력 (첫 번째 이벤트 한정)
        if (i == 0) {
            std::cout << "[디버그] 첫 이벤트 내부 Pmt 데이터 개수: " << nPmts << "개" << std::endl;
            for (int d = 0; d < nPmts; ++d) {
                Pmt* p = (Pmt*)pmtArray->At(d);
                std::cout << "  -> 발견된 채널 ID: " << p->Id() << std::endl;
            }
        }
        
        for (int j = 0; j < nPmts; ++j) {
            Pmt* pmt = (Pmt*)pmtArray->At(j);
            
            // 타겟 채널 필터링
            if (pmt->Id() != target_ch) continue;
            
            filled_events++;
            
            float charge = pmt->Qtot(); // 핵심 물리량: 적분 전하량 (Charge Sum)
            float amp = pmt->Qmax();    // 비교 물리량: 베이스라인이 차감된 최대 진폭 (Pulse Height)

            h_charge_raw->Fill(charge);
            h_amp_raw->Fill(amp);
            h_amp_charge->Fill(charge, amp); 

            // 물리 필터링: 지정된 진폭 컷을 통과한 신호 분리
            if (amp > amp_cut) {
                h_charge_cut->Fill(charge);
                h_amp_cut->Fill(amp);
            }

            float* wave = pmt->Waveform();
            int ndp = pmt->Ndp();
            if (wave && ndp > 0) {
                // 동적 NDP 대응: 기본 설정 범위를 벗어날 경우 히스토그램 범위를 자동 재조정
                if (ndp * 2.5 > h_wave_persist->GetXaxis()->GetXmax()) {
                     h_wave_persist->SetBins(ndp, 0, ndp * 2.5, 200, -50, 1000);
                }
                for (int pt = 0; pt < ndp; ++pt) {
                    float time_ns = pt * 2.5; 
                    h_wave_persist->Fill(time_ns, wave[pt]); 
                }
            }
        }
    }

    std::cout << "[분석 완료] 타겟 채널(" << target_ch << ") 매칭 이벤트: " << filled_events << "개" << std::endl;

    // [5. 대시보드 렌더링]
    TCanvas* c1 = new TCanvas("c1", "Educational Analysis Dashboard", 1200, 800);
    c1->Divide(2, 2);

    // 패널 1: Waveform Persistence
    c1->cd(1); gPad->SetGrid(); gPad->SetRightMargin(0.12); h_wave_persist->Draw("COLZ");
    
    // 패널 2: Amplitude vs Charge 상관관계 산점도
    c1->cd(2); gPad->SetGrid(); gPad->SetRightMargin(0.12); h_amp_charge->Draw("COLZ");
    TLine* line_cut = new TLine(-500, amp_cut, 30000, amp_cut);
    line_cut->SetLineColor(kRed); line_cut->SetLineWidth(2); line_cut->SetLineStyle(2); line_cut->Draw();

    // 패널 3: Charge Spectrum (Charge Sum의 우수성 확인)
    c1->cd(3); gPad->SetLogy(); gPad->SetGrid(); h_charge_raw->Draw(); h_charge_cut->Draw("SAME");
    TLegend* leg_q = new TLegend(0.55, 0.7, 0.85, 0.85);
    leg_q->AddEntry(h_charge_raw, "Raw Data (Noise included)", "l");
    leg_q->AddEntry(h_charge_cut, Form("Signal (Amp > %.1f)", amp_cut), "f");
    leg_q->Draw();

    // 패널 4: Amplitude Spectrum (Pulse Height의 한계 확인)
    c1->cd(4); gPad->SetLogy(); gPad->SetGrid(); h_amp_raw->Draw(); h_amp_cut->Draw("SAME");
    TLine* line_amp = new TLine(amp_cut, 0, amp_cut, h_amp_raw->GetMaximum());
    line_amp->SetLineColor(kRed); line_amp->SetLineWidth(2); line_amp->SetLineStyle(2); line_amp->Draw();
}