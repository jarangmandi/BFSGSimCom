#pragma once

#include <windows.h>

#include <thread>

#include "FSUIPC_User.h"

class FSUIPCWrapper
{
public:
    enum ComRadio
    {
        None = 0,
        Com1 = 1,
        Com2 = 2,
        Com12 = 3
    };

    struct SimComData {
        int iCom1Freq;
        int iCom1Sby;
        int iCom2Freq;
        int iCom2Sby;
        ComRadio selectedCom;
        bool blWoW;
        double dLat;
        double dLon;
        bool blComChanged;
        bool blPosChanged;
    };

private:
    static bool cFSUIPCConnected;
    static bool cRun;
    WORD cCom1Freq;
    WORD cCom1Sby;
    WORD cCom2Freq;
    WORD cCom2Sby;
    BYTE cSelectedCom;
    WORD cWoW;
    double cLat;
    double cLon;

    void (*callback)(SimComData);

    BOOL checkConnection(DWORD*);
    void workerThread(void);
    
    SimComData FSUIPCWrapper::getSimComData(bool blComChanged, bool blPosChange, bool blOtherChanged);

    std::thread* t1 = NULL;

public:

    FSUIPCWrapper(void (*cb)(SimComData));
    ~FSUIPCWrapper();

    bool isConnected() {
        return cFSUIPCConnected;
    };

    BOOL FSUIPC_Read(DWORD dwOffset, DWORD dwSize, void* pDest);
    BOOL FSUIPC_Write(DWORD dwOffset, DWORD dwSize, void* pSrc);
    BOOL FSUIPC_Process();
    void start(void);
    void stop(void);

    static std::string toString(FSUIPCWrapper::SimComData scd);
};

