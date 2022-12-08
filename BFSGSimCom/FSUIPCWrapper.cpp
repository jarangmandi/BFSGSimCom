#include <thread>
#include <chrono>
#include <sstream>

#include "FSUIPCWrapper.h"
#include "TS3Channels.h"

bool FSUIPCWrapper::cFSUIPCConnected = false;
bool FSUIPCWrapper::cRun = false;

FSUIPCWrapper::FSUIPCWrapper(void(*cb)(SimComData), SimComConfig* scc)
{
	DWORD dwResult;

	checkConnection(&dwResult);
	callback = cb;
	simComConfig = scc;
}

void FSUIPCWrapper::start(void)
{
	cRun = true;

	t1 = new std::thread(&FSUIPCWrapper::workerThread, FSUIPCWrapper(callback, simComConfig));
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
	BYTE simCom1833;
	BYTE simCom2833;
	BYTE simRadSw;
	WORD simOnGnd;
	int64_t simLatitude;
	int64_t simLongitude;

	DWORD simCom1_833;
	DWORD simCom2_833;
	DWORD simCom1s_833;
	DWORD simCom2s_833;

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

		FSUIPC_Read(0x0B47, 1, &simCom1833);
		FSUIPC_Read(0x0B48, 1, &simCom2833);

		FSUIPC_Read(0x05c4, 4, &simCom1_833);
		FSUIPC_Read(0x05c8, 4, &simCom2_833);
		FSUIPC_Read(0x05cc, 4, &simCom1s_833);
		FSUIPC_Read(0x05d0, 4, &simCom2s_833);

		if (FSUIPC_Process())
		{
			bool blComChanged = false;
			bool blPosChange = false;

			bool blOtherChanged = false;

			currentLat = double(simLatitude);
			currentLat *= 90.0 / (10001750.0 * 65536.0 * 65536.0);
			currentLon = double(simLongitude);
			currentLon *= 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);

			simIsXPlane =	((FSUIPC_FS_Version == SIM_FSX) && (FSUIPC_Version & 0xf0000000) == 0x50000000) ||
							FSUIPC_FS_Version >= SIM_CIX_XP;
			simIsP3D5 = (FSUIPC_FS_Version == SIM_P3D64) && (FSUIPC_Version & 0xf0000000) == 0x60000000;
			simIsMSFS = (FSUIPC_FS_Version == SIM_MSFS) && (FSUIPC_Version & 0xf0000000) == 0x70000000;

			simCom1Is833 = (simIsXPlane || simCom1833 || simIsP3D5 || simComConfig->blForce833) && !simComConfig->blForce25;
			simCom2Is833 = (simIsXPlane || simCom2833 || simIsP3D5 || simComConfig->blForce833) && !simComConfig->blForce25;

			if (simCom1Is833)
			{
				if (simCom1_833 != cCom1Freq833)
				{
					blComChanged = true;
					cCom1Freq833 = simCom1_833;
				}

				if (simCom1s_833 != cCom1Sby833)
				{
					blComChanged = true;
					cCom1Sby833 = simCom1s_833;
				}

				// Force a refresh if the 833 configuration changes
				cCom1Freq = 0xFFFFFFFF;
				cCom1Sby = 0xFFFFFFFF;
			}
			else
			{
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

				// Force a refresh if the 833 configuration changes
				cCom1Freq833 = 0xFFFFFFFF;
				cCom1Sby833 = 0xFFFFFFFF;
			}

			if (simCom2Is833)
			{
				if (simCom2_833 != cCom2Freq833)
				{
					blComChanged = true;
					cCom2Freq833 = simCom2_833;
				}

				if (simCom2s_833 != cCom2Sby833)
				{
					blComChanged = true;
					cCom2Sby833 = simCom2s_833;
				}

				// Force a refresh if the 833 configuration changes
				cCom2Freq = 0xFFFFFFFF;
				cCom2Sby = 0xFFFFFFFF;
			}
			else
			{
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

				// Force a refresh if the 833 configuration changes
				cCom2Freq833 = 0xFFFFFFFF;
				cCom2Sby833 = 0xFFFFFFFF;
			}

			if (cCom1Is833 != simCom1833 || cCom2Is833 != simCom2833)
			{
				blComChanged = true;
			}

			cCom1Is833 = simCom1833;
			cCom2Is833 = simCom2833;

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

			if (blComChanged || blPosChange || blOtherChanged || firstConnectedPass)
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

	// Depending if we're working on a 25KHz only radio (true) or an 8.333KHz radio (false)...
	if (simCom1Is833)
	{
		// No fancy stuff here (yet) - just pull the latest values.
		simcomdata.iCom1Freq = (int)(0.001 * cCom1Freq833);
		simcomdata.iCom1Sby = (int)(0.001 * cCom1Sby833);
	}
	else
	{
		simcomdata.iCom1Freq = 100000 + 10000 * ((cCom1Freq & 0xf000) >> 12) + 1000 * ((cCom1Freq & 0x0f00) >> 8) + 100 * ((cCom1Freq & 0x00f0) >> 4) + 10 * (cCom1Freq & 0x000f);
		simcomdata.iCom1Sby = 100000 + 10000 * ((cCom1Sby & 0xf000) >> 12) + 1000 * ((cCom1Sby & 0x0f00) >> 8) + 100 * ((cCom1Sby & 0x00f0) >> 4) + 10 * (cCom1Sby & 0x000f);

		// If it's a 25KHz frequency, then we might need to add the last digit.
		if (simcomdata.iCom1Freq % 50 == 20) simcomdata.iCom1Freq += 5;
		if (simcomdata.iCom1Sby % 50 == 20) simcomdata.iCom1Sby += 5;
	}

	if (simCom2Is833)
	{
		// No fancy stuff here (yet) - just pull the latest values.
		simcomdata.iCom2Freq = (int)(0.001 * cCom2Freq833);
		simcomdata.iCom2Sby = (int)(0.001 * cCom2Sby833);
	}
	else
	{
		simcomdata.iCom2Freq = 100000 + 10000 * ((cCom2Freq & 0xf000) >> 12) + 1000 * ((cCom2Freq & 0x0f00) >> 8) + 100 * ((cCom2Freq & 0x00f0) >> 4) + 10 * (cCom2Freq & 0x000f);
		simcomdata.iCom2Sby = 100000 + 10000 * ((cCom2Sby & 0xf000) >> 12) + 1000 * ((cCom2Sby & 0x0f00) >> 8) + 100 * ((cCom2Sby & 0x00f0) >> 4) + 10 * (cCom2Sby & 0x000f);

		// If it's a 25KHz frequency, then we might need to add the last digit.
		if (simcomdata.iCom2Freq % 50 == 20) simcomdata.iCom2Freq += 5;
		if (simcomdata.iCom2Sby % 50 == 20) simcomdata.iCom2Sby += 5;
	}
	// This is a kluge because XPUIPC doesn't report the correct channel and the config file that comes with it doesn't seem to work as advertised.
	// Look for a FSUIPC_Version with an most significant half word of 0x50000000 AND an FS Version of 8 (FSX).
	// FSUIPC 5 is specific to P3D which has an FS version of 10.
	if (simIsXPlane)
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

	simcomdata.blCom1833kHz = simCom1Is833;
	simcomdata.blCom2833kHz = simCom2Is833;

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
