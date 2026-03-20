#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>

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
    TStopwatch netTimer; netTimer.Start();

    while (g_dataQueue.WaitAndPop(popData)) {
        if (popData) {
            RawData* temp = treeEvtData;
            treeEvtData = popData; // Branch 포인터 스왑
            
            tree->Fill();
            nWrite++;

            // TCP 버퍼 폭발 방지 (50ms마다 한 번만 소켓 송신)
            if (socket && socket->IsValid() && netTimer.RealTime() > 0.05) {
                TMessage mess(kMESS_OBJECT);
                mess.WriteObject(treeEvtData);
                socket->Send(mess);
                netTimer.Start(); 
            } else if (socket) {
                netTimer.Continue();
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

    int nbd = runInfo->GetNFadcBD();
    for (int i = 0; i < nbd; i++) {
        unsigned long mid = runInfo->GetFadcBD(i)->MID();
        fadc.NFADC400open(mid);
        fadc.NFADC400reset(mid);
        fadc.NFADC400write_RL(mid, runInfo->GetFadcBD(i)->RL());
        fadc.NFADC400write_TLT(mid, runInfo->GetFadcBD(i)->TLT());
        fadc.NFADC400write_TOW(mid, runInfo->GetFadcBD(i)->TOW());
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

        if (nevt % (hevt * 4) == 0) {
            printf("\rEvents: %d | Time: %.1f s | DataQ: %zu | Pool: %zu ", 
                   nevt, sw.RealTime(), g_dataQueue.Size(), g_freeQueue.Size());
            fflush(stdout);
            sw.Continue();
        }
    }

    g_isRunning = false;
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

    std::cout << std::endl;
    ELog::Print(ELog::INFO, "DAQ System gracefully stopped.");
    return 0;
}
