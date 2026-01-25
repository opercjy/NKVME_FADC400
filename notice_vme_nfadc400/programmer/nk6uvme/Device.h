#ifndef _DEVICE_H_
#define _DEVICE_H_

class Device {
public:
    Device(int id);
    ~Device();
    
    int Open();
    int Close();
    int Write(unsigned long addr, unsigned long data, unsigned short am);
    unsigned long Read(unsigned long addr, unsigned short am);

private:
    int _id;
};

#endif