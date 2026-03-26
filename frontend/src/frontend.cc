#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono> 
#include <cstring>

#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TSocket.h"
#include "TMessage.h"
#include "TStopwatch.h"
#include "TError.h"      

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
        g_isRunning = false; return;
    }

    TTree* tree = new TTree("FADC", "FADC Bulk Data Tree");
    tree->SetAutoSave(0); 
    
    RawData* treeEvtData = new RawData(); 
    tree->Branch("RawData", &treeEvtData);

    TSocket* socket = nullptr;
    auto lastConnTry = std::chrono::steady_clock::now(); 

    if (useDisplay) {
        Int_t oldLevel = gErrorIgnoreLevel; gErrorIgnoreLevel = kFatal; 
        socket = new TSocket("localhost", 9090);
        if (socket->IsValid()) {
            socket->SetOption(kNoBlock, 1);
            ELog::Print(ELog::INFO, "Connected to Display Server (localhost:9090)");
        } else { delete socket; socket = nullptr; }
        gErrorIgnoreLevel = oldLevel; 
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
            if (useDisplay && (!socket || !socket->IsValid())) {
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastConnTry).count() >= 2) {
                    if (socket) { delete socket; socket = nullptr; }
                    Int_t oldLevel = gErrorIgnoreLevel; gErrorIgnoreLevel = kFatal; 
                    socket = new TSocket("localhost", 9090);
                    if (socket->IsValid()) {
                        socket->SetOption(kNoBlock, 1);
                    } else { delete socket; socket = nullptr; }
                    gErrorIgnoreLevel = oldLevel; 
                    lastConnTry = now;
                }
            }

            if (socket && socket->IsValid()) {
                TMessage mess(kMESS_OBJECT);
                mess.WriteObject(treeEvtData);
                
                Int_t oldLevel = gErrorIgnoreLevel; gErrorIgnoreLevel = kFatal; 
                int snd = socket->Send(mess);
                gErrorIgnoreLevel = oldLevel;
                
                if (snd <= 0 && snd != -4) { 
                    socket->Close(); delete socket; socket = nullptr;
                }
            }

            temp->Clear("C"); 
            g_freeQueue.Push(temp); 
        }
    }

    tree->AutoSave(); 
    outFile->Write(); outFile->Close(); delete outFile;

    if (treeEvtData) { treeEvtData->Clear("C"); delete treeEvtData; }
    if (socket) { socket->Close(); delete socket; }
    ELog::Print(ELog::INFO, Form("Consumer Thread: Saved %d Bulk Blocks successfully.", nWrite));
}

int main(int argc, char ** argv) {
    ROOT::EnableThreadSafety(); 
    std::signal(SIGPIPE, SIG_IGN); 
    std::signal(SIGINT, SigIntHandler);
    std::signal(SIGTERM, SigIntHandler);

    TString configFile = ""; TString outFile = "data.root";
    int presetNEvt = 0; bool useDisplay = false; 
    
    int opt;
    while((opt = getopt(argc, argv, "f:n:o:d")) != -1) {
        switch(opt) {
            case 'f': configFile = optarg; break;
            case 'n': presetNEvt = atoi(optarg); break;
            case 'o': outFile = optarg; break;
            case 'd': useDisplay = true; break; 
        }
    }

    if(configFile == "") {
        ELog::Print(ELog::ERROR, "Usage: frontend -f <ticket.config> [-o <out.root>] [-n <events>] [-d]");
        return 1;
    }

    RunInfo* runInfo = new RunInfo();
    if (!ConfigParser::Parse(configFile.Data(), runInfo)) return 1;
    runInfo->Print();

    NK6UVME vme; NKNFADC400 fadc;
    if (vme.VMEopen() < 0) { ELog::Print(ELog::FATAL, "Failed to open USB-VME Controller."); return 1; }

    int nbd = runInfo->GetNFadcBD();
    for (int i = 0; i < nbd; i++) {
        FadcBD* bd = runInfo->GetFadcBD(i);
        unsigned long mid = bd->MID();
        
        fadc.NFADC400open(mid);
        unsigned long stat = fadc.NFADC400read_STAT(mid);
        if (stat == 0xFFFFFFFF) {
            ELog::Print(ELog::FATAL, Form(" [FATAL ERROR] FADC Board (MID: %lu) Not Found!", mid));
            vme.VMEclose(); return 1;
        }
        
        int rst = bd->RST();
        int resetTime = (rst & 0x4) ? 1 : 0;
        int resetNEvt = (rst & 0x2) ? 1 : 0;
        int resetRegi = (rst & 0x1) ? 1 : 0;
        fadc.NFADC400write_RM(mid, resetTime, resetNEvt, resetRegi);
        fadc.NFADC400reset(mid);

        fadc.NFADC400write_RL(mid, bd->RL());
        fadc.NFADC400write_TLT(mid, bd->TLT());
        fadc.NFADC400write_TOW(mid, bd->TOW());
        
        if (bd->DCE() == 0) fadc.NFADC400enable_DCE(mid);
        else fadc.NFADC400disable_DCE(mid);
        
        // 💡 [순서 주의] DACOFF 등 설정을 먼저 밀어넣음
        for (int ch = 0; ch < bd->NCHANNEL(); ch++) {
            int cid = bd->CID(ch) + 1; 
            fadc.NFADC400write_DACOFF(mid, cid, bd->DACOFF(ch));
            fadc.NFADC400write_DACGAIN(mid, cid, bd->DACGAIN(ch));
            fadc.NFADC400write_DLY(mid, cid, bd->DLY(ch));
            fadc.NFADC400write_THR(mid, cid, bd->THR(ch));
            fadc.NFADC400write_POL(mid, cid, bd->POL(ch));
            fadc.NFADC400write_DT(mid, cid, bd->DT(ch));
            fadc.NFADC400write_CW(mid, cid, bd->CW(ch));
            
            int tm_val = bd->TM(ch);
            int ew = (tm_val & 0x2) >> 1; int en = (tm_val & 0x1);
            fadc.NFADC400write_TM(mid, cid, ew, en);
            fadc.NFADC400write_PCT(mid, cid, bd->PCT(ch));
            fadc.NFADC400write_PCI(mid, cid, bd->PCI(ch));
            fadc.NFADC400write_PWT(mid, cid, bd->PWT(ch));
        }

        // 💡 [결정적 버그픽스] DACOFF 전압이 아날로그 회로에 정착할 시간을 준 뒤, Pedestal을 하드웨어에 각인!
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
        fadc.NFADC400measure_PED(mid, 0); 
        
        ELog::Print(ELog::INFO, Form("Board MID %lu Hardware Pedestal measured:", mid));
        for (int ch = 0; ch < bd->NCHANNEL(); ch++) {
            unsigned long ped_val = fadc.NFADC400read_PED(mid, bd->CID(ch) + 1);
            std::cout << "       [Ch " << bd->CID(ch) << "] Pedestal = " << ped_val << "\n";
        }
    }

    TFile* hfile = new TFile(outFile.Data(), "RECREATE");
    runInfo->Write(); hfile->Close(); delete hfile;

    int recordLength = runInfo->GetFadcBD(0)->RL(); 
    int dataPoints = recordLength * 128;            
    int hevt = (recordLength > 4) ? (4096 / recordLength) : 512;
    
    std::thread consumerTh(ConsumerWorker, outFile.Data(), useDisplay);
    ELog::Print(ELog::INFO, "DAQ Running. Press ABORT to stop.");

    int bufnum = 0;
    for (int i = 0; i < nbd; i++) {
        fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
    }

    int nevt = 0;
    TStopwatch sw; sw.Start();
    double lastTime = 0.0; int lastEvt = 0;
    auto lastPrintTime = std::chrono::steady_clock::now();

    while (g_isRunning) {
        if (g_dataQueue.Size() > 20000) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        unsigned long primary_mid = runInfo->GetFadcBD(0)->MID();
        int isFill = 0; int zombie_err_cnt = 0;

        while (g_isRunning && !isFill) {
            unsigned long stat = (bufnum == 0) ? fadc.NFADC400read_RunL(primary_mid) : fadc.NFADC400read_RunH(primary_mid);
            if (stat == 0xFFFFFFFF) {
                zombie_err_cnt++;
                if (zombie_err_cnt > 10) {
                    ELog::Print(ELog::FATAL, "\n[FATAL] VME Bus Disconnected or Timeout (-1).");
                    g_isRunning = false; break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); continue;
            }
            zombie_err_cnt = 0;
            isFill = (stat == 0); 
            if (!isFill) std::this_thread::yield(); 
        }
        if(!g_isRunning) break;

        RawData* dumpData = nullptr;
        if (!g_freeQueue.TryPop(dumpData)) dumpData = new RawData(); 
        
        for (int i = 0; i < nbd; i++) {
            FadcBD* bd = runInfo->GetFadcBD(i);
            if (bd->IsTrgBD()) continue;

            for (int j = 0; j < bd->NCHANNEL(); j++) {
                int cid = bd->CID(j) + 1;
                int global_chId = (bd->MID() * 4) + bd->CID(j);
                
                RawChannel* chObj = dumpData->AddChannel(global_chId);
                chObj->ReserveBulk(hevt, dataPoints);
                
                fadc.NFADC400dump_BUFFER(bd->MID(), cid, recordLength, bufnum, (char*)chObj->GetBulkData());
                fadc.NFADC400dump_TAG(bd->MID(), cid, recordLength, bufnum, (char*)chObj->GetTagData()); 
            }
        }
        
        g_dataQueue.Push(dumpData);
        nevt += hevt; 

        if (presetNEvt > 0 && nevt >= presetNEvt) {
            g_isRunning = false; break;
        }

        if (bufnum == 0) {
            bufnum = 1;
            for (int i = 0; i < nbd; i++) fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        } else {
            bufnum = 0;
            for (int i = 0; i < nbd; i++) fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
        }

        auto nowTime = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastPrintTime).count() > 500) {
            double curTime = sw.RealTime(); sw.Continue();
            double dt = curTime - lastTime;
            double rate = (dt > 0) ? (nevt - lastEvt) / dt : 0.0;
            lastTime = curTime; lastEvt = nevt;
            
            printf("\r\033[1;32m ⚡ Events: %-8d\033[0m | \033[1;33m⏱ Time: %-5.1f s\033[0m | \033[1;35m🔥 Rate: %-6.1f Hz\033[0m | \033[1;36m💾 DataQ: %-4zu\033[0m | \033[1;35m♻ Pool: %-4zu\033[0m", 
                   nevt, curTime, rate, g_dataQueue.Size(), g_freeQueue.Size());
            fflush(stdout);
            lastPrintTime = nowTime;
        }
    }

    g_isRunning = false; printf("\n"); g_dataQueue.Stop(); 
    if (consumerTh.joinable()) consumerTh.join(); 

    RawData* leftover = nullptr;
    while (g_freeQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }
    while (g_dataQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }
    
    delete runInfo; vme.VMEclose();

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