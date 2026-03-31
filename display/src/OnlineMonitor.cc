#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
#include <TGClient.h>      
#include <TMessage.h>
#include <TStyle.h>
#include <TSystem.h>
#include <iostream>
#include <algorithm> 
#include <sys/select.h> 

ClassImp(OnlineMonitor)

// 비동기 키보드 입력 감지 (터미널에서 'c' 입력 시 히스토그램 초기화용)
bool kbhit() {
    struct timeval tv = { 0L, 0L }; fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

SafeTimer::SafeTimer(OnlineMonitor* monitor, Long_t ms) : TTimer(ms, kFALSE), fMonitor(monitor) {}
Bool_t SafeTimer::Notify() { if (fMonitor) fMonitor->HandleTimer(); Reset(); return kTRUE; }

OnlineMonitor::OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h) 
    : TGMainFrame(p, w, h), fServerSocket(nullptr), fSocket(nullptr)
{
    gSystem->Load("libNKVME_FADC400.so"); gStyle->SetOptStat(111111); 
    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));
    TCanvas *c = fEcanvas->GetCanvas(); c->SetFillColor(kWhite);
    SetWindowName("NKVME_FADC400 Real-time DQM (Port: 9090)");
    MapSubwindows(); Resize(GetDefaultSize()); MapRaised(); 

    // 💡 [UX 강화] 직관적이고 아름다운 모니터 대시보드 출력
    std::cout << "\n\033[1;36m======================================================================\033[0m\n";
    std::cout << "\033[1;32m      NKVME_FADC400 - Live Online DQM Monitor\033[0m\n";
    std::cout << "\033[1;36m======================================================================\033[0m\n";
    
    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) {
        std::cout << "\033[1;31m[ERROR]\033[0m Port 9090 is already in use! Cannot start DQM Server." << std::endl;
    } else {
        std::cout << "  \033[1;33m[Listening]\033[0m Waiting for DAQ stream on port 9090...\n";
        std::cout << "  \033[1;37m[Commands]\033[0m  Press \033[1;36m'c' + ENTER\033[0m in this terminal to Clear Histograms.\n";
        std::cout << "              (Close the GUI window to Quit)\n";
    }
    std::cout << "\033[1;36m======================================================================\033[0m\n\n";

    fTimer = new SafeTimer(this, 50); fTimer->Start(50, kFALSE); 
}

OnlineMonitor::~OnlineMonitor() {
    fTimer->Stop(); delete fTimer;
    if (fSocket) { fSocket->Close(); delete fSocket; }
    if (fServerSocket) { fServerSocket->Close(); delete fServerSocket; }
    for(auto& pair : fWaveform) delete pair.second;
    for(auto& pair : fSpectrum) delete pair.second;
    for(auto& pair : fBaseLine) delete pair.second;
    fWaveform.clear(); fSpectrum.clear(); fBaseLine.clear();
    Cleanup();
    std::cout << "\033[1;35m[MONITOR]\033[0m DQM Shutdown complete.\n";
}

void OnlineMonitor::BuildCanvas(const std::vector<int>& activeChs) {
    int nCh = activeChs.size(); if (nCh == 0) return;
    TCanvas *c = fEcanvas->GetCanvas(); c->Clear(); c->Divide(nCh, 2); 

    for(auto& pair : fWaveform) delete pair.second;
    for(auto& pair : fSpectrum) delete pair.second;
    for(auto& pair : fBaseLine) delete pair.second;
    fWaveform.clear(); fSpectrum.clear(); fBaseLine.clear();

    for (int i = 0; i < nCh; i++) {
        int chId = activeChs[i];
        // 💡 [물리 스탠다드] Y축을 "전압 강하량(Signal Amplitude)"으로 변경
        fWaveform[chId] = new TH1F(Form("hWave_%d", chId), Form("Ch %d Signal (Inverted);Time (ns);Signal Amplitude (ADC)", chId), 512, 0, 512 * 2.5);
        fWaveform[chId]->SetDirectory(nullptr); fWaveform[chId]->SetStats(0); fWaveform[chId]->SetLineColor(kAzure + 2); fWaveform[chId]->SetLineWidth(2);
        fBaseLine[chId] = new TLine(); fBaseLine[chId]->SetLineColor(kRed); fBaseLine[chId]->SetLineStyle(2); fBaseLine[chId]->SetLineWidth(2);

        c->cd(i + 1); gPad->SetGrid(); fWaveform[chId]->Draw("HIST");
        
        fSpectrum[chId] = new TH1F(Form("hSpec_%d", chId), Form("Ch %d Charge Spectrum;Charge (ADC);Counts", chId), 500, 0, 50000);
        fSpectrum[chId]->SetDirectory(nullptr); fSpectrum[chId]->SetLineColor(kRed + 1); fSpectrum[chId]->SetFillColor(kRed - 9); fSpectrum[chId]->SetFillStyle(1001); fSpectrum[chId]->SetMinimum(0.5); 
        
        c->cd(i + 1 + nCh); gPad->SetGrid(); gPad->SetLogy(); fSpectrum[chId]->Draw("HIST");
    }
    c->Update(); fCurrentChs = activeChs;
}

void OnlineMonitor::HandleTimer() {
    // 💡 터미널에서 'c' 입력 시 히스토그램 리셋 로직
    if (kbhit()) {
        std::string cmd; std::cin >> cmd;
        if (cmd == "c" || cmd == "C") {
            for(auto& pair : fWaveform) if(pair.second) pair.second->Reset();
            for(auto& pair : fSpectrum) if(pair.second) pair.second->Reset();
            fEcanvas->GetCanvas()->Update();
            std::cout << "\033[1;36m[MONITOR]\033[0m Histograms cleared by user command." << std::endl;
        }
    }

    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        TSocket *tempSock = fServerSocket->Accept(0); 
        if (tempSock && tempSock != (TSocket*)-1) {
            fSocket = tempSock;
            for(auto& pair : fWaveform) if(pair.second) pair.second->Reset();
            for(auto& pair : fSpectrum) if(pair.second) pair.second->Reset();
            fEcanvas->GetCanvas()->Update();
            std::cout << "\033[1;32m[MONITOR]\033[0m DAQ stream connected!" << std::endl;
        }
    }

    if (fSocket && fSocket->IsValid()) {
        int readLimit = 2000; bool needUpdate = false;
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            if (fSocket->Recv(mess) <= 0) {
                fSocket->Close(); delete fSocket; fSocket = nullptr; fCurrentChs.clear(); 
                std::cout << "\033[1;33m[MONITOR]\033[0m DAQ stream disconnected. Waiting for new connection..." << std::endl;
                return;
            }

            if (mess->What() == kMESS_OBJECT) {
                TObject *obj = mess->ReadObject(RawData::Class());
                if (obj && obj->InheritsFrom(RawData::Class())) {
                    RawData *dumpData = (RawData*)obj;
                    
                    std::vector<int> incomingChs;
                    int nCh = dumpData->GetNChannels();
                    for (int i = 0; i < nCh; i++) {
                        RawChannel *ch = dumpData->GetChannel(i);
                        if (ch) incomingChs.push_back(ch->GetChId());
                    }
                    if (incomingChs != fCurrentChs && !incomingChs.empty()) BuildCanvas(incomingChs);

                    if (!fCurrentChs.empty() && nCh > 0) {
                        int nEvents = dumpData->GetChannel(0)->GetNumEvents();
                        int nPts = dumpData->GetChannel(0)->GetDataPoints();

                        for (int evtIdx = 0; evtIdx < nEvents; evtIdx++) {
                            for (int i = 0; i < nCh; i++) {
                                RawChannel *ch = dumpData->GetChannel(i);
                                int chId = ch->GetChId();
                                if (fWaveform.find(chId) == fWaveform.end()) continue; 
                                
                                // 동적 페데스탈 계산
                                double bsl_sum = 0;
                                int bsl_cnt = std::min(20, nPts); 
                                for(int pt = 0; pt < bsl_cnt; pt++) bsl_sum += ch->GetSample(evtIdx, pt);
                                double bsl = (bsl_cnt > 0) ? (bsl_sum / bsl_cnt) : 0;

                                double charge = 0; 
                                double minV = 99999, maxV = -99999;
                                
                                for (int pt = 0; pt < nPts; pt++) {
                                    unsigned short raw_val = ch->GetSample(evtIdx, pt);
                                    // 💡 [핵심] 파형을 위로 솟구치게(Positive-going) 반전 계산
                                    double inverted_sig = bsl - raw_val; 
                                    
                                    if (inverted_sig > maxV) maxV = inverted_sig;
                                    if (inverted_sig < minV) minV = inverted_sig;
                                    
                                    if (inverted_sig > 0) charge += inverted_sig;
                                    
                                    // 렌더링 부하를 줄이기 위해 마지막 이벤트 파형만 그리기 갱신
                                    if (evtIdx == nEvents - 1) fWaveform[chId]->SetBinContent(pt + 1, inverted_sig);
                                }
                                
                                if (charge > 0) fSpectrum[chId]->Fill(charge);

                                if (evtIdx == nEvents - 1) {
                                    if (fWaveform[chId]->GetNbinsX() != nPts) fWaveform[chId]->SetBins(nPts, 0, nPts * 2.5);
                                    
                                    // 반전되었으므로 베이스라인(페데스탈)의 기준은 항상 0입니다.
                                    fBaseLine[chId]->SetY1(0); fBaseLine[chId]->SetY2(0);
                                    
                                    double margin = (maxV - minV) * 0.1;
                                    fWaveform[chId]->GetYaxis()->SetRangeUser(minV - (margin < 5 ? 5 : margin), maxV + (margin < 5 ? 5 : margin));
                                }
                            }
                        }
                        needUpdate = true;
                    }
                    dumpData->Clear("C"); 
                }
                if (obj) delete obj; 
            }
            delete mess; 
        }

        if (needUpdate && !fCurrentChs.empty()) {
            TCanvas *c = fEcanvas->GetCanvas();
            for (size_t i = 0; i < fCurrentChs.size(); i++) {
                int chId = fCurrentChs[i];
                c->cd(i + 1); 
                fWaveform[chId]->Draw("HIST");
                fBaseLine[chId]->SetX2(fWaveform[chId]->GetNbinsX() * 2.5); 
                fBaseLine[chId]->Draw("SAME"); // 💡 파형 위에 베이스라인(0) 겹쳐 그리기
                
                c->cd(i + 1 + fCurrentChs.size()); 
                fSpectrum[chId]->Draw("HIST");
            }
            c->Modified(); c->Update();
        }
    }
}

int main(int argc, char **argv) {
    TApplication app("OnlineMonitorApp", &argc, argv);
    // GUI 초기화 및 메인 루프 진입
    OnlineMonitor* monitor = new OnlineMonitor(gClient->GetRoot(), 1200, 800);
    app.Run(); 
    return 0;
}
