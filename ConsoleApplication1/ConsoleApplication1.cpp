// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include <cstdint>

#include "public_definitions.h"

#include "stdafx.h"

#include "windows.h"
#include "FSUIPC_User.h"
#include "TS3Channels.h"

int _tmain(int argc, _TCHAR* argv[])
{
    TS3Channels tch;
    uint64 result;
    DWORD dwResult;

    uint16_t com1;

    if (FSUIPC_Open(SIM_ANY, &dwResult))
    {
        if (FSUIPC_Read(0x034e, 2, &com1, &dwResult))
        {
            FSUIPC_Process(&dwResult);
        }
    }

    tch.addOrUpdateChannel("Lobby", 1, 0);
    tch.addOrUpdateChannel("Fly-in 1", 2, 0);
    tch.addOrUpdateChannel("F1 Dep - 118.300", 3, 2);
    tch.addOrUpdateChannel("F1 Unicom - 122.800", 4, 2);
    tch.addOrUpdateChannel("F1 Dep - 119.125", 5, 2);
    tch.addOrUpdateChannel("Fly-in 2", 6, 0);
    tch.addOrUpdateChannel("F2 Departure", 7, 6);
    tch.addOrUpdateChannel("F2D Ground - 118.300", 8, 7);
    tch.addOrUpdateChannel("F2D Tower - 125.100", 9, 7);
    tch.addOrUpdateChannel("F2D Departure - 119.125", 10, 7);
    tch.addOrUpdateChannel("F2 EnRoute", 11, 6);
    tch.addOrUpdateChannel("F2 Unicom - 122.800", 12, 11);
    tch.addOrUpdateChannel("F2 Arrival", 13, 6);
    tch.addOrUpdateChannel("F2D Departure - 118.300", 14, 13);
    tch.addOrUpdateChannel("F2D Tower - 125.100", 15, 13);
    tch.addOrUpdateChannel("F2D Ground - 119.125", 16, 13);
    tch.addOrUpdateChannel("F2 Other - 118.300", 17, 6);
    tch.addOrUpdateChannel("F2 Other - 118.400", 18, 17);
    tch.addOrUpdateChannel("F2 Other - 118.300", 19, 18);


    result = tch.getChannelID(uint16_t(11830)); // should return 3 - first in the list
    result = tch.getChannelID(uint16_t(11830), 4); // should return 3 - first in the list with a common parent
    result = tch.getChannelID(uint16_t(11830), 9); // should return 8 - first in the list with a common parent
    result = tch.getChannelID(uint16_t(11830), 12); // should return 17 - first in the list with a common parent
    result = tch.getChannelID(uint16_t(11830), 15); // should return 14 - first in the list with a common parent
    result = tch.getChannelID(uint16_t(11830), 9, 0); //
    result = tch.getChannelID(uint16_t(11830), 15, 0);
    result = tch.getChannelID(uint16_t(11830), 9, 6); // should return 8
    result = tch.getChannelID(uint16_t(11830), 9, 15); // should return not found
    result = tch.getChannelID(uint16_t(11830), 9, 13); // should return not found
    result = tch.getChannelID(uint16_t(12280), 3, 2); // should return not found
    result = tch.getChannelID(uint16_t(12280), 9, 7); // should return not found
    result = tch.getChannelID(uint16_t(12280), 9, 6); // should return not found

    //tch.addOrUpdateChannel("F2D Tower2 - 125.100");
    
	return 0;
}

