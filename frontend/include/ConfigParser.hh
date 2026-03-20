#ifndef ConfigParser_hh
#define ConfigParser_hh

#include "RunInfo.hh"

class ConfigParser {
public:
    static bool Parse(const char* filename, RunInfo* runinfo);
};
#endif
