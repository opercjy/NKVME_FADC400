#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono> // [수정] 모니터 송신용 독립 타이머를 위한 헤더

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TSocket.h"
#include "TMessage.h"
#include "TStopwatch.h"

#include "ThreadSafeQueue.hh"
#include "ConfigParser.hh"
#include "ELog.hh"
#include "RunInfo.hh"
#include "RawData.hh"

#include "Notice6UVMEROOT.h"
#include "NoticeNFADC400ROOT.h"

std::atomic<bool> g_isRunning(true);
ThreadSafeQueue<RawData*> g_dataQueue;
ThreadSafeQueue<RawData*> g_freeQueue; // 객체 재사용을 위한 Object Pool

// Ctrl+C 시그널 시 하드웨어 락 없이 안전하게 종료
void SigIntHandler(int /*signum*/) {
    ELog::Print(ELog::WARNING, "\nInterrupt signal (Ctrl+C) received! Shutting down gracefully...");
    g_isRunning = false;
    g_dataQueue.Stop(); 
}

// [Consumer 스레드] 디스크 쓰기 및 네트워크 브로드캐스팅 전담
void ConsumerWorker(const char* outFileName, bool useDisplay) {
    TFile* outFile = new TFile(outFileName, "UPDATE");
    if(!outFile || outFile->IsZombie()) {
        ELog::Print(ELog::FATAL, Form("Cannot update ROOT file: %s", outFileName));
        g_isRunning = false;
        return;
    }

    TTree* tree = new TTree("FADC", "FADC Raw Data Tree");
    RawData* treeEvtData = new RawData(); 
    tree->Branch("RawData", &treeEvtData);

    TSocket* socket = nullptr;
    if (useDisplay) {
        socket = new TSocket("localhost", 9090);
        if (!socket->IsValid()) {
            ELog::Print(ELog::WARNING, "Display Server offline. No network broadcast.");
            delete socket; socket = nullptr;
        } else {
            ELog::Print(ELog::INFO, "Connected to Display Server (localhost:9090)");
        }
    }

    int nWrite = 0;
    RawData* popData = nullptr;
    
    // [수정] TStopwatch 간섭을 피하기 위한 절대 시간(chrono) 타이머 사용
    auto lastNetTime = std::chrono::steady_clock::now();

    while (g_dataQueue.WaitAndPop(popData)) {
        if (popData) {
            RawData* temp = treeEvtData;
            treeEvtData = popData; // Branch 포인터 스왑
            
            tree->Fill();
            nWrite++;

            auto now = std::chrono::steady_clock::now();
            // TCP 버퍼 폭발 방지 (50ms마다 한 번만 소켓 송신)
            if (socket && socket->IsValid() && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNetTime).count() > 50) {
                TMessage mess(kMESS_OBJECT);
                mess.WriteObject(treeEvtData);
                socket->Send(mess);
                lastNetTime = now;
            }

            // 다 쓴 객체는 힙에 반환하지 않고 Free Queue로 보내 재활용
            temp->Clear("C"); 
            g_freeQueue.Push(temp); 
        }
    }

    tree->AutoSave();
    outFile->Write();
    outFile->Close();
    delete outFile;

    if (treeEvtData) { treeEvtData->Clear("C"); delete treeEvtData; }
    if (socket) { socket->Close(); delete socket; }
    ELog::Print(ELog::INFO, Form("Consumer Thread: Saved %d events successfully.", nWrite));
}

int main(int argc, char ** argv) {
    ROOT::EnableThreadSafety(); 
    std::signal(SIGINT, SigIntHandler);
    std::signal(SIGTERM, SigIntHandler);

    TString configFile = "";
    TString outFile = "data.root";
    int presetNEvt = 0;
    
    int opt;
    while((opt = getopt(argc, argv, "f:n:o:")) != -1) {
        switch(opt) {
            case 'f': configFile = optarg; break;
            case 'n': presetNEvt = atoi(optarg); break;
            case 'o': outFile = optarg; break;
        }
    }

    if(configFile == "") {
        ELog::Print(ELog::ERROR, "Usage: frontend -f <ticket.config> [-o <out.root>] [-n <events>]");
        return 1;
    }

    RunInfo* runInfo = new RunInfo();
    if (!ConfigParser::Parse(configFile.Data(), runInfo)) return 1;
    runInfo->Print();

    NK6UVME vme;
    NKNFADC400 fadc;

    if (vme.VMEopen() < 0) {
        ELog::Print(ELog::FATAL, "Failed to open USB-VME Controller.");
        return 1;
    }

    // 하드웨어 통신 및 파라미터 전송 로직
    int nbd = runInfo->GetNFadcBD();
    for (int i = 0; i < nbd; i++) {
        FadcBD* bd = runInfo->GetFadcBD(i);
        unsigned long mid = bd->MID();
        
        fadc.NFADC400open(mid);
        
        // 1. 하드웨어 생존 및 MID 일치 검증
        unsigned long stat = fadc.NFADC400read_STAT(mid);
        if (stat == 0xFFFFFFFF) {
            ELog::Print(ELog::FATAL, "==============================================================");
            ELog::Print(ELog::FATAL, Form(" [FATAL ERROR] FADC Board (MID: %lu) Not Found or Mismatch!", mid));
            ELog::Print(ELog::FATAL, "==============================================================");
            ELog::Print(ELog::FATAL, "  1. Check if the VME crate is powered ON.");
            ELog::Print(ELog::FATAL, "  2. Check if the Rotary Switch matches the config file MID.");
            ELog::Print(ELog::FATAL, "  3. Check USB/Optical link connections.");
            ELog::Print(ELog::FATAL, "==============================================================");
            vme.VMEclose();
            return 1;
        }
        
        ELog::Print(ELog::INFO, Form("Board MID %lu connected. Writing parameters to Hardware...", mid));
        
        fadc.NFADC400reset(mid);
        
        // 2. Global Parameters (공통 설정)
        fadc.NFADC400write_RL(mid, bd->RL());
        fadc.NFADC400write_TLT(mid, bd->TLT());
        fadc.NFADC400write_TOW(mid, bd->TOW());
        if (bd->DCE()) fadc.NFADC400enable_DCE(mid);
        else fadc.NFADC400disable_DCE(mid);
        
        // 3. Channel Parameters (각 채널별 개별 설정 전송 루프)
        for (int ch = 0; ch < bd->NCHANNEL(); ch++) {
            int cid = bd->CID(ch) + 1; // VME 라이브러리 규격에 맞춰 +1 (1~4채널)
            
            fadc.NFADC400write_THR(mid, cid, bd->THR(ch));
            fadc.NFADC400write_POL(mid, cid, bd->POL(ch));
            fadc.NFADC400write_DACOFF(mid, cid, bd->DACOFF(ch));
            fadc.NFADC400write_DLY(mid, cid, bd->DLY(ch));
            fadc.NFADC400write_DACGAIN(mid, cid, bd->DACGAIN(ch));
            fadc.NFADC400write_DT(mid, cid, bd->DT(ch));
            fadc.NFADC400write_CW(mid, cid, bd->CW(ch));
            
            // TM(Trigger Mode)의 ew, en 분리 전송
            int tm_val = bd->TM(ch);
            int ew = (tm_val & 0x2) >> 1;
            int en = (tm_val & 0x1);
            fadc.NFADC400write_TM(mid, cid, ew, en);
            
            fadc.NFADC400write_PCT(mid, cid, bd->PCT(ch));
            fadc.NFADC400write_PCI(mid, cid, bd->PCI(ch));
            fadc.NFADC400write_PWT(mid, cid, bd->PWT(ch));
        }
    }

    TFile* hfile = new TFile(outFile.Data(), "RECREATE");
    runInfo->Write();
    hfile->Close();
    delete hfile;

    // 하프 버퍼당 들어있는 정확한 이벤트 수(hevt) 계산
    int recordLength = runInfo->GetFadcBD(0)->RL(); 
    int dataPoints = recordLength * 128;            
    int hevt = (recordLength > 4) ? (4096 / recordLength) : 512;
    
    unsigned short* raw_buffer = new unsigned short[dataPoints];

    std::thread consumerTh(ConsumerWorker, outFile.Data(), true);
    ELog::Print(ELog::INFO, "DAQ Running. Press Ctrl+C to stop.");

    int bufnum = 0;
    for (int i = 0; i < nbd; i++) {
        fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
    }

    int nevt = 0;
    TStopwatch sw; sw.Start();
    double lastTime = 0.0;
    int lastEvt = 0;

    while (g_isRunning) {
        // OOM 방어: 큐가 비정상적으로 팽창하면 잠시 대기
        if (g_dataQueue.Size() > 20000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        unsigned long primary_mid = runInfo->GetFadcBD(0)->MID();
        int isFill = 0;
        // 하드웨어 버퍼 폴링 (데드타임이 발생하는 유일한 지점)
        while (g_isRunning && !isFill) {
            isFill = (bufnum == 0) ? !(fadc.NFADC400read_RunL(primary_mid)) 
                                   : !(fadc.NFADC400read_RunH(primary_mid));
            if (!isFill) std::this_thread::yield(); 
        }
        if(!g_isRunning) break;

        // 버퍼 내의 모든 페이지(Event)를 누락 없이 순회
        for (int evtIdx = 0; evtIdx < hevt; evtIdx++) {
            RawData* eventData = nullptr;
            // Object Pool에서 재활용 객체 꺼내기
            if (!g_freeQueue.TryPop(eventData)) {
                eventData = new RawData(); 
            }
            
            for (int i = 0; i < nbd; i++) {
                FadcBD* bd = runInfo->GetFadcBD(i);
                if (bd->IsTrgBD()) continue;

                for (int j = 0; j < bd->NCHANNEL(); j++) {
                    int cid = bd->CID(j) + 1; 
                    int page = bufnum * hevt + evtIdx; 
                    
                    fadc.NFADC400read_BUFFER(bd->MID(), cid, recordLength, page, raw_buffer);
                    
                    RawChannel* chObj = eventData->AddChannel(bd->CID(j), dataPoints);
                    for (int k = 0; k < dataPoints; k++) {
                        chObj->AddSample(raw_buffer[k]);
                    }
                }
            }
            g_dataQueue.Push(eventData);
            nevt++;

            if (presetNEvt > 0 && nevt >= presetNEvt) {
                g_isRunning = false;
                break;
            }
        }

        // 핑퐁 버퍼 즉시 스위칭 및 재시작
        bufnum = 1 - bufnum; 
        for (int i = 0; i < nbd; i++) {
            if (bufnum == 1) fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
            else             fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        }

        // [핵심] 실시간 Trigger Rate(Hz) 계산 및 출력
        if (nevt % (hevt * 4) == 0) {
            double curTime = sw.RealTime(); sw.Continue();
            double dt = curTime - lastTime;
            double rate = (dt > 0) ? (nevt - lastEvt) / dt : 0.0;
            lastTime = curTime; lastEvt = nevt;
            
            printf("\r\033[1;32m ⚡ Events: %-8d\033[0m | \033[1;33m⏱ Time: %-5.1f s\033[0m | \033[1;35m🔥 Rate: %-6.1f Hz\033[0m | \033[1;36m💾 DataQ: %-4zu\033[0m | \033[1;35m♻ Pool: %-4zu\033[0m", 
                   nevt, curTime, rate, g_dataQueue.Size(), g_freeQueue.Size());
            fflush(stdout);
        }
    }

    g_isRunning = false;
    printf("\n"); // 줄바꿈 버그 픽스
    g_dataQueue.Stop(); 
    
    if (consumerTh.joinable()) {
        consumerTh.join(); 
    }

    // 종료 시 잔여 메모리 풀 완벽 해제
    RawData* leftover = nullptr;
    while (g_freeQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }
    while (g_dataQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }

    delete[] raw_buffer;
    delete runInfo;
    vme.VMEclose();

    // [핵심] 종료 시 완벽한 서머리(Summary) 출력
    double totalTime = sw.RealTime();
    double avgRate = (totalTime > 0) ? (nevt / totalTime) : 0.0;
    std::cout << "\n\033[1;36m╔═══════════════════════ DAQ SUMMARY ═══════════════════════╗\033[0m" << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35d\033[0m \033[1;36m║\033[0m", "Total Events", nevt) << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-20s\033[0m : \033[1;37m%-35.2f sec\033[0m \033[1;36m║\033[0m", "Total Elapsed Time", totalTime) << std::endl;
    std::cout << Form("\033[1;36m║\033[0m \033[1;33m%-20s\033[0m : \033[1;32m%-35.2f Hz\033[0m \033[1;36m║\033[0m", "Average Trigger Rate", avgRate) << std::endl;
    std::cout << "\033[1;36m╚═══════════════════════════════════════════════════════════╝\033[0m\n" << std::endl;

    ELog::Print(ELog::INFO, "DAQ System gracefully stopped.");
    return 0;
}