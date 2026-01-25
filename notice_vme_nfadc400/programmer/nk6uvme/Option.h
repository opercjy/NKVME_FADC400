#ifndef _OPTION_H_
#define _OPTION_H_

#include <string>

class Option {
public:
    Option(int argc, char ** argv);
    ~Option();

    bool IsWrite() const { return _iswrite; }
    bool IsRead() const { return _isread; }
    bool IsLoop() const { return _isloop; }
    bool IsQuiet() const { return _isquiet; }
    
    unsigned long GetAddress() const { return _address; }
    unsigned long GetData() const { return _data; }
    unsigned long GetCount() const { return _count; }
    int GetDevID() const { return _devid; }
    unsigned short GetAM() const { return _am; }

private:
    bool _iswrite;
    bool _isread;
    bool _isloop;
    bool _isquiet;
    
    unsigned long _address;
    unsigned long _data;
    unsigned long _count;
    int _devid;
    unsigned short _am;
    
    void PrintHelp();
};

#endif