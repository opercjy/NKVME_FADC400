#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono> 
#include <cstring>

#include "TROOT.h"
#include "TStopwatch.h"
#include "TError.h"      
#include <zmq.hpp> // 💡 [Phase 6] ZMQ 헤더 추가

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

// 💡 [Phase 6 핵심] TTree와 TSocket을 버리고 Pure Binary Dump 및 ZMQ 통신 적용
void ConsumerWorker(const char* outFileName, bool useDisplay) {
    FILE* fp = fopen(outFileName, "wb");
    if(!fp) {
        ELog::Print(ELog::FATAL, Form("Cannot create binary file: %s", outFileName));
        g_isRunning = false;
        return;
    }

    // ZMQ 퍼블리셔 셋업 (스레드 내부에서 독립적으로 바인딩)
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, ZMQ_PUB);
    if (useDisplay) {
        int conflate = 1;
        publisher.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
        publisher.bind("tcp://127.0.0.1:5556"); // FADC400용 ZMQ 포트
        ELog::Print(ELog::INFO, "ZMQ High-Speed Publisher Started (Port: 5556)");
    }

    int nWrite = 0;
    RawData* popData = nullptr;
    auto lastNetTime = std::chrono::steady_clock::now();

    while (g_dataQueue.WaitAndPop(popData)) {
        if (popData) {
            uint32_t nCh = popData->GetNChannels();
            uint32_t nPts = (nCh > 0) ? popData->GetChannel(0)->GetNSamples() : 0;

            // 1. 디스크에 순수 바이너리(Flat) 덤프 (무손실 최고 속도)
            uint32_t header[3] = { (uint32_t)nWrite, nCh, nPts };
            fwrite(header, sizeof(uint32_t), 3, fp);

            // ZMQ로 보낼 플랫 바이너리 페이로드 사전 할당
            size_t payloadSize = 12 + nCh * (4 + nPts * 2);
            zmq::message_t msg(payloadSize);
            unsigned char* msg_ptr = static_cast<unsigned char*>(msg.data());
            std::memcpy(msg_ptr, header, 12);
            size_t offset = 12;

            for (uint32_t c = 0; c < nCh; c++) {
                RawChannel* ch = popData->GetChannel(c);
                uint32_t chId = ch->GetChannelID();
                const std::vector<unsigned short>& trace = ch->GetTrace();

                // 파일 쓰기
                fwrite(&chId, sizeof(uint32_t), 1, fp);
                fwrite(trace.data(), sizeof(unsigned short), nPts, fp);

                // ZMQ 메모리 복사 (Zero-Copy 준비)
                std::memcpy(msg_ptr + offset, &chId, 4);
                offset += 4;
                std::memcpy(msg_ptr + offset, trace.data(), nPts * 2);
                offset += nPts * 2;
            }
            nWrite++;

            // 2. 파이썬 GUI가 뻗지 않도록 50ms 마다 1번씩만 ZMQ로 브로드캐스팅
            auto now = std::chrono::steady_clock::now();
            if (useDisplay && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastNetTime).count() > 50) {
                publisher.send(msg, zmq::send_flags::dontwait);
                lastNetTime = now;
            }

            popData->Clear("C"); 
            g_freeQueue.Push(popData); 
        }
    }

    fclose(fp);
    ELog::Print(ELog::INFO, Form("Consumer Thread: Saved %d binary events successfully.", nWrite));
}

int main(int argc, char ** argv) {
    ROOT::EnableThreadSafety(); 
    std::signal(SIGINT, SigIntHandler);
    std::signal(SIGTERM, SigIntHandler);

    TString configFile = "";
    TString outFile = "data.dat"; // 💡 확장자 기본값 .dat 로 변경
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
        ELog::Print(ELog::ERROR, "Usage: frontend -f <ticket.config> [-o <out.dat>] [-n <events>]");
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

    int recordLength = runInfo->GetFadcBD(0)->RL(); 
    int dataPoints = recordLength * 128;            
    int hevt = (recordLength > 4) ? (4096 / recordLength) : 512;
    
    char* bulk_buffers[32][4] = {nullptr}; 
    for (int i = 0; i < nbd; i++) {
        for (int j = 0; j < 4; j++) {
            bulk_buffers[i][j] = new char[0x100000]; 
        }
    }

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
        int zombie_err_cnt = 0;

        while (g_isRunning && !isFill) {
            unsigned long stat = (bufnum == 0) ? fadc.NFADC400read_RunL(primary_mid) : fadc.NFADC400read_RunH(primary_mid);
            if (stat == 0xFFFFFFFF) {
                zombie_err_cnt++;
                if (zombie_err_cnt > 10) {
                    ELog::Print(ELog::FATAL, "\n[FATAL] VME Bus Disconnected or Timeout (-1). Triggering Graceful Shutdown!");
                    g_isRunning = false;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            zombie_err_cnt = 0;
            isFill = (stat == 0); 
            if (!isFill) std::this_thread::yield(); 
        }
        if(!g_isRunning) break;

        int next_bufnum = 1 - bufnum; 
        for (int i = 0; i < nbd; i++) {
            if (next_bufnum == 1) fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
            else                  fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        }

        for (int i = 0; i < nbd; i++) {
            FadcBD* bd = runInfo->GetFadcBD(i);
            if (bd->IsTrgBD()) continue;
            for (int j = 0; j < bd->NCHANNEL(); j++) {
                int cid = bd->CID(j) + 1;
                fadc.NFADC400dump_BUFFER(bd->MID(), cid, recordLength, bufnum, bulk_buffers[i][j]);
            }
        }

        for (int evtIdx = 0; evtIdx < hevt; evtIdx++) {
            RawData* eventData = nullptr;
            if (!g_freeQueue.TryPop(eventData)) {
                eventData = new RawData(); 
            }
            
            for (int i = 0; i < nbd; i++) {
                FadcBD* bd = runInfo->GetFadcBD(i);
                if (bd->IsTrgBD()) continue;

                for (int j = 0; j < bd->NCHANNEL(); j++) {
                    int global_chId = (bd->MID() * 4) + bd->CID(j);
                    RawChannel* chObj = eventData->AddChannel(global_chId, dataPoints);
                    
                    unsigned char* evt_ptr = (unsigned char*)bulk_buffers[i][j] + (evtIdx * dataPoints * 2);
                    for (int k = 0; k < dataPoints; k++) {
                        unsigned short val = (evt_ptr[k*2 + 1] << 8) | evt_ptr[k*2];
                        chObj->AddSample(val & 0x0FFF); 
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

        bufnum = next_bufnum; 

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
    printf("\n"); 
    g_dataQueue.Stop(); 
    
    if (consumerTh.joinable()) {
        consumerTh.join(); 
    }

    RawData* leftover = nullptr;
    while (g_freeQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }
    while (g_dataQueue.TryPop(leftover)) { leftover->Clear("C"); delete leftover; }

    for (int i = 0; i < nbd; i++) {
        for (int j = 0; j < 4; j++) {
            delete[] bulk_buffers[i][j];
        }
    }
    
    delete runInfo;
    vme.VMEclose();

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
