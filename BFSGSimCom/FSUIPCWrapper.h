#pragma once

#include <windows.h>

#include "FSUIPC_User.h"

class FSUIPCWrapper
{
private:
    static bool blFSUIPCConnected;
    BOOL checkConnection(DWORD*);

public:
    FSUIPCWrapper();
    ~FSUIPCWrapper();

    BOOL FSUIPC_Read(DWORD dwOffset, DWORD dwSize, void* pDest);
    BOOL FSUIPC_Write(DWORD dwOffset, DWORD dwSize, void* pSrc);
    BOOL FSUIPC_Process();
};

