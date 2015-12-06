#include <thread>
#include <chrono>
#include <sstream>

#include "FSUIPCWrapper.h"

bool FSUIPCWrapper::cFSUIPCConnected = false;
bool FSUIPCWrapper::cRun = false;

FSUIPCWrapper::FSUIPCWrapper(void (*cb)(SimComData))
{
    DWORD dwResult;

    checkConnection(&dwResult);
    callback = cb;
}

void FSUIPCWrapper::start(void)
{
    cRun = true;

    t1 = new std::thread(&FSUIPCWrapper::workerThread, FSUIPCWrapper(callback));
}

void FSUIPCWrapper::stop(void)
{
    cRun = false;

    // There's no harm in calling this whether FSUIPC is open or not.
    ::FSUIPC_Close();

    if (t1 != NULL) t1->join();
}

void FSUIPCWrapper::workerThread(void)
{
    WORD simCom1;
    WORD simCom1s;
    WORD simCom2;
    WORD simCom2s;
    BYTE simRadSw;
    WORD simOnGnd;
    int64_t simLatitude;
    int64_t simLongitude;


    while (cRun)
    {
        FSUIPC_Read(0x034E, 2, &simCom1);
        FSUIPC_Read(0x3118, 2, &simCom2);
        FSUIPC_Read(0x311A, 2, &simCom1s);
        FSUIPC_Read(0x311C, 2, &simCom2s);
        FSUIPC_Read(0x3122, 1, &simRadSw);
        FSUIPC_Read(0x0366, 2, &simOnGnd);
        FSUIPC_Read(0x0560, 8, &simLatitude);
        FSUIPC_Read(0x0568, 8, &simLongitude);

        if (FSUIPC_Process())
        {
            bool blChanged = false;

            cLat = double(simLatitude);
            cLat *= 90.0 / (10001750.0 * 65536.0 * 65536.0);
            cLon = double(simLongitude);
            cLon *= 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);

            if (simCom1 != cCom1Freq)
            {
                blChanged = true;
                cCom1Freq = simCom1;
            }

            if (simCom1s != cCom1Sby)
            {
                blChanged = true;
                cCom1Sby = simCom1s;
            }

            if (simCom2 != cCom2Freq)
            {
                blChanged = true;
                cCom2Freq = simCom2;
            }

            if (simCom2s != cCom2Sby)
            {
                blChanged = true;
                cCom2Sby = simCom2s;
            }

            if (simRadSw != cSelectedCom)
            {
                blChanged = true;
                cSelectedCom = simRadSw;
            }

            if (simOnGnd != cWoW)
            {
                blChanged = true;
                cWoW = simOnGnd;
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

    simcomdata.iCom1Freq = 10000 + 1000 * ((cCom1Freq & 0xf000) >> 12) + 100 * ((cCom1Freq & 0x0f00) >> 8) + 10 * ((cCom1Freq & 0x00f0) >> 4) + (cCom1Freq & 0x000f);
    simcomdata.iCom1Sby = 10000 + 1000 * ((cCom1Sby & 0xf000) >> 12) + 100 * ((cCom1Sby & 0x0f00) >> 8) + 10 * ((cCom1Sby & 0x00f0) >> 4) + (cCom1Sby & 0x000f);
    simcomdata.iCom2Freq = 10000 + 1000 * ((cCom2Freq & 0xf000) >> 12) + 100 * ((cCom2Freq & 0x0f00) >> 8) + 10 * ((cCom2Freq & 0x00f0) >> 4) + (cCom2Freq & 0x000f);
    simcomdata.iCom2Sby = 10000 + 1000 * ((cCom2Sby & 0xf000) >> 12) + 100 * ((cCom2Sby & 0x0f00) >> 8) + 10 * ((cCom2Sby & 0x00f0) >> 4) + (cCom2Sby & 0x000f);

    // This is a kluge because XPUIPC doesn't report the correct channel and the config file that comes with it doesn't seem to work as advertised.
    // Look for a FSUIPC_Version of 0x50000006 - who knows what will happen when FSUIPC catches up
    if (FSUIPC_Version == 0x50000006)
        simcomdata.selectedCom = ComRadio(((cSelectedCom & 0x40) ? Com1 : None) + ((cSelectedCom & 0x80) ? Com2 : None));
    else
        simcomdata.selectedCom = ComRadio(((cSelectedCom & 0x80) ? Com1 : None) + ((cSelectedCom & 0x40) ? Com2 : None));

    simcomdata.blWoW = (cWoW != 0);

    return simcomdata;
};



// Check connection returns TRUE if connected, and FALSE if not...
BOOL FSUIPCWrapper::checkConnection(DWORD* pdwResult)
{
    BOOL retValue = 0;

    if (!cFSUIPCConnected)
    {
        try
        {
            retValue = ::FSUIPC_Open(SIM_ANY, pdwResult);
            cFSUIPCConnected = (retValue != FALSE) || (*pdwResult == FSUIPC_ERR_OPEN);
        }
        catch (...)
        {
            retValue = FALSE;
            cFSUIPCConnected = false;
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

    cFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

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

    cFSUIPCConnected = (dwResult != FSUIPC_ERR_NOTOPEN);

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

    cFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

    return retValue;
}

std::string FSUIPCWrapper::toString(FSUIPCWrapper::SimComData simComData)
{
    std::ostringstream ostr;

    ostr << "WoW: " << simComData.blWoW << " - ";
    ostr << "Switches: " << simComData.selectedCom << " - ";
    ostr << "Com1: " << simComData.iCom1Freq << " / " << simComData.iCom1Sby << " - ";
    ostr << "Com2: " << simComData.iCom2Freq << " / " << simComData.iCom2Sby;

    return ostr.str();
}



FSUIPCWrapper::~FSUIPCWrapper()
{
}
