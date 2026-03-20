#ifndef ELog_hh
#define ELog_hh

#include "TString.h"
#include <mutex>

class ELog {
public:
    enum ELogType { INFO = 0, WARNING, ERROR, FATAL };

    // 멀티스레드 환경에서 출력이 겹치지 않게 보장
    static void Print(ELogType type, const TString& msg);

private:
    static std::mutex _logMutex;
};
#endif
