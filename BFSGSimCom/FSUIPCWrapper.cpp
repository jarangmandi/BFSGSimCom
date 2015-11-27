#include <thread>
#include <chrono>

#include "FSUIPCWrapper.h"

bool FSUIPCWrapper::blFSUIPCConnected = false;
bool FSUIPCWrapper::blRun = false;

FSUIPCWrapper::FSUIPCWrapper(void (*cb)(SimComData))
{
    DWORD dwResult;

    checkConnection(&dwResult);
    callback = cb;
}

void FSUIPCWrapper::start(void)
{
    blRun = true;

    t1 = new std::thread(&FSUIPCWrapper::workerThread, FSUIPCWrapper(callback));
}

void FSUIPCWrapper::stop(void)
{
    blRun = false;

    // There's no harm in calling this whether FSUIPC is open or not.
    ::FSUIPC_Close();

    if (t1 != NULL) t1->join();
}

void FSUIPCWrapper::workerThread(void)
{
    WORD com1;
    WORD com1s;
    WORD com2;
    WORD com2s;
    BYTE rSwitch;

    while (blRun)
    {
        FSUIPC_Read(0x034E, 2, &com1);
        FSUIPC_Read(0x3118, 2, &com2);
        FSUIPC_Read(0x311A, 2, &com1s);
        FSUIPC_Read(0x311C, 2, &com2s);
        FSUIPC_Read(0x3122, 1, &rSwitch);

        if (FSUIPC_Process())
        {
            bool blChanged = false;


            if (com1 != iCom1Freq)
            {
                blChanged = true;
                iCom1Freq = com1;
            }

            if (com1s != iCom1Sby)
            {
                blChanged = true;
                iCom1Sby = com1s;
            }

            if (com2 != iCom2Freq)
            {
                blChanged = true;
                iCom2Freq = com2;
            }

            if (com2s != iCom2Sby)
            {
                blChanged = true;
                iCom2Sby = com2s;
            }

            if (rSwitch != selectedCom)
            {
                blChanged = true;
                selectedCom = rSwitch;
            }

            if (blChanged)
            {
                if (callback != NULL)
                {
                    (*callback)(getSimComData());
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
};


FSUIPCWrapper::SimComData FSUIPCWrapper::getSimComData() {
    
    SimComData simcomdata;

    simcomdata.iCom1Freq = 10000 + 1000 * ((iCom1Freq & 0xf000) >> 12) + 100 * ((iCom1Freq & 0x0f00) >> 8) + 10 * ((iCom1Freq & 0x00f0) >> 4) + (iCom1Freq & 0x000f);
    simcomdata.iCom1Sby = 10000 + 1000 * ((iCom1Sby & 0xf000) >> 12) + 100 * ((iCom1Sby & 0x0f00) >> 8) + 10 * ((iCom1Sby & 0x00f0) >> 4) + (iCom1Sby & 0x000f);
    simcomdata.iCom2Freq = 10000 + 1000 * ((iCom2Freq & 0xf000) >> 12) + 100 * ((iCom2Freq & 0x0f00) >> 8) + 10 * ((iCom2Freq & 0x00f0) >> 4) + (iCom2Freq & 0x000f);
    simcomdata.iCom2Sby = 10000 + 1000 * ((iCom2Sby & 0xf000) >> 12) + 100 * ((iCom2Sby & 0x0f00) >> 8) + 10 * ((iCom2Sby & 0x00f0) >> 4) + (iCom2Sby & 0x000f);

    // This is a kluge because XPUIPC doesn't report the correct channel and the config file that comes with it doesn't seem to work as advertised.
    // Look for a FSUIPC_Version of 0x50000006 - who knows what will happen when FSUIPC catches up
    if (FSUIPC_Version == 0x50000006)
        simcomdata.selectedCom = ComRadio(((selectedCom & 0x40) ? Com1 : None) + ((selectedCom & 0x80) ? Com2 : None));
    else
        simcomdata.selectedCom = ComRadio(((selectedCom & 0x80) ? Com1 : None) + ((selectedCom & 0x40) ? Com2 : None));

    return simcomdata;
};



// Check connection returns TRUE if connected, and FALSE if not...
BOOL FSUIPCWrapper::checkConnection(DWORD* pdwResult)
{
    BOOL retValue = 0;

    if (!blFSUIPCConnected)
    {
        try
        {
            retValue = ::FSUIPC_Open(SIM_ANY, pdwResult);
            blFSUIPCConnected = (retValue != FALSE) || (*pdwResult == FSUIPC_ERR_OPEN);
        }
        catch (...)
        {
            retValue = FALSE;
            blFSUIPCConnected = false;
        }
    }
    else
    {
        retValue = TRUE;
    }

    return retValue;
}


BOOL FSUIPCWrapper::FSUIPC_Read(DWORD dwOffset, DWORD dwSize, void* pDest)
{
    BOOL retValue = 0;
    DWORD dwResult = FSUIPC_ERR_OK;

    if (checkConnection(&dwResult))
    {
        try
        {
            retValue = ::FSUIPC_Read(dwOffset, dwSize, pDest, &dwResult);
        }
        catch (...)
        {
            retValue = FALSE;
        }
    }

    blFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

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
            retValue = FALSE;
        }
    }

    blFSUIPCConnected = (dwResult != FSUIPC_ERR_NOTOPEN);

    return retValue;
}

BOOL FSUIPCWrapper::FSUIPC_Process()
{
    BOOL retValue = 0;
    DWORD dwResult = FSUIPC_ERR_NOTOPEN;

    if (checkConnection(&dwResult))
    {
        try
        {
            retValue = ::FSUIPC_Process(&dwResult);
        }
        catch (...)
        {
            retValue = FALSE;
        }
    }

    blFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

    return retValue;
}


FSUIPCWrapper::~FSUIPCWrapper()
{
}
