#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono> 

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TSocket.h"
#include "TMessage.h"
#include "TStopwatch.h"
#include "TError.h"      // 🔥 [버그 픽스] ROOT 내부 에러 억제용 헤더

#include "ThreadSafeQueue.hh"
#include "ConfigParser.hh"
#include "ELog.hh"
#include "RunInfo.hh"
#include "RawData.hh"

#include "Notice6UVMEROOT.h"
#include "NoticeNFADC400ROOT.h"

std::atomic<bool> g_isRunning(true);
ThreadSafeQueue<RawData*> g_dataQueue;
ThreadSafeQueue<RawData*> g_freeQueue; 

void SigIntHandler(int /*signum*/) {
    ELog::Print(ELog::WARNING, "\nInterrupt signal (Ctrl+C) received! Shutting down gracefully...");
    g_isRunning = false;
    g_dataQueue.Stop(); 
}

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
    auto lastNetTime = std::chrono::steady_clock::now();
    auto lastConnTry = std::chrono::steady_clock::now(); 

    // 💡 [버그 픽스] 첫 연결 시 ROOT 에러 묵음 처리 (터미널 스팸 방지)
    if (useDisplay) {
        Int_t oldLevel = gErrorIgnoreLevel;
        gErrorIgnoreLevel = kFatal; 
        
        socket = new TSocket("localhost", 9090);
        
        gErrorIgnoreLevel = oldLevel; 

        if (!socket->IsValid()) {
            delete socket; socket = nullptr;
        } else {
            ELog::Print(ELog::INFO, "Connected to Display Server (localhost:9090)");
        }
    }

    int nWrite = 0;
    RawData* popData = nullptr;

    while (g_dataQueue.WaitAndPop(popData)) {
        if (popData) {
            RawData* temp = treeEvtData;
            treeEvtData = popData; 
            
            tree->Fill();
            nWrite++;

            auto now = std::chrono::steady_clock::now();

            // 💡 [버그 픽스] 소켓 재연결 시(2초마다) ROOT 에러 묵음 처리
            if (useDisplay && (!socket || !socket->IsValid())) {
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastConnTry).count() >= 2) {
                    if (socket) { delete socket; socket = nullptr; }
                    
                    Int_t oldLevel = gErrorIgnoreLevel;
                    gErrorIgnoreLevel = kFatal; 
                    
                    socket = new TSocket("localhost", 9090);
                    
                    gErrorIgnoreLevel = oldLevel; 

                    if (socket->IsValid()) {
                        ELog::Print(ELog::INFO, "Reconnected to Display Server!");
                    } else {
                        delete socket; socket = nullptr;
                    }
                    lastConnTry = now;
                }
            }

            // 50ms 간격으로 소켓을 통해 데이터를 온라인 모니터(Display)로 전송
            if (socket && socket->IsValid() && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNetTime).count() > 50) {
                TMessage mess(kMESS_OBJECT);
                mess.WriteObject(treeEvtData);
                
                // 💡 [버그 픽스] Send 과정에서 모니터가 꺼졌을 때 발생하는 "Connection reset by peer" 에러 묵음 처리
                Int_t oldLevel = gErrorIgnoreLevel;
                gErrorIgnoreLevel = kFatal; 
                
                int snd = socket->Send(mess);
                
                gErrorIgnoreLevel = oldLevel;
                
                if (snd <= 0) { 
                    socket->Close();
                    delete socket;
                    socket = nullptr;
                }
                lastNetTime = now;
            }

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
        FadcBD* bd = runInfo->GetFadcBD(i);
        unsigned long mid = bd->MID();
        
        fadc.NFADC400open(mid);
        unsigned long stat = fadc.NFADC400read_STAT(mid);
        if (stat == 0xFFFFFFFF) {
            ELog::Print(ELog::FATAL, "==============================================================");
            ELog::Print(ELog::FATAL, Form(" [FATAL ERROR] FADC Board (MID: %lu) Not Found or Mismatch!", mid));
            vme.VMEclose();
            return 1;
        }
        
        ELog::Print(ELog::INFO, Form("Board MID %lu connected. Writing parameters to Hardware...", mid));
        fadc.NFADC400reset(mid);
        
        fadc.NFADC400write_RL(mid, bd->RL());
        fadc.NFADC400write_TLT(mid, bd->TLT());
        fadc.NFADC400write_TOW(mid, bd->TOW());
        if (bd->DCE()) fadc.NFADC400enable_DCE(mid);
        else fadc.NFADC400disable_DCE(mid);
        
        for (int ch = 0; ch < bd->NCHANNEL(); ch++) {
            int cid = bd->CID(ch) + 1; 
            fadc.NFADC400write_THR(mid, cid, bd->THR(ch));
            fadc.NFADC400write_POL(mid, cid, bd->POL(ch));
            fadc.NFADC400write_DACOFF(mid, cid, bd->DACOFF(ch));
            fadc.NFADC400write_DLY(mid, cid, bd->DLY(ch));
            fadc.NFADC400write_DACGAIN(mid, cid, bd->DACGAIN(ch));
            fadc.NFADC400write_DT(mid, cid, bd->DT(ch));
            fadc.NFADC400write_CW(mid, cid, bd->CW(ch));
            
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

    int recordLength = runInfo->GetFadcBD(0)->RL(); 
    int dataPoints = recordLength * 128;            
    int hevt = (recordLength > 4) ? (4096 / recordLength) : 512;
    
    unsigned short* raw_buffer = new unsigned short[dataPoints];

    std::thread consumerTh(ConsumerWorker, outFile.Data(), true);
    ELog::Print(ELog::INFO, "DAQ Running. Press ABORT to stop.");

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
        if (g_dataQueue.Size() > 20000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        unsigned long primary_mid = runInfo->GetFadcBD(0)->MID();
        int isFill = 0;
        while (g_isRunning && !isFill) {
            isFill = (bufnum == 0) ? !(fadc.NFADC400read_RunL(primary_mid)) 
                                   : !(fadc.NFADC400read_RunH(primary_mid));
            if (!isFill) std::this_thread::yield(); 
        }
        if(!g_isRunning) break;

        for (int evtIdx = 0; evtIdx < hevt; evtIdx++) {
            RawData* eventData = nullptr;
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
                    
                    // 💡 [버그 픽스] 다중 보드 지원 글로벌 채널 매핑 (채널 덮어쓰기 방지)
                    int global_chId = (bd->MID() * 4) + bd->CID(j);
                    RawChannel* chObj = eventData->AddChannel(global_chId, dataPoints);
                    
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

        bufnum = 1 - bufnum; 
        for (int i = 0; i < nbd; i++) {
            if (bufnum == 1) fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
            else             fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        }

        if (nevt % (hevt * 4) == 0) {
            double curTime = sw.RealTime(); sw.Continue();
            double dt = curTime - lastTime;
            double rate = (dt > 0) ? (nevt - lastEvt) / dt : 0.0;
            lastTime = curTime; lastEvt = nevt;
            
            // ✅ [기능 보존] 큐/풀 상태 모니터링 출력
            printf("\r\033[1;32m ⚡ Events: %-8d\033[0m | \033[1;33m⏱ Time: %-5.1f s\033[0m | \033[1;35m🔥 Rate: %-6.1f Hz\033[0m | \033[1;36m💾 DataQ: %-4zu\033[0m | \033[1;35m♻ Pool: %-4zu\033[0m", 
                   nevt, curTime, rate, g_dataQueue.Size(), g_freeQueue.Size());
            fflush(stdout);
        }
    }

    g_isRunning = false;
    printf("\n"); 
    g_dataQueue.Stop(); 
    
    if (consumerTh.joinable()) {
        consumerTh.join(); 
    }

    RawData* leftover = nullptr;
    while (g_freeQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }
    while (g_dataQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }

    delete[] raw_buffer;
    delete runInfo;
    vme.VMEclose();

    // ✅ [기능 보존] 종료 시 Summary 표 출력
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