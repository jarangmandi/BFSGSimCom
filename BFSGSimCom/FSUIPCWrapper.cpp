#include "FSUIPCWrapper.h"

bool FSUIPCWrapper::blFSUIPCConnected = 0;

FSUIPCWrapper::FSUIPCWrapper()
{
    DWORD dwResult;

    checkConnection(&dwResult);
}

BOOL FSUIPCWrapper::checkConnection(DWORD* pdwResult)
{
    BOOL retValue = 0;

    if (!blFSUIPCConnected)
    {
        try
        {
            retValue = ::FSUIPC_Open(SIM_ANY, pdwResult);
            blFSUIPCConnected = (retValue!=0);
        }
        catch (...)
        {
            retValue = 0;
            blFSUIPCConnected = false;
        }
    }

    return retValue;
}


BOOL FSUIPCWrapper::FSUIPC_Read(DWORD dwOffset, DWORD dwSize, void* pDest)
{
    BOOL retValue = 0;
    DWORD dwResult;

    if (checkConnection(&dwResult))
    {
        try
        {
            retValue = ::FSUIPC_Read(dwOffset, dwSize, pDest, &dwResult);
        }
        catch (...)
        {
            retValue = 0;
        }
    }

    return retValue;
}

BOOL FSUIPCWrapper::FSUIPC_Write(DWORD dwOffset, DWORD dwSize, void* pSrc)
{
    BOOL retValue = 0;
    DWORD dwResult;

    if (checkConnection(&dwResult))
    {
        try
        {
            retValue = ::FSUIPC_Write(dwOffset, dwSize, pSrc, &dwResult);
        }
        catch (...)
        {
            retValue = 0;
        }
    }

    return retValue;
}

BOOL FSUIPCWrapper::FSUIPC_Process()
{
    BOOL retValue = 0;
    DWORD dwResult;

    if (checkConnection(&dwResult))
    {
        try
        {
            retValue = ::FSUIPC_Process(&dwResult);
        }
        catch (...)
        {
            retValue = 0;
        }
    }

    return retValue;
}


FSUIPCWrapper::~FSUIPCWrapper()
{
    // There's no harm in calling this whether FSUIPC is open or not.
    ::FSUIPC_Close();
}
