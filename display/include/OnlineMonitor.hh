#ifndef OnlineMonitor_hh
#define OnlineMonitor_hh

#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TServerSocket.h>
#include <TSocket.h>
#include <TTimer.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TLine.h>
#include <map>
#include <vector>

class OnlineMonitor;

class SafeTimer : public TTimer {
public:
    SafeTimer(OnlineMonitor* monitor, Long_t ms);
    virtual Bool_t Notify() override;
private:
    OnlineMonitor* fMonitor;
};

class OnlineMonitor : public TGMainFrame {
public:
    OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~OnlineMonitor();
    
    void HandleTimer(); 

private:
    void BuildCanvas(const std::vector<int>& activeChs); // 동적 캔버스 빌더

    TRootEmbeddedCanvas *fEcanvas;
    TServerSocket       *fServerSocket;
    TSocket             *fSocket;
    SafeTimer           *fTimer;
    
    std::vector<int>      fCurrentChs; // 현재 켜진 채널 목록 추적
    std::map<int, TH1F*>  fWaveform; 
    std::map<int, TH1F*>  fSpectrum; 
    std::map<int, TLine*> fBaseLine; 
    
    ClassDefOverride(OnlineMonitor, 0)
};

#endif