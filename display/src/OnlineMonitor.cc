#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
#include <TGClient.h>      // 💡 [추가] gClient 사용을 위한 헤더
#include <TMessage.h>
#include <TStyle.h>
#include <TSystem.h>
#include <iostream>
#include <algorithm> 

ClassImp(OnlineMonitor)

SafeTimer::SafeTimer(OnlineMonitor* monitor, Long_t ms) 
    : TTimer(ms, kFALSE), fMonitor(monitor) {}

Bool_t SafeTimer::Notify() {
    if (fMonitor) fMonitor->HandleTimer();
    Reset();
    return kTRUE;
}

OnlineMonitor::OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h) 
    : TGMainFrame(p, w, h), fServerSocket(nullptr), fSocket(nullptr)
{
    gSystem->Load("libNKVME_FADC400.so");
    gStyle->SetOptStat(111111); 

    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

    TCanvas *c = fEcanvas->GetCanvas();
    c->SetFillColor(kWhite);

    SetWindowName("NoticeDAQ Real-time DQM (Port: 9090)");
    MapSubwindows(); Resize(GetDefaultSize()); MapRaised(); 

    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) {
        std::cout << "[ERROR] Port 9090 is already in use!" << std::endl;
    } else {
        std::cout << "[INFO] DQM Server listening on Port 9090..." << std::endl;
    }

    fTimer = new SafeTimer(this, 50);
    fTimer->Start(50, kFALSE); 
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
    int nCh = activeChs.size();
    if (nCh == 0) return;

    TCanvas *c = fEcanvas->GetCanvas();
    c->Clear();
    c->Divide(nCh, 2); 

    for(auto& pair : fWaveform) delete pair.second;
    for(auto& pair : fSpectrum) delete pair.second;
    for(auto& pair : fBaseLine) delete pair.second;
    fWaveform.clear(); fSpectrum.clear(); fBaseLine.clear();

    for (int i = 0; i < nCh; i++) {
        int chId = activeChs[i];

        fWaveform[chId] = new TH1F(Form("hWave_%d", chId), Form("Ch %d Waveform;Time (ns);ADC Count", chId), 512, 0, 512 * 2.5);
        fWaveform[chId]->SetDirectory(nullptr); 
        fWaveform[chId]->SetStats(0);           
        fWaveform[chId]->SetLineColor(kAzure + 2);
        fWaveform[chId]->SetLineWidth(2);

        fBaseLine[chId] = new TLine();
        fBaseLine[chId]->SetLineColor(kRed); fBaseLine[chId]->SetLineStyle(2); 

        c->cd(i + 1); gPad->SetGrid(); fWaveform[chId]->Draw("HIST");

        fSpectrum[chId] = new TH1F(Form("hSpec_%d", chId), Form("Ch %d Charge Spectrum;Charge (ADC);Counts", chId), 500, 0, 50000);
        fSpectrum[chId]->SetDirectory(nullptr); 
        fSpectrum[chId]->SetLineColor(kRed + 1);
        fSpectrum[chId]->SetFillColor(kRed - 9); 
        fSpectrum[chId]->SetFillStyle(1001);
        fSpectrum[chId]->SetMinimum(0.5); 
        
        c->cd(i + 1 + nCh); gPad->SetGrid(); gPad->SetLogy(); fSpectrum[chId]->Draw("HIST");
    }
    c->Update();
    fCurrentChs = activeChs;
    std::cout << "[DQM] Canvas Dynamically Rebuilt for " << nCh << " Channels." << std::endl;
}

void OnlineMonitor::HandleTimer() {
    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        TSocket *tempSock = fServerSocket->Accept(0); 
        if (tempSock && tempSock != (TSocket*)-1) {
            fSocket = tempSock;
            std::cout << "[SUCCESS] Frontend DAQ Connected to DQM!" << std::endl;
        }
    }

    if (fSocket && fSocket->IsValid()) {
        int readLimit = 2000; 
        bool needUpdate = false;
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            if (fSocket->Recv(mess) <= 0) {
                std::cout << "[WARN] Frontend DAQ Disconnected." << std::endl;
                fSocket->Close(); delete fSocket; fSocket = nullptr;
                fCurrentChs.clear();
                return;
            }

            if (mess->What() == kMESS_OBJECT) {
                TObject *obj = mess->ReadObject(RawData::Class());
                
                if (obj && obj->InheritsFrom(RawData::Class())) {
                    RawData *evtData = (RawData*)obj;
                    
                    std::vector<int> incomingChs;
                    int nCh = evtData->GetNChannels();
                    for (int i = 0; i < nCh; i++) {
                        RawChannel *ch = evtData->GetChannel(i);
                        if (ch) incomingChs.push_back(ch->GetChId());
                    }
                    
                    if (incomingChs != fCurrentChs && !incomingChs.empty()) {
                        BuildCanvas(incomingChs);
                    }

                    if (!fCurrentChs.empty()) {
                        for (int i = 0; i < nCh; i++) {
                            RawChannel *ch = evtData->GetChannel(i);
                            if (!ch) continue;
                            
                            int chId = ch->GetChId();
                            if (fWaveform.find(chId) == fWaveform.end()) continue; 
                            
                            int nPts = ch->GetNPoints();
                            if (nPts == 0) continue;

                            if (fWaveform[chId]->GetNbinsX() != nPts) {
                                fWaveform[chId]->SetBins(nPts, 0, nPts * 2.5); 
                            }
                            
                            fWaveform[chId]->Reset();

                            double bsl_sum = 0;
                            int bsl_cnt = std::min(20, nPts); 
                            for(int pt = 0; pt < bsl_cnt; pt++) bsl_sum += ch->GetSample(pt);
                            double bsl = (bsl_cnt > 0) ? (bsl_sum / bsl_cnt) : 0;

                            double charge = 0;
                            for (int pt = 0; pt < nPts; pt++) {
                                unsigned short val = ch->GetSample(pt);
                                fWaveform[chId]->SetBinContent(pt + 1, val);
                                double drop = bsl - val;
                                if (drop > 0) charge += drop;
                            }
                            
                            if (charge > 0) fSpectrum[chId]->Fill(charge);
                            fBaseLine[chId]->SetY1(bsl); fBaseLine[chId]->SetY2(bsl);
                            
                            needUpdate = true;
                        }
                    }
                    evtData->Clear("C"); 
                }
                if (obj) delete obj; 
            }
            delete mess; 
        }

        // 💡 [수정] Warning 해결을 위해 size_t 사용
        if (needUpdate && !fCurrentChs.empty()) {
            TCanvas *c = fEcanvas->GetCanvas();
            for (size_t i = 0; i < fCurrentChs.size(); i++) {
                int chId = fCurrentChs[i];
                c->cd(i + 1);
                double maxV = fWaveform[chId]->GetMaximum();
                double minV = fWaveform[chId]->GetMinimum();
                double margin = (maxV - minV) * 0.1;
                fWaveform[chId]->GetYaxis()->SetRangeUser(minV - (margin < 5 ? 5 : margin), maxV + (margin < 5 ? 5 : margin));
                
                fWaveform[chId]->Draw("HIST");
                fBaseLine[chId]->SetX2(fWaveform[chId]->GetNbinsX() * 2.5);
                fBaseLine[chId]->Draw();
                
                c->cd(i + 1 + fCurrentChs.size());
                fSpectrum[chId]->Draw("HIST");
            }
            c->Modified();
            c->Update();
        }
    }
}

// ---------------------------------------------------------
// 💡 [누락되었던 부분] 실행 파일의 진입점(Entry Point)
// ---------------------------------------------------------
int main(int argc, char **argv) {
    TApplication app("OnlineMonitorApp", &argc, argv);
    
    // 메인 윈도우 생성 (가로 1200, 세로 800)
    OnlineMonitor* monitor = new OnlineMonitor(gClient->GetRoot(), 1200, 800);
    
    app.Run();
    return 0;
}