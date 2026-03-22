#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
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
    // 🔥 [버그 픽스] 과거의 libObjects.so 대신 통합 공유 라이브러리를 로드하도록 수정!
    gSystem->Load("libNKVME_FADC400.so");
    gStyle->SetOptStat(111111); 

    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

    TCanvas *c = fEcanvas->GetCanvas();
    c->SetFillColor(kWhite);

    SetWindowName("NoticeDAQ Real-time DQM (Port: 9090)");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapRaised(); 

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
    fTimer->Stop(); 
    delete fTimer;
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
        fBaseLine[chId]->SetLineColor(kRed);
        fBaseLine[chId]->SetLineStyle(2); 

        c->cd(i + 1);
        gPad->SetGrid();
        fWaveform[chId]->Draw("HIST");

        fSpectrum[chId] = new TH1F(Form("hSpec_%d", chId), Form("Ch %d Charge Spectrum;Charge (ADC);Counts", chId), 500, 0, 50000);
        fSpectrum[chId]->SetDirectory(nullptr); 
        fSpectrum[chId]->SetLineColor(kRed + 1);
        fSpectrum[chId]->SetFillColor(kRed - 9); 
        fSpectrum[chId]->SetFillStyle(1001);
        fSpectrum[chId]->SetMinimum(0.5); 
        
        c->cd(i + 1 + nCh); 
        gPad->SetGrid();
        gPad->SetLogy(); 
        fSpectrum[chId]->Draw("HIST");
    }
    c->Update();
    fCurrentChs = activeChs;
    std::cout << "[DQM] Canvas Dynamically Rebuilt for " << nCh << " Channels." << std::endl;
}

void OnlineMonitor::HandleTimer() {
    // [핵심 픽스 1] gSystem->ProcessEvents() 삭제 (창 크기 조절 시 데드락 방지)
    
    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        TSocket *tempSock = fServerSocket->Accept(0); 
        if (tempSock && tempSock != (TSocket*)-1) {
            fSocket = tempSock;
            std::cout << "[SUCCESS] Frontend DAQ Connected to DQM!" << std::endl;
        }
    }

    if (fSocket && fSocket->IsValid()) {
        TMessage *latestMess = nullptr;
        int readLimit = 50; 
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            if (fSocket->Recv(mess) <= 0) {
                std::cout << "[WARN] Frontend DAQ Disconnected." << std::endl;
                fSocket->Close(); delete fSocket; fSocket = nullptr;
                if (latestMess) delete latestMess;
                fCurrentChs.clear();
                return;
            }
            if (latestMess) delete latestMess; 
            latestMess = mess;
        }

        if (latestMess) {
            if (latestMess->What() == kMESS_OBJECT) {
                TObject *obj = latestMess->ReadObject(RawData::Class());
                
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
                        TCanvas *c = fEcanvas->GetCanvas();
                        bool needUpdate = false;
                        
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

                                double drop = bsl - val;
                                if (drop > 0) charge += drop;
                            }
                            
                            if (charge > 0) fSpectrum[chId]->Fill(charge);

                            double margin = (maxV - minV) * 0.1;
                            if (margin < 5) margin = 5;
                            fWaveform[chId]->GetYaxis()->SetRangeUser(minV - margin, maxV + margin);

                            c->cd(i + 1);
                            fWaveform[chId]->Draw("HIST");
                            
                            fBaseLine[chId]->SetX1(0);
                            fBaseLine[chId]->SetY1(bsl);
                            fBaseLine[chId]->SetX2(nPts * 2.5);
                            fBaseLine[chId]->SetY2(bsl);
                            fBaseLine[chId]->Draw();

                            c->cd(i + 1 + nCh);
                            fSpectrum[chId]->Draw("HIST");

                            needUpdate = true;
                        }
                        
                        if (needUpdate) {
                            c->Modified();
                            c->Update();
                            // [핵심 픽스 2] ProcessEvents 삭제로 무한 대기 록(Lock) 원천 차단
                        }
                    }
                    evtData->Clear("C"); 
                }
                if (obj) delete obj; 
            }
            delete latestMess; 
        }
    }
}

int main(int argc, char **argv) {
    TApplication app("MonitorApp", &argc, argv);
    new OnlineMonitor(gClient->GetRoot(), 1200, 800); 
    app.Run();
    return 0;
}