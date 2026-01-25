#include "Option.h"
#include "Device.h"
#include "Actions.h"

int main(int argc, char ** argv) {
    Option * opt = new Option(argc, argv);
    Device * dev = new Device(opt->GetDevID());
    Actions * act = new Actions(opt, dev);

    act->Run();

    delete act;
    delete dev;
    delete opt;

    return 0;
}