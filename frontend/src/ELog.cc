#include "ELog.hh"
#include <iostream>
#include <ctime>
#include <iomanip>

std::mutex ELog::_logMutex;

void ELog::Print(ELogType type, const TString& msg) {
    std::lock_guard<std::mutex> lock(_logMutex); 

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";

    switch(type) {
        case INFO:    std::cout << "\033[1;32m[INFO]\033[0m    "; break; // 녹색
        case WARNING: std::cout << "\033[1;33m[WARN]\033[0m    "; break; // 노란색
        case ERROR:   std::cout << "\033[1;31m[ERROR]\033[0m   "; break; // 빨간색
        case FATAL:   std::cout << "\033[1;41m[FATAL]\033[0m   "; break; // 배경 빨강
    }
    std::cout << msg.Data() << std::endl;
}
