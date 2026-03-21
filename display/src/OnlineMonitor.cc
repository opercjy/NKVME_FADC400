#include "OnlineMonitor.hh"
#include "RawData.hh"
#include "RawChannel.hh"

#include <TApplication.h>
#include <TMessage.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TTimer.h>
#include <iostream>

ClassImp(OnlineMonitor)

class SafeTimer : public TTimer {
private:
    OnlineMonitor* fMon;
public:
    SafeTimer(OnlineMonitor* mon) : TTimer(50, kFALSE), fMon(mon) {}
    Bool_t Notify() override {
        fMon->HandleTimer(); 
        return kTRUE;
    }
};

OnlineMonitor::OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h) 
    : TGMainFrame(p, w, h), fSocket(nullptr) 
{
    // RawData 딕셔너리 로드 보장
    gSystem->Load("libObjects.so");

    gStyle->SetOptStat(0); 

    fEcanvas = new TRootEmbeddedCanvas("Ecanvas", this, w, h);
    AddFrame(fEcanvas, new TGLayoutHints(kLHintsExpandX | kLHintsExpandY, 10, 10, 10, 10));

    TCanvas *c = fEcanvas->GetCanvas();
    c->Divide(2, 2); 

    for (int i = 0; i < 4; i++) {
        fWaveform[i] = new TH1F(Form("hWave_%d", i), Form("FADC Channel %d;Sample Time (ns);ADC Count", i), 512, 0, 512 * 2.5);
        fWaveform[i]->SetLineColor(kBlue + i); 
        fWaveform[i]->SetLineWidth(2);
        c->cd(i + 1);
        fWaveform[i]->Draw("HIST");
    }

    SetWindowName("NoticeDAQ Online Monitor (Port: 9090)");
    MapSubwindows();
    Resize(GetDefaultSize());
    MapRaised(); 

    fServerSocket = new TServerSocket(9090, kTRUE);
    if (!fServerSocket->IsValid()) {
        std::cout << "[ERROR] Port 9090 is blocked or already in use!" << std::endl;
    } else {
        std::cout << "[INFO] Display Server ready. Listening on Port 9090..." << std::endl;
    }

    fTimer = new SafeTimer(this);
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
    if (!fSocket && fServerSocket && fServerSocket->IsValid()) {
        if (fServerSocket->Select(TSocket::kRead, 0) > 0) {
            TSocket *tempSock = fServerSocket->Accept(); 
            if (tempSock && tempSock != (TSocket*)-1) {
                fSocket = tempSock;
                std::cout << "[SUCCESS] Frontend DAQ Connected to Monitor!" << std::endl;
            }
        }
    }

    if (fSocket && fSocket->IsValid()) {
        RawData* latestEvt = nullptr;
        int readLimit = 50; 
        
        while (fSocket->Select(TSocket::kRead, 0) > 0 && readLimit-- > 0) {
            TMessage *mess = nullptr;
            
            if (fSocket->Recv(mess) <= 0) {
                std::cout << "[WARN] Frontend DAQ Disconnected." << std::endl;
                fSocket->Close(); 
                delete fSocket; 
                fSocket = nullptr;
                if (latestEvt) delete latestEvt;
                return;
            }

            if (mess && mess->What() == kMESS_OBJECT) {
                TObject *obj = mess->ReadObject(RawData::Class());
                if (obj) {
                    if (latestEvt) delete latestEvt; 
                    latestEvt = (RawData*)obj; 
                }
            }
            if (mess) delete mess; 
        }

        if (latestEvt) {
            TCanvas *c = fEcanvas->GetCanvas();
            int nCh = latestEvt->GetNChannels();
            bool needUpdate = false;
            
            for (int i = 0; i < nCh; i++) {
                RawChannel *ch = latestEvt->GetChannel(i);
                if (!ch) continue;
                
                int chId = ch->GetChId();
                if (chId < 0 || chId >= 4) continue; 

                int nPts = ch->GetNPoints();
                if (nPts <= 0) continue;
                
                if (fWaveform[chId]->GetNbinsX() != nPts) {
                    fWaveform[chId]->SetBins(nPts, 0, nPts * 2.5);
                }
                
                fWaveform[chId]->Reset(); 
                
                double minVal = 99999, maxVal = -99999;
                for (int pt = 0; pt < nPts; pt++) {
                    unsigned short val = ch->GetSample(pt);
                    fWaveform[chId]->SetBinContent(pt + 1, val);
                    if (val > maxVal) maxVal = val;
                    if (val < minVal) minVal = val;
                }
                
                if (maxVal <= minVal) { maxVal = minVal + 10; minVal = minVal - 10; }
                
                double margin = (maxVal - minVal) * 0.1;
                if (margin < 5) margin = 5; 
                
                fWaveform[chId]->SetMinimum(minVal - margin);
                fWaveform[chId]->SetMaximum(maxVal + margin);

                c->cd(chId + 1);
                fWaveform[chId]->Draw("HIST"); 
                gPad->Modified(); 
                needUpdate = true;
            }
            
            if (needUpdate) {
                c->Modified(); 
                c->Update();
                gSystem->ProcessEvents(); 
            }
            
            delete latestEvt; 
        }
    }
}

int main(int argc, char **argv) {
    TApplication app("MonitorApp", &argc, argv);
    new OnlineMonitor(gClient->GetRoot(), 1200, 800);
    app.Run();
    return 0;
}