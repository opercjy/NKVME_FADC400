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

// 환경 변수(setup.sh) 설정 덕분에 절대 경로 없이 깔끔하게 Include 가능
#include "Pmt.hh"
#include "RunInfo.hh"

// [주의] ROOT 매크로의 특성상 파일명(offline_charge)과 함수명이 반드시 일치해야 합니다.
void offline_charge(const char* inputFile = "") {
    TString filename(inputFile);

    // =========================================================
    // 0. 입력 파라미터 검증 및 사용법 안내
    // =========================================================
    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] General Charge Spectrum Analysis\033[0m\n";
        std::cout << "  root -l 'offline_charge.cpp(\"your_data_prod.root\")'\n\n";
        return;
    }

    // =========================================================
    // 1. 필수 공유 라이브러리 로드 (.so)
    // =========================================================
    // gSystem->Load는 환경 변수 LD_LIBRARY_PATH를 참조합니다.
    if (gSystem->Load("libNKVME_FADC400.so") < 0) {
        std::cerr << "\033[1;31m[ERROR]\033[0m Cannot load libNKVME_FADC400.so!\n";
        std::cerr << "        Did you forget to run 'source setup.sh' ?\n";
        return;
    }

    // =========================================================
    // 2. 스마트 포인터로 파일 열기 및 메타데이터 파싱
    // =========================================================
    // TString의 .Data()를 통해 C-style 문자열로 안전하게 전달합니다.
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

    // =========================================================
    // 3. 동적 채널 바인딩 (Dynamic Channel Parsing)
    // =========================================================
    // 모던 ROOT 6의 핵심: SetBranchAddress 대신 TTreeReader 사용
    TTreeReader reader("PROD", f.get());
    TTreeReaderValue<TClonesArray> pmtArray(reader, "Pmt");

    // 파형 브랜치가 존재하는 채널만 Map에 동적으로 할당 (-w 옵션 대응)
    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wTimeMap;
    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wDropMap;
    
    for (int i = 0; i < 4; ++i) {
        if (tree->GetBranch(Form("wTime_Ch%d", i))) {
            wTimeMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wTime_Ch%d", i));
            wDropMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wDrop_Ch%d", i));
        }
    }

    // =========================================================
    // 4. 데이터 컨테이너 준비
    // =========================================================
    std::map<int, TH1F*> hQtotMap;               // 채널별 전하량 히스토그램
    std::map<int, std::vector<double>> ev0Time;  // 첫 번째 이벤트 파형(시간)
    std::map<int, std::vector<double>> ev0Drop;  // 첫 번째 이벤트 파형(전압 강하량)

    int eventCount = 0;

    // =========================================================
    // 5. 이벤트 추출 루프 (Fast Data Extraction)
    // =========================================================
    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            // 처음 발견된 채널이면 히스토그램을 동적으로 생성
            if (hQtotMap.find(chId) == hQtotMap.end()) {
                // 범용 분석이므로 X축(Charge)을 0~10000 범위로 넓게 잡습니다.
                hQtotMap[chId] = new TH1F(Form("hQtot_Ch%d", chId), 
                                          Form("General Charge Spectrum (Ch%d);Charge (ADC int);Counts", chId), 
                                          500, 0, 10000);
                
                // 로그 스케일에서 빈(Bin)이 0일 때 그래픽이 깨지는 현상을 방지하는 트릭
                hQtotMap[chId]->SetMinimum(0.5); 
                
                // 채널 번호에 따라 그래프 색상을 다르게 자동 배정
                int colors[] = {kAzure+1, kRed-4, kGreen+2, kOrange+1};
                hQtotMap[chId]->SetFillColor(colors[chId % 4]);
            }
            // 해당 채널의 전하량 누적
            hQtotMap[chId]->Fill(pmt->GetQtot());
        }

        // 파형 시각화를 위해 첫 번째(Event 0) 파형 데이터만 복사
        if (eventCount == 0 && !wTimeMap.empty()) {
            for (auto const& [ch, wTimeVal] : wTimeMap) {
                ev0Time[ch] = **wTimeVal;
                ev0Drop[ch] = **wDropMap[ch];
            }
        }
        eventCount++;
    }

    std::cout << "\033[1;32m[INFO]\033[0m Successfully analyzed " << eventCount << " events.\n";

    // =========================================================
    // 6. 동적 캔버스 렌더링 (파형 + Log-Y 스펙트럼)
    // =========================================================
    int nActiveCh = hQtotMap.size();
    if (nActiveCh == 0) return;

    // 활성화된 채널 개수에 따라 캔버스 세로 길이를 동적으로 늘림 (채널당 400px)
    auto c1 = new TCanvas("c1_charge", "Charge Spectrum Viewer", 1200, 400 * nActiveCh);
    c1->Divide(2, nActiveCh); // 좌측: 파형 / 우측: 스펙트럼

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        // [A] 좌측 패드: 파형 그리기
        c1->cd(row * 2 + 1);
        if (ev0Time.find(chId) != ev0Time.end() && !ev0Time[chId].empty()) {
            auto grWave = new TGraph(ev0Time[chId].size(), ev0Time[chId].data(), ev0Drop[chId].data());
            grWave->SetTitle(Form("Event 0 Waveform (Ch%d);Time (ns);Voltage Drop (ADC)", chId));
            int colors[] = {kBlue, kRed, kGreen+2, kOrange+2};
            grWave->SetLineColor(colors[chId % 4]);
            grWave->SetLineWidth(2);
            grWave->Draw("AL");
        }

        // [B] 우측 패드: 전하량 히스토그램 (Log-Y 스케일)
        c1->cd(row * 2 + 2);
        gPad->SetLogy(); // 🔥 고에너지 Tail을 보기 위한 Y축 로그 스케일 적용
        hist->Draw("HIST");

        row++;
    }
    c1->Update();
}
