#include <thread>
#include <chrono>
#include <sstream>

#include "FSUIPCWrapper.h"
#include "TS3Channels.h"

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

    int counter = 0;

	bool firstPass = true;

	bool firstDisconnectedPass = true;
	bool firstConnectedPass = true;
    
    while (cRun)
    {
		DWORD dwResult;
		checkConnection(&dwResult);

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
            bool blComChanged = false;
            bool blPosChange = false;
            
			bool blOtherChanged = false;

            currentLat = double(simLatitude);
            currentLat *= 90.0 / (10001750.0 * 65536.0 * 65536.0);
            currentLon = double(simLongitude);
            currentLon *= 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);

            if (simCom1 != cCom1Freq)
            {
                blComChanged = true;
                cCom1Freq = simCom1;
            }

            if (simCom1s != cCom1Sby)
            {
                blComChanged = true;
                cCom1Sby = simCom1s;
            }

            if (simCom2 != cCom2Freq)
            {
                blComChanged = true;
                cCom2Freq = simCom2;
            }

            if (simCom2s != cCom2Sby)
            {
                blComChanged = true;
                cCom2Sby = simCom2s;
            }

            if (simRadSw != cSelectedCom)
            {
                blComChanged = true;
                cSelectedCom = simRadSw;
            }

            if (simOnGnd != cWoW)
            {
                blOtherChanged = true;
                cWoW = simOnGnd;
            }

			// This is set to fire if we've moved more than 0.5nm from where we were the last time it fired,
			if (TS3Channels::getDistanceBetweenLatLonInNm(currentLat, currentLon, cLat, cLon) > 0.5)
            {
                blPosChange = true;

				// Reset counter and save the last position
				counter = 0;
				cLat = currentLat;
				cLon = currentLon;
            }

			// This makes sure we go through the callback at least every 10 seconds - is it redundant now?
			if (++counter >= 100)
			{
				blOtherChanged = true;
				counter = 0;
			}

            if (blComChanged || blPosChange || blOtherChanged || firstConnectedPass )
            {
                if (callback != NULL)
                {
                    (*callback)(getSimComData(blComChanged, blPosChange, blOtherChanged));
                }

				firstConnectedPass = false;
            }

			firstDisconnectedPass = true;

        }
		else
		{
			if (firstDisconnectedPass)
			{
				if (callback != NULL)
				{
					(*callback)(getSimComData(false, false, false));
				}

				firstDisconnectedPass = false;
			}

			::FSUIPC_Close();
			cFSUIPCConnected = false;

			firstConnectedPass = true;

		}

		// Fetch data from the simulator every 10th of a second
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
};


FSUIPCWrapper::SimComData FSUIPCWrapper::getSimComData(bool blComChanged, bool blPosChange, bool blOtherChanged) {
    
    SimComData simcomdata;

    simcomdata.iCom1Freq = 10000 + 1000 * ((cCom1Freq & 0xf000) >> 12) + 100 * ((cCom1Freq & 0x0f00) >> 8) + 10 * ((cCom1Freq & 0x00f0) >> 4) + (cCom1Freq & 0x000f);
    simcomdata.iCom1Sby = 10000 + 1000 * ((cCom1Sby & 0xf000) >> 12) + 100 * ((cCom1Sby & 0x0f00) >> 8) + 10 * ((cCom1Sby & 0x00f0) >> 4) + (cCom1Sby & 0x000f);
    simcomdata.iCom2Freq = 10000 + 1000 * ((cCom2Freq & 0xf000) >> 12) + 100 * ((cCom2Freq & 0x0f00) >> 8) + 10 * ((cCom2Freq & 0x00f0) >> 4) + (cCom2Freq & 0x000f);
    simcomdata.iCom2Sby = 10000 + 1000 * ((cCom2Sby & 0xf000) >> 12) + 100 * ((cCom2Sby & 0x0f00) >> 8) + 10 * ((cCom2Sby & 0x00f0) >> 4) + (cCom2Sby & 0x000f);

    // This is a kluge because XPUIPC doesn't report the correct channel and the config file that comes with it doesn't seem to work as advertised.
    // Look for a FSUIPC_Version with an most significant half word of 0x50000000 - who knows what will happen when FSUIPC catches up
    if ((FSUIPC_Version & 0xffff0000) == 0x50000000)
        simcomdata.selectedCom = ComRadio(((cSelectedCom & 0x80) ? None : Com1) + ((cSelectedCom & 0x40) ? None : Com2));
    else
        simcomdata.selectedCom = ComRadio(((cSelectedCom & 0x80) ? Com1 : None) + ((cSelectedCom & 0x40) ? Com2 : None));

    simcomdata.blComChanged = blComChanged;

    // Required for reporting...
    simcomdata.blWoW = (cWoW != 0);

    // And finally, report the aircraft position.
    simcomdata.dLat = currentLat;
    simcomdata.dLon = currentLon;

    simcomdata.blPosChanged = blPosChange;

	simcomdata.blOtherChanged = blOtherChanged;

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

//    if (checkConnection(&dwResult))
//    {
        try
        {
            retValue = ::FSUIPC_Read(dwOffset, dwSize, pDest, &dwResult);
        }
        catch (...)
        {
            retValue = FALSE;
        }
//    }

    cFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

    return retValue;
}

BOOL FSUIPCWrapper::FSUIPC_Write(DWORD dwOffset, DWORD dwSize, void* pSrc)
{
    BOOL retValue = 0;
    DWORD dwResult;

//    if (checkConnection(&dwResult))
//    {
        try
        {
            retValue = ::FSUIPC_Write(dwOffset, dwSize, pSrc, &dwResult);
        }
        catch (...)
        {
            retValue = FALSE;
        }
//    }

    cFSUIPCConnected = (dwResult != FSUIPC_ERR_NOTOPEN);

    return retValue;
}

BOOL FSUIPCWrapper::FSUIPC_Process()
{
    BOOL retValue = 0;
    DWORD dwResult = FSUIPC_ERR_NOTOPEN;

//    if (checkConnection(&dwResult))
//    {
        try
        {
            retValue = ::FSUIPC_Process(&dwResult);
        }
        catch (...)
        {
            retValue = FALSE;
        }
//    }

    cFSUIPCConnected = (dwResult == FSUIPC_ERR_OK);

    return retValue;
}

std::string FSUIPCWrapper::toString(FSUIPCWrapper::SimComData simComData)
{
    std::ostringstream ostr;

	ostr << "ComChanged : " << simComData.blComChanged;
	ostr << " | Switches: " << simComData.selectedCom;
	ostr << " | Com1: " << simComData.iCom1Freq << " / " << simComData.iCom1Sby;
    ostr << " | Com2: " << simComData.iCom2Freq << " / " << simComData.iCom2Sby;
	ostr << " | PosChanged : " << simComData.blPosChanged;
	ostr << " | Lat: " << simComData.dLat;
	ostr << " | Lon: " << simComData.dLon;
	ostr << " | WoW: " << simComData.blWoW;

    return ostr.str();
}



FSUIPCWrapper::~FSUIPCWrapper()
{
}
