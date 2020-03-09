#ifndef __NEXDUALBUTTON_H__
#define __NEXDUALBUTTON_H__

#include <NexButton.h>
#include <NexHardware.h>

class NexDualButton : public NexButton {
public:
    NexDualButton(uint8_t pid, uint8_t cid, const char *name)
        :NexButton(pid, cid, name)
    {
    }

    uint16_t getVal(uint32_t* ret)
    {
        String cmd = String("get ");
        cmd += getObjName();
        cmd += ".val";
        sendCommand(cmd.c_str());
        return recvRetNumber(ret);
    }

    bool setVal(uint32_t val)
    {
        char buf[10] = {0};
        String cmd;

        utoa(val, buf, 10);
        cmd += getObjName();
        cmd += ".val=";
        cmd += buf;
        sendCommand(cmd.c_str());
        return recvRetCommandFinished();    
    }
};

#endif