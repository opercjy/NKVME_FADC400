#include "Device.h"
#include "pvmex.h"
#include <iostream>

using namespace std;

Device::Device(int id) : _id(id) {}
Device::~Device() {}

int Device::Open() {
    return Vopen(_id);
}

int Device::Close() {
    return Vclose(_id);
}

int Device::Write(unsigned long addr, unsigned long data, unsigned short am) {
    return Vwrite(_id, addr, data, am, 100);
}

unsigned long Device::Read(unsigned long addr, unsigned short am) {
    return Vread(_id, addr, am, 100);
}