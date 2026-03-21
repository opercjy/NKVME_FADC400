#include <TGClient.h>
#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
#include <TMessage.h>
#include <TStyle.h>
#include <iostream>

ClassImp(OnlineMonitor)

OnlineMonitor::OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h) 
    : TGMainFrame(p, w, h), fSocket(nullptr) 
{
    gStyle->SetOptStat(0); // 통계 박스 숨김

    // 1. 캔버스 프레임 배치
    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

    TCanvas *c = fEcanvas->GetCanvas();
    c->Divide(2, 2); // 4채널 동시 표출을 위한 2x2 분할

    // 2. 파형 드로잉용 히스토그램 껍데기 미리 생성 (매 이벤트마다 new 금지)
    for (int i = 0; i < 4; i++) {
        fWaveform[i] = new TH1F(Form("hWave_%d", i), Form("FADC Channel %d;Sample Time (ns);ADC Count", i), 512, 0, 512);
        fWaveform[i]->SetLineColor(kBlue + i); // 채널별로 색상 구분
        c->cd(i + 1);
        fWaveform[i]->Draw("HIST");
    }

    SetWindowName("NoticeDAQ Online Monitor (Port: 9090)");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapWindow();

    // 3. 서버 소켓 오픈
    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) {
        std::cerr << "[ERROR] Port 9090 is already in use!" << std::endl;
    } else {
        std::cout << "[INFO] Display Server listening on Port 9090..." << std::endl;
    }

    // 4. GUI 멈춤(Blocking)을 방지하기 위한 비동기 타이머 (50ms)
    fTimer = new TTimer();
    fTimer->Connect("Timeout()", "OnlineMonitor", this, "HandleTimer()");
    fTimer->Start(50, kFALSE);
}

OnlineMonitor::~OnlineMonitor() {
    fTimer->Stop(); 
    delete fTimer;
    if (fSocket) { fSocket->Close(); delete fSocket; }
    if (fServerSocket) { fServerSocket->Close(); delete fServerSocket; }
    for (int i = 0; i < 4; i++) delete fWaveform[i];
    Cleanup();
}

void OnlineMonitor::HandleTimer() {
    // [해결] Select()를 통한 완벽한 Non-blocking 클라이언트 접속 수락
    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        if (fServerSocket->Select(TSocket::kRead, 0) > 0) {
            TSocket *tempSock = fServerSocket->Accept(); 
            if (tempSock && tempSock != (TSocket*)-1) {
                fSocket = tempSock;
                std::cout << "[INFO] Frontend DAQ Connected." << std::endl;
            }
        }
    }

    if (fSocket && fSocket->IsValid()) {
        // 소켓에 읽을 데이터가 있는지 논블로킹(Non-blocking)으로 확인
        if (fSocket->Select(TSocket::kRead, 0) > 0) {
            TMessage *mess = nullptr;
            
            // 프론트엔드 연결 끊김 감지
            if (fSocket->Recv(mess) <= 0) {
                std::cout << "[WARN] Frontend DAQ Disconnected." << std::endl;
                fSocket->Close(); 
                delete fSocket; 
                fSocket = nullptr;
                return;
            }

            if (mess && mess->What() == kMESS_OBJECT) {
                TObject *obj = mess->ReadObject(mess->GetClass());
                if (obj && obj->InheritsFrom(RawData::Class())) {
                    RawData *evtData = (RawData*)obj;
                    TCanvas *c = fEcanvas->GetCanvas();
                    
                    int nCh = evtData->GetNChannels();
                    
                    // 수신된 채널 개수만큼 반복하며 그리기 (최대 4채널)
                    for (int i = 0; i < nCh && i < 4; i++) {
                        RawChannel *ch = evtData->GetChannel(i);
                        if (!ch) continue;
                        
                        int nPts = ch->GetNPoints();
                        // 데이터 길이에 맞춰 X축 동적 리사이징 (2.5ns per point)
                        if (fWaveform[i]->GetNbinsX() != nPts) {
                            fWaveform[i]->SetBins(nPts, 0, nPts * 2.5);
                        }
                        
                        fWaveform[i]->Reset(); // 캔버스 잔상 제거
                        
                        // [개선] 파형 크기에 맞게 Y축 Auto-Scaling
                        double minVal = 99999, maxVal = -99999;
                        for (int pt = 0; pt < nPts; pt++) {
                            unsigned short val = ch->GetSample(pt);
                            fWaveform[i]->SetBinContent(pt + 1, val);
                            if (val > maxVal) maxVal = val;
                            if (val < minVal) minVal = val;
                        }
                        
                        double margin = (maxVal - minVal) * 0.1;
                        if (margin < 10) margin = 10; // 최소 마진 확보
                        fWaveform[i]->GetYaxis()->SetRangeUser(minVal - margin, maxVal + margin);

                        c->cd(i + 1);
                        fWaveform[i]->Draw("HIST");
                    }
                    c->Modified(); 
                    c->Update();
                    
                    // [핵심] 수신된 힙 메모리 내부 객체들을 깔끔하게 비움 (메모리 누수 원천 차단)
                    evtData->Clear("C"); 
                }
                if (obj) delete obj; // 역직렬화된 껍데기 객체 강제 소멸
            }
            if (mess) delete mess; 
        }
    }
}

int main(int argc, char **argv) {
    TApplication app("MonitorApp", &argc, argv);
    new OnlineMonitor(gClient->GetRoot(), 1200, 800);
    app.Run();
    return 0;
}
