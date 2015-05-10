// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include <cstdint>

#include "public_definitions.h"

#include "stdafx.h"

#include "TS3Channels.h"

int _tmain(int argc, _TCHAR* argv[])
{
    TS3Channels tch;
    uint64 result;

    tch.addOrUpdateChannel("122.80 - Unicom",2);
    tch.addOrUpdateChannel("118.30 - Tauranga Tower", 3);
    tch.addOrUpdateChannel("123.450 - Andyville", 4);
    tch.addOrUpdateChannel("123.425 - Missed A", 5);
    tch.addOrUpdateChannel("Auckland Information - 121.10", 6);
    tch.addOrUpdateChannel("Test - 234.45", 7);
    tch.addOrUpdateChannel("Test2 - 123.46", 8);

    result = tch.getChannelID(uint16_t(12345));
    result = tch.getChannelID(uint16_t(11830));
    result = tch.getChannelID(122.80);
    result = tch.getChannelID(123.42);
    result = tch.getChannelID(1.);
    result = tch.getChannelID(uint16_t(12110));

    if (result == TS3Channels::CHANNEL_ID_NOT_FOUND) printf("Ha");

	return 0;
}
