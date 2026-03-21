#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
#include <TMessage.h>
#include <TStyle.h>
#include <TSystem.h>
#include <iostream>
#include <algorithm> // for std::min

ClassImp(OnlineMonitor)

// -----------------------------------------------------------------------------
// SafeTimer 구현부
// -----------------------------------------------------------------------------
SafeTimer::SafeTimer(OnlineMonitor* monitor, Long_t ms) 
    : TTimer(ms, kTRUE), fMonitor(monitor) {}

Bool_t SafeTimer::Notify() {
    if (fMonitor) fMonitor->HandleTimer();
    Reset();
    return kTRUE;
}

// -----------------------------------------------------------------------------
// OnlineMonitor 구현부
// -----------------------------------------------------------------------------
OnlineMonitor::OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h) 
    : TGMainFrame(p, w, h), fServerSocket(nullptr), fSocket(nullptr)
{
    // [Pro-Tip] 스펙트럼 통계 정보(Entries, Mean, RMS 등) 표출 활성화
    gStyle->SetOptStat(111111); 

    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

    TCanvas *c = fEcanvas->GetCanvas();
    
    // [요구사항 1] 4x2 분할 (상단 4개 파형, 하단 4개 스펙트럼)
    c->Divide(4, 2); 

    for (int i = 0; i < 4; i++) {
        // [상단 1~4번 패드] Waveform
        fWaveform[i] = new TH1F(Form("hWave_%d", i), Form("Ch %d Waveform;Time (ns);ADC Count", i), 512, 0, 512 * 2.5);
        fWaveform[i]->SetDirectory(nullptr); // [Pro-Tip] 메모리 누수 원천 차단
        fWaveform[i]->SetStats(0);           // 파형은 통계창 숨김
        fWaveform[i]->SetLineColor(kAzure + 2);
        fWaveform[i]->SetLineWidth(2);

        // Baseline 렌더링용 TLine 초기화
        fBaseLine[i] = new TLine();
        fBaseLine[i]->SetLineColor(kRed);
        fBaseLine[i]->SetLineStyle(2); // 점선

        c->cd(i + 1);
        gPad->SetGrid();
        fWaveform[i]->Draw("HIST");

        // [하단 5~8번 패드] Charge Spectrum (X축: 0~50000 넉넉하게 설정)
        fSpectrum[i] = new TH1F(Form("hSpec_%d", i), Form("Ch %d Charge Spectrum;Charge (ADC);Counts", i), 500, 0, 50000);
        fSpectrum[i]->SetDirectory(nullptr); 
        fSpectrum[i]->SetLineColor(kRed + 1);
        fSpectrum[i]->SetFillColor(kRed - 9); // 면적 색상 채움
        fSpectrum[i]->SetFillStyle(1001);
        
        c->cd(i + 5);
        gPad->SetGrid();
        gPad->SetLogy(); // [Pro-Tip] 에너지 스펙트럼은 주로 Log Y 스케일에서 유의미함
        fSpectrum[i]->Draw("HIST");
    }

    SetWindowName("NoticeDAQ Real-time DQM (Port: 9090)");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) {
        std::cerr << "[ERROR] Port 9090 is already in use!" << std::endl;
    } else {
        std::cout << "[INFO] DQM Server listening on Port 9090..." << std::endl;
    }

    // 50ms(20Hz) 간격 SafeTimer 가동
    fTimer = new SafeTimer(this, 50);
    fTimer->TurnOn();
}

OnlineMonitor::~OnlineMonitor() {
    fTimer->TurnOff(); 
    delete fTimer;
    if (fSocket) { fSocket->Close(); delete fSocket; }
    if (fServerSocket) { fServerSocket->Close(); delete fServerSocket; }
    for (int i = 0; i < 4; i++) {
        delete fWaveform[i];
        delete fSpectrum[i];
        delete fBaseLine[i];
    }
    Cleanup();
}

void OnlineMonitor::HandleTimer() {
    // [필수 제약] GUI 먹통 방지
    gSystem->ProcessEvents(); 

    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        TSocket *tempSock = fServerSocket->Accept(0); 
        if (tempSock && tempSock != (TSocket*)-1) {
            fSocket = tempSock;
            std::cout << "[INFO] Frontend DAQ Connected to DQM." << std::endl;
        }
    }

    if (fSocket && fSocket->IsValid()) {
        TMessage *latestMess = nullptr;
        // [필수 제약] TCP 버퍼 밀림(Backpressure) 방지용 Drain 로직
        int readLimit = 100; 
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            if (fSocket->Recv(mess) <= 0) {
                std::cout << "[WARN] Frontend DAQ Disconnected." << std::endl;
                fSocket->Close(); delete fSocket; fSocket = nullptr;
                if (latestMess) delete latestMess;
                return;
            }
            if (latestMess) delete latestMess; // 이전 패킷 즉각 폐기
            latestMess = mess;
        }

        // 오직 가장 최신 이벤트(latestMess) 1개만 화면에 렌더링
        if (latestMess) {
            if (latestMess->What() == kMESS_OBJECT) {
                TObject *obj = latestMess->ReadObject(latestMess->GetClass());
                
                if (obj && obj->InheritsFrom(RawData::Class())) {
                    RawData *evtData = (RawData*)obj;
                    TCanvas *c = fEcanvas->GetCanvas();
                    int nCh = evtData->GetNChannels();
                    
                    for (int i = 0; i < nCh; i++) {
                        RawChannel *ch = evtData->GetChannel(i);
                        if (!ch) continue;
                        
                        // [Pro-Tip] 이빨 빠진 채널 하드웨어 인덱스 정확히 매핑 (Sparse Channel 방어)
                        int chId = ch->GetChId();
                        if (chId < 0 || chId >= 4) continue; 
                        
                        int nPts = ch->GetNPoints();
                        if (nPts == 0) continue;

                        if (fWaveform[chId]->GetNbinsX() != nPts) {
                            fWaveform[chId]->SetBins(nPts, 0, nPts * 2.5); // 2.5ns/sample
                        }
                        
                        // [요구사항 3] 파형(Waveform)은 매 이벤트마다 Reset
                        fWaveform[chId]->Reset();

                        // ---------------------------------------------------------
                        // [요구사항 2] On-the-fly Analysis (Baseline & Charge)
                        // ---------------------------------------------------------
                        double bsl_sum = 0;
                        int bsl_cnt = std::min(20, nPts); // 첫 20샘플 (안전 방어)
                        
                        for(int pt = 0; pt < bsl_cnt; pt++) {
                            bsl_sum += ch->GetSample(pt);
                        }
                        double bsl = (bsl_cnt > 0) ? (bsl_sum / bsl_cnt) : 0;

                        double charge = 0;
                        double minV = 99999, maxV = -99999;
                        
                        for (int pt = 0; pt < nPts; pt++) {
                            unsigned short val = ch->GetSample(pt);
                            fWaveform[chId]->SetBinContent(pt + 1, val);
                            
                            if (val > maxV) maxV = val;
                            if (val < minV) minV = val;

                            // Pulse Inversion: Negative 극성 펄스에 대해 양수 Drop만 적분
                            double drop = bsl - val;
                            if (drop > 0) {
                                charge += drop;
                            }
                        }
                        
                        // [요구사항 3] 에너지 스펙트럼은 Reset 없이 데이터 누적 (Fill)
                        if (charge > 0) {
                            fSpectrum[chId]->Fill(charge);
                        }

                        // 파형 Y축 오토 스케일링
                        double margin = (maxV - minV) * 0.1;
                        if (margin < 5) margin = 5;
                        fWaveform[chId]->GetYaxis()->SetRangeUser(minV - margin, maxV + margin);

                        // --- [렌더링 갱신] ---
                        c->cd(chId + 1);
                        fWaveform[chId]->Draw("HIST");
                        
                        // 시각적 Baseline 라인 업데이트
                        fBaseLine[chId]->SetX1(0);
                        fBaseLine[chId]->SetY1(bsl);
                        fBaseLine[chId]->SetX2(nPts * 2.5);
                        fBaseLine[chId]->SetY2(bsl);
                        fBaseLine[chId]->Draw();

                        c->cd(chId + 5);
                        fSpectrum[chId]->Draw("HIST");
                    }
                    
                    c->Modified();
                    c->Update();
                    
                    evtData->Clear("C"); 
                }
                if (obj) delete obj; 
            }
            delete latestMess; 
        }
    }
}

// -----------------------------------------------------------------------------
int main(int argc, char **argv) {
    TApplication app("MonitorApp", &argc, argv);
    // 4x2 분할 레이아웃이 답답하지 않도록 GUI 초기 해상도를 1600x800으로 넓게 설정
    new OnlineMonitor(gClient->GetRoot(), 1600, 800); 
    app.Run();
    return 0;
}