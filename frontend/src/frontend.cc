#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <unistd.h>
#include <chrono> 
#include <cstring>
#include <iomanip>

#include "TROOT.h"
#include "TStopwatch.h"
#include "TError.h"      
#include <zmq.hpp> 

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

// -------------------------------------------------------------------------
// Consumer: 순수 바이너리 디스크 덤프 + ZMQ 브로드캐스팅 + 💡 0.5초 정밀 상태 출력
// -------------------------------------------------------------------------
void ConsumerWorker(const char* outFileName) {
    FILE* fp = fopen(outFileName, "wb");
    if(!fp) {
        ELog::Print(ELog::FATAL, Form("Cannot create binary file: %s", outFileName));
        g_isRunning = false;
        return;
    }

    // ZMQ 브로드캐스터 (GUI 연결 대기)
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, ZMQ_PUB);
    int conflate = 1;
    publisher.setsockopt(ZMQ_CONFLATE, &conflate, sizeof(conflate));
    publisher.bind("tcp://127.0.0.1:5556");

    int nWrite = 0;
    size_t total_written_bytes = 0;
    RawData* popData = nullptr;
    
    auto sys_start_time = std::chrono::system_clock::now();
    auto ui_timer = std::chrono::steady_clock::now();
    auto perf_start_time = std::chrono::steady_clock::now();
    auto zmq_timer = std::chrono::steady_clock::now();
    
    int last_print_events = 0;
    size_t last_print_bytes = 0;

    while (g_isRunning || g_dataQueue.Size() > 0) {
        if (g_dataQueue.TryPop(popData)) {
            if (popData) {
                uint32_t nCh = popData->GetNChannels();
                uint32_t nPts = (nCh > 0) ? popData->GetChannel(0)->GetNSamples() : 0;

                // 1. 하드디스크에 순수 바이너리 덤프
                uint32_t header[3] = { (uint32_t)nWrite, nCh, nPts };
                total_written_bytes += fwrite(header, sizeof(uint32_t), 3, fp) * 4;

                size_t payloadSize = 12 + nCh * (4 + nPts * 2);
                zmq::message_t msg(payloadSize);
                unsigned char* msg_ptr = static_cast<unsigned char*>(msg.data());
                std::memcpy(msg_ptr, header, 12);
                size_t offset = 12;

                for (uint32_t c = 0; c < nCh; c++) {
                    RawChannel* ch = popData->GetChannel(c);
                    uint32_t chId = ch->GetChannelID();
                    const std::vector<unsigned short>& trace = ch->GetTrace();

                    total_written_bytes += fwrite(&chId, sizeof(uint32_t), 1, fp) * 4;
                    total_written_bytes += fwrite(trace.data(), sizeof(unsigned short), nPts, fp) * 2;

                    std::memcpy(msg_ptr + offset, &chId, 4); offset += 4;
                    std::memcpy(msg_ptr + offset, trace.data(), nPts * 2); offset += nPts * 2;
                }
                nWrite++;

                auto now = std::chrono::steady_clock::now();
                // 2. 파이썬 GUI용 ZMQ 전송 (50ms 당 1회 제한으로 GUI 뻗음 방지)
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - zmq_timer).count() > 50) {
                    publisher.send(msg, zmq::send_flags::dontwait);
                    zmq_timer = now;
                }

                popData->Clear("C"); 
                g_freeQueue.Push(popData); 
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }

        // 💡 3. 파이썬 GUI Regex 100% 매칭을 위한 0.5초 주기 상태 출력
        auto current_time = std::chrono::steady_clock::now();
        double ui_elapsed_sec = std::chrono::duration<double>(current_time - ui_timer).count();
        
        if (ui_elapsed_sec >= 0.5) {
            double speed_mbps = (((total_written_bytes - last_print_bytes) / 1048576.0) / ui_elapsed_sec);
            double evt_rate = (nWrite - last_print_events) / ui_elapsed_sec;
            double total_elapsed = std::chrono::duration<double>(current_time - perf_start_time).count();
            
            std::cout << "[LIVE DAQ] "
                      << "Time: \033[1;32m" << std::fixed << std::setprecision(1) << total_elapsed << " s\033[0m | "
                      << "Events: " << nWrite << " | "
                      << "Size: " << std::fixed << std::setprecision(2) << (total_written_bytes / 1048576.0) << " MB | "
                      << "Rate: " << std::fixed << std::setprecision(1) << evt_rate << " Hz | "
                      << "Speed: " << std::fixed << std::setprecision(2) << speed_mbps << " MB/s | "
                      << "DataQ: " << g_dataQueue.Size() << " | "
                      << "Pool: " << g_freeQueue.Size() << "\n" << std::flush;
            
            ui_timer = current_time;
            last_print_events = nWrite;
            last_print_bytes = total_written_bytes;
        }
    }

    fclose(fp);
    
    // 💡 4. 파이썬 GUI가 런 정보를 DB에 완벽히 기록하도록 Summary 출력 포맷 통일
    auto sys_end_time = std::chrono::system_clock::now();
    auto perf_end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> total_elapsed = perf_end_time - perf_start_time;
    double total_sec = total_elapsed.count();
    double avg_rate = (total_sec > 0) ? (nWrite / total_sec) : 0.0;
    
    std::cout << "\n\033[1;36m========================================================\033[0m\n";
    std::cout << "\033[1;32m   [ Run Summary ]\033[0m\n";
    std::cout << "   Total Elapsed Time : " << std::fixed << std::setprecision(2) << total_sec << " sec\n";
    std::cout << "   Total Events       : " << nWrite << "\n";
    std::cout << "   Total Written      : " << std::fixed << std::setprecision(2) << (total_written_bytes / 1048576.0) << " MB\n";
    std::cout << "   Average Trigger Rate : " << std::fixed << std::setprecision(2) << avg_rate << " Hz\n";
    std::cout << "\033[1;36m========================================================\033[0m\n";
}

int main(int argc, char ** argv) {
    ROOT::EnableThreadSafety(); 
    std::signal(SIGINT, SigIntHandler);
    std::signal(SIGTERM, SigIntHandler);

    TString configFile = "";
    TString outFile = "data.dat"; 
    int presetNEvt = 0;
    int presetMaxTime = 0; 
    
    int opt;
    // 💡 [버그 수정 1] 파이썬 GUI의 -t (시간 제한) 인자 파싱 추가
    while((opt = getopt(argc, argv, "f:n:o:t:")) != -1) {
        switch(opt) {
            case 'f': configFile = optarg; break;
            case 'n': presetNEvt = atoi(optarg); break;
            case 'o': outFile = optarg; break;
            case 't': presetMaxTime = atoi(optarg); break; 
        }
    }

    if(configFile == "") {
        ELog::Print(ELog::ERROR, "Usage: frontend -f <ticket.config> [-o <out.dat>] [-n <events>] [-t <seconds>]");
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

    std::thread consumerTh(ConsumerWorker, outFile.Data());
    ELog::Print(ELog::INFO, "DAQ Running. Press ABORT to stop.");

    int bufnum = 0;
    for (int i = 0; i < nbd; i++) {
        fadc.NFADC400startL(runInfo->GetFadcBD(i)->MID());
        fadc.NFADC400startH(runInfo->GetFadcBD(i)->MID());
    }

    int nevt = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (g_isRunning) {
        // 💡 [버그 수정 2] -t 옵션에 의한 시간 강제 종료 기능 정상화
        if (presetMaxTime > 0) {
            auto current_time = std::chrono::steady_clock::now();
            int elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            if (elapsed >= presetMaxTime) {
                std::cout << "\n";
                ELog::Print(ELog::INFO, Form("Time limit reached (%d sec). Stopping DAQ...", presetMaxTime));
                g_isRunning = false;
                break;
            }
        }

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
    }

    g_isRunning = false;
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

    ELog::Print(ELog::INFO, "DAQ System gracefully stopped.");
    return 0;
}
