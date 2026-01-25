#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include "Option.h"

using namespace std;

Option::Option(int argc, char ** argv) {
    _iswrite = false;
    _isread = false;
    _isloop = false;
    _isquiet = false;
    _address = 0;
    _data = 0;
    _count = 1;
    _devid = 0;
    _am = 0x9; // Default A32D32 (0x09)

    int opt;
    while ((opt = getopt(argc, argv, "w:r:l:d:a:n:q")) != -1) {
        switch (opt) {
            case 'w':
                _iswrite = true;
                _address = strtoul(optarg, NULL, 0);
                break;
            case 'r':
                _isread = true;
                _address = strtoul(optarg, NULL, 0);
                break;
            case 'l':
                _isloop = true;
                _count = strtoul(optarg, NULL, 0);
                break;
            case 'd':
                _data = strtoul(optarg, NULL, 0);
                break;
            case 'a':
                _am = (unsigned short)strtoul(optarg, NULL, 0);
                break;
            case 'n':
                _devid = atoi(optarg);
                break;
            case 'q':
                _isquiet = true;
                break;
            default:
                PrintHelp();
                exit(1);
        }
    }

    if (!_iswrite && !_isread) {
        PrintHelp();
        exit(1);
    }
}

Option::~Option() {}

void Option::PrintHelp() {
    cout << "Usage: nk6uvme [-w/r address] [-d data] [-l count] [-a am] [-n devid]" << endl;
    cout << "  -w address : Write mode" << endl;
    cout << "  -r address : Read mode" << endl;
    cout << "  -d data    : Data to write" << endl;
    cout << "  -l count   : Loop count (0=infinite)" << endl;
    cout << "  -a am      : AM Code (default 0x09)" << endl;
    cout << "  -n devid   : Device ID (default 0)" << endl;
}