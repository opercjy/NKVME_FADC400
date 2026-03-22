// =========================================================================
// 범용 전하량 스펙트럼 분석 매크로 (General Charge Spectrum Analyzer)
// 작성 목적: 학생 및 연구원들이 다중 채널 DAQ 데이터를 읽어오고, 
//           ROOT를 이용해 고에너지 대역(Log-Y)을 시각화하는 방법을 학습합니다.
// =========================================================================

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>

// ROOT 코어 라이브러리 (파일, 트리, 렌더링 담당)
#include "TSystem.h"
#include "TFile.h"
#include "TTree.h"
#include "TTreeReader.h"        // [교육] 모던 ROOT의 핵심: 트리를 안전하게 순회하는 클래스
#include "TTreeReaderValue.h"   // [교육] 트리의 개별 브랜치 값을 캡슐화하는 클래스
#include "TClonesArray.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TGraph.h"
#include "TString.h"
#include "TPad.h"

// 사용자 정의 데이터 클래스 헤더 (setup.sh의 환경 변수 덕분에 경로 없이 인식됨)
#include "Pmt.hh"
#include "RunInfo.hh"

void offline_charge(const char* inputFile = "") {
    TString filename(inputFile);

    // ---------------------------------------------------------
    // [학습 포인트 1] 입력 파라미터 검증 (방어적 프로그래밍)
    // 매크로 실행 시 파일명을 누락했을 때 프로그램이 뻗지 않고 
    // 올바른 사용법(Usage)을 안내한 뒤 안전하게 종료하도록 합니다.
    // ---------------------------------------------------------
    if (filename.IsNull() || filename == "") {
        std::cout << "\n\033[1;33m[USAGE] 사용법:\033[0m\n";
        std::cout << "  root -l 'offline_charge.cpp(\"your_data_prod.root\")'\n\n";
        return;
    }

    // ---------------------------------------------------------
    // [학습 포인트 2] 사용자 정의 클래스(Dictionary) 동적 로드
    // ROOT는 기본 타입(int, double)이 아닌 우리가 만든 'Pmt' 클래스 등을
    // 파일에서 읽으려면 그 설계도가 담긴 .so 라이브러리가 반드시 필요합니다.
    // ---------------------------------------------------------
    if (gSystem->Load("libNKVME_FADC400.so") < 0) {
        std::cerr << "\033[1;31m[ERROR]\033[0m 라이브러리 로드 실패! 'source setup.sh'를 실행했는지 확인하세요.\n";
        return;
    }

    // ---------------------------------------------------------
    // [학습 포인트 3] 스마트 포인터(std::unique_ptr)를 활용한 메모리 누수 방지
    // 일반 포인터(new)를 쓰면 함수 종료 시 delete를 빼먹기 쉽습니다.
    // unique_ptr을 사용하면 에러로 함수를 빠져나가도 파일이 안전하게 닫힙니다.
    // ---------------------------------------------------------
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

    // 과거의 SetBranchAddress를 대체하는 모던 ROOT 순회 방식 (타입 안정성 보장)
    TTreeReader reader("PROD", f.get());
    TTreeReaderValue<TClonesArray> pmtArray(reader, "Pmt");

    // ---------------------------------------------------------
    // [학습 포인트 4] std::map을 활용한 동적 채널 할당
    // 하드코딩된 배열 크기를 쓰지 않고, 파일 안에 실제로 존재하는 채널만 
    // 탐색하여 맵(Map)에 등록합니다. 보드가 몇 개든 알아서 적응합니다.
    // ---------------------------------------------------------
    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wTimeMap;
    std::map<int, std::unique_ptr<TTreeReaderValue<std::vector<double>>>> wDropMap;
    
    for (int i = 0; i < 32; ++i) { // 보드 8장 분량(32채널)까지 스캔
        if (tree->GetBranch(Form("wTime_Ch%d", i))) {
            wTimeMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wTime_Ch%d", i));
            wDropMap[i] = std::make_unique<TTreeReaderValue<std::vector<double>>>(reader, Form("wDrop_Ch%d", i));
        }
    }

    std::map<int, TH1F*> hQtotMap;               
    std::map<int, std::vector<double>> ev0Time;  
    std::map<int, std::vector<double>> ev0Drop;  

    int eventCount = 0;

    // ---------------------------------------------------------
    // [학습 포인트 5] 이벤트 데이터 추출 루프
    // reader.Next()를 호출할 때마다 파일에서 다음 이벤트를 메모리로 가져옵니다.
    // ---------------------------------------------------------
    while (reader.Next()) {
        int nPmts = (*pmtArray)->GetEntriesFast();
        for (int j = 0; j < nPmts; ++j) {
            // static_cast로 다운캐스팅하여 Pmt 객체의 물리량(전하량 등)에 접근
            auto pmt = static_cast<Pmt*>((*pmtArray)->At(j));
            int chId = pmt->GetId();

            // 처음 발견된 채널이면 히스토그램을 메모리에 동적 생성
            if (hQtotMap.find(chId) == hQtotMap.end()) {
                hQtotMap[chId] = new TH1F(Form("hQtot_Ch%d", chId), 
                                          Form("General Charge Spectrum (Ch%d);Charge (ADC int);Counts", chId), 
                                          500, 0, 10000);
                
                // [실전 꿀팁] Y축을 Log 스케일로 그릴 때 빈 카운트가 0이면 
                // 그래픽 하단이 깨지거나 경고가 뜹니다. 0.5로 하한선을 두어 방어합니다.
                hQtotMap[chId]->SetMinimum(0.5); 
                
                int colors[] = {kAzure+1, kRed-4, kGreen+2, kOrange+1};
                hQtotMap[chId]->SetFillColor(colors[chId % 4]);
            }
            hQtotMap[chId]->Fill(pmt->GetQtot()); // 전하량 누적
        }

        // 파형 시각화를 위해 첫 번째 이벤트(Event 0)의 파형만 따로 복사해둠
        if (eventCount == 0 && !wTimeMap.empty()) {
            for (auto const& [ch, wTimeVal] : wTimeMap) {
                ev0Time[ch] = **wTimeVal;
                ev0Drop[ch] = **wDropMap[ch];
            }
        }
        eventCount++;
    }

    std::cout << "\033[1;32m[INFO]\033[0m Successfully analyzed " << eventCount << " events.\n";

    // ---------------------------------------------------------
    // 6. 캔버스 렌더링 (활성화된 채널 수만큼 동적 세로 분할)
    // ---------------------------------------------------------
    int nActiveCh = hQtotMap.size();
    if (nActiveCh == 0) return;

    auto c1 = new TCanvas("c1_charge", "Charge Spectrum Viewer", 1200, 400 * nActiveCh);
    c1->Divide(2, nActiveCh); 

    int row = 0;
    for (auto const& [chId, hist] : hQtotMap) {
        c1->cd(row * 2 + 1); // 좌측 패드: 파형 그리기
        if (ev0Time.find(chId) != ev0Time.end() && !ev0Time[chId].empty()) {
            auto grWave = new TGraph(ev0Time[chId].size(), ev0Time[chId].data(), ev0Drop[chId].data());
            grWave->SetTitle(Form("Event 0 Waveform (Ch%d);Time (ns);Voltage Drop (ADC)", chId));
            int colors[] = {kBlue, kRed, kGreen+2, kOrange+2};
            grWave->SetLineColor(colors[chId % 4]);
            grWave->Draw("AL");
        }

        c1->cd(row * 2 + 2); // 우측 패드: 스펙트럼 그리기
        gPad->SetLogy();     // 고에너지 꼬리(Tail) 대역을 보기 위해 Y축을 로그 스케일로 전환
        hist->Draw("HIST");

        row++;
    }
    c1->Update();
}
