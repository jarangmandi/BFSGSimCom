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
		bool blOtherChanged;
		bool bl833Capable;
    };

private:
    static bool cFSUIPCConnected;
    static bool cRun;

	bool simIs833Capable;
	bool simIsXPlane;

    DWORD cCom1Freq;
    DWORD cCom1Sby;
    DWORD cCom2Freq;
    DWORD cCom2Sby;
    BYTE cSelectedCom;
    WORD cWoW;
	DWORD cLatitude;
	DWORD cLongitude;
	DWORD cCom1Freq833;
	DWORD cCom1Sby833;
	DWORD cCom2Freq833;
	DWORD cCom2Sby833;
	double cLat;
	double cLon;

	double currentLat;
    double currentLon;

    void (*callback)(SimComData);

    BOOL checkConnection(DWORD*);
    void workerThread(void);
    
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

	SimComData FSUIPCWrapper::getSimComData(bool blComChanged, bool blPosChange, bool blOtherChanged);

    static std::string toString(FSUIPCWrapper::SimComData scd);
};

