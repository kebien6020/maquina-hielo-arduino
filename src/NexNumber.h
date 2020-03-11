#ifndef __NEXNUMBER_H__
#define __NEXNUMBER_H__

#include <NexTouch.h>
#include <NexHardware.h>

class NexNumber : public NexTouch {
public:
    NexNumber(uint8_t pid, uint8_t cid, const char *name)
        :NexTouch(pid, cid, name)
    {
    }

    bool setValue(int32_t val)
    {
        char buf[10] = {0};
        String cmd;

        itoa(val, buf, 10);
        cmd += getObjName();
        cmd += ".val=";
        cmd += buf;
        sendCommand(cmd.c_str());
        return recvRetCommandFinished();
    }
};

#endif