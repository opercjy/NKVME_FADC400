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
    SetWindowName("NoticeDAQ Real-time DQM (Port: 9090)");
    MapSubwindows(); Resize(GetDefaultSize()); MapRaised(); 

    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) std::cout << "\033[1;31m[ERROR]\033[0m Port 9090 is already in use!" << std::endl;
    else std::cout << "\033[1;36m[INFO]\033[0m DQM Server listening on Port 9090..." << std::endl;

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
        fWaveform[chId] = new TH1F(Form("hWave_%d", chId), Form("Ch %d Waveform;Time (ns);ADC Count", chId), 512, 0, 512 * 2.5);
        fWaveform[chId]->SetDirectory(nullptr); fWaveform[chId]->SetStats(0); fWaveform[chId]->SetLineColor(kAzure + 2); fWaveform[chId]->SetLineWidth(2);
        fBaseLine[chId] = new TLine(); fBaseLine[chId]->SetLineColor(kRed); fBaseLine[chId]->SetLineStyle(2); 

        c->cd(i + 1); gPad->SetGrid(); fWaveform[chId]->Draw("HIST");
        fSpectrum[chId] = new TH1F(Form("hSpec_%d", chId), Form("Ch %d Charge Spectrum;Charge (ADC);Counts", chId), 500, 0, 50000);
        fSpectrum[chId]->SetDirectory(nullptr); fSpectrum[chId]->SetLineColor(kRed + 1); fSpectrum[chId]->SetFillColor(kRed - 9); fSpectrum[chId]->SetFillStyle(1001); fSpectrum[chId]->SetMinimum(0.5); 
        c->cd(i + 1 + nCh); gPad->SetGrid(); gPad->SetLogy(); fSpectrum[chId]->Draw("HIST");
    }
    c->Update(); fCurrentChs = activeChs;
}

void OnlineMonitor::HandleTimer() {
    if (kbhit()) {
        char cmd; std::cin >> cmd;
        if (cmd == 'c') {
            for(auto& pair : fWaveform) if(pair.second) pair.second->Reset();
            for(auto& pair : fSpectrum) if(pair.second) pair.second->Reset();
            fEcanvas->GetCanvas()->Update();
        }
    }

    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        TSocket *tempSock = fServerSocket->Accept(0); 
        if (tempSock && tempSock != (TSocket*)-1) {
            fSocket = tempSock;
            for(auto& pair : fWaveform) if(pair.second) pair.second->Reset();
            for(auto& pair : fSpectrum) if(pair.second) pair.second->Reset();
            fEcanvas->GetCanvas()->Update();
        }
    }

    if (fSocket && fSocket->IsValid()) {
        int readLimit = 2000; bool needUpdate = false;
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            if (fSocket->Recv(mess) <= 0) {
                fSocket->Close(); delete fSocket; fSocket = nullptr; fCurrentChs.clear(); return;
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

                        // 💡 [핵심] 수신된 바이너리 덩어리를 쪼개서 12-bit 디코딩을 수행
                        for (int evtIdx = 0; evtIdx < nEvents; evtIdx++) {
                            for (int i = 0; i < nCh; i++) {
                                RawChannel *ch = dumpData->GetChannel(i);
                                int chId = ch->GetChId();
                                if (fWaveform.find(chId) == fWaveform.end()) continue; 
                                
                                double bsl_sum = 0;
                                int bsl_cnt = std::min(20, nPts); 
                                for(int pt = 0; pt < bsl_cnt; pt++) bsl_sum += ch->GetSample(evtIdx, pt);
                                double bsl = (bsl_cnt > 0) ? (bsl_sum / bsl_cnt) : 0;

                                double charge = 0; double minV = 99999, maxV = -99999;
                                for (int pt = 0; pt < nPts; pt++) {
                                    unsigned short val = ch->GetSample(evtIdx, pt);
                                    if (val > maxV) maxV = val;
                                    if (val < minV) minV = val;
                                    double drop = bsl - val;
                                    if (drop > 0) charge += drop;
                                    
                                    // 💡 속도 최적화를 위해 파형 렌더링용 데이터는 블록의 '마지막 이벤트'만 저장합니다!
                                    if (evtIdx == nEvents - 1) fWaveform[chId]->SetBinContent(pt + 1, val);
                                }
                                
                                if (charge > 0) fSpectrum[chId]->Fill(charge);

                                if (evtIdx == nEvents - 1) {
                                    if (fWaveform[chId]->GetNbinsX() != nPts) fWaveform[chId]->SetBins(nPts, 0, nPts * 2.5);
                                    fBaseLine[chId]->SetY1(bsl); fBaseLine[chId]->SetY2(bsl);
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
                c->cd(i + 1); fWaveform[chId]->Draw("HIST");
                fBaseLine[chId]->SetX2(fWaveform[chId]->GetNbinsX() * 2.5); fBaseLine[chId]->Draw();
                c->cd(i + 1 + fCurrentChs.size()); fSpectrum[chId]->Draw("HIST");
            }
            c->Modified(); c->Update();
        }
    }
}

int main(int argc, char **argv) {
    TApplication app("OnlineMonitorApp", &argc, argv);
    OnlineMonitor* monitor = new OnlineMonitor(gClient->GetRoot(), 1200, 800);
    app.Run(); return 0;
}