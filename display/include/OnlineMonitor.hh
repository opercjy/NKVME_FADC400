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

class OnlineMonitor;

// =====================================================================
// [필수 제약 조건] ROOT Cling JIT 에러 방지 및 안전한 비동기 타이머
// =====================================================================
class SafeTimer : public TTimer {
public:
    SafeTimer(OnlineMonitor* monitor, Long_t ms);
    virtual Bool_t Notify() override;
private:
    OnlineMonitor* fMonitor;
};

// =====================================================================
// 메인 GUI DQM 클래스
// =====================================================================
class OnlineMonitor : public TGMainFrame {
public:
    OnlineMonitor(const TGWindow *p, UInt_t w, UInt_t h);
    virtual ~OnlineMonitor();
    
    void HandleTimer(); 

private:
    TRootEmbeddedCanvas *fEcanvas;
    TServerSocket       *fServerSocket;
    TSocket             *fSocket;
    SafeTimer           *fTimer;
    
    // [요구사항 1] 4채널 상단/하단 분할 렌더링 배열
    TH1F                *fWaveform[4]; // 1~4 패드 (파형)
    TH1F                *fSpectrum[4]; // 5~8 패드 (누적 스펙트럼)

    // [Pro-Tip] 시각적 Baseline 인디케이터 (메모리 누수 방지용 멤버 변수)
    TLine               *fBaseLine[4]; 
    
    ClassDefOverride(OnlineMonitor, 0)
};

#endif