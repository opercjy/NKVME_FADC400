#include <iostream>
#include <cstdio>
#include "Actions.h"

using namespace std;

Actions::Actions(Option * opt, Device * dev) : _opt(opt), _dev(dev) {}
Actions::~Actions() {}

void Actions::Run() {
    if (_dev->Open() < 0) {
        cerr << "Error: Cannot open device " << _opt->GetDevID() << endl;
        return;
    }

    unsigned long count = _opt->GetCount();
    unsigned long addr = _opt->GetAddress();
    unsigned long data = _opt->GetData();
    unsigned short am = _opt->GetAM();
    bool quiet = _opt->IsQuiet();

    unsigned long i = 0;
    while (true) {
        if (_opt->IsWrite()) {
            _dev->Write(addr, data, am);
            if (!quiet) printf("Write: Addr=0x%08lX, Data=0x%08lX, AM=0x%X\n", addr, data, am);
        } 
        else if (_opt->IsRead()) {
            unsigned long rdata = _dev->Read(addr, am);
            if (!quiet) printf("Read : Addr=0x%08lX, Data=0x%08lX, AM=0x%X\n", addr, rdata, am);
        }

        i++;
        if (!_opt->IsLoop()) break;
        if (count > 0 && i >= count) break;
    }

    _dev->Close();
}