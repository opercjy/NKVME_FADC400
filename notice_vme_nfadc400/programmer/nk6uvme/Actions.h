#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include "Option.h"
#include "Device.h"

class Actions {
public:
    Actions(Option * opt, Device * dev);
    ~Actions();
    void Run();

private:
    Option * _opt;
    Device * _dev;
};

#endif