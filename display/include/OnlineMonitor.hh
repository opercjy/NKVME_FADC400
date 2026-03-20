#ifndef OnlineMonitor_hh
#define OnlineMonitor_hh

#include <TGFrame.h>
#include <TRootEmbeddedCanvas.h>
#include <TServerSocket.h>
#include <TSocket.h>
#include <TTimer.h>
#include <TH1F.h>
#include <TCanvas.h>

class OnlineMonitor : public TGMainFrame {
public:
    OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~OnlineMonitor();
    
    // TTimer에 의해 50ms 마다 비동기적으로 호출되어 소켓 버퍼를 검사하는 슬롯
    void HandleTimer(); 

private:
    TRootEmbeddedCanvas *fEcanvas;
    TServerSocket       *fServerSocket;
    TSocket             *fSocket;
    TTimer              *fTimer;
    
    // 최대 4개 채널의 파형을 그리기 위한 히스토그램 배열
    TH1F                *fWaveform[4]; 
    
    ClassDefOverride(OnlineMonitor, 0)
};
#endif
