#pragma once

#include <cstdlib>
#include <cstdint>
#include <map>
#include <string>

#include "public_definitions.h"

using namespace std;

class TS3Channels
{
private:
    map<uint16_t, uint64> channelMap;

    uint16_t getFrequencyFromString(string);

public:
    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    uint16_t addOrUpdateChannel(string, uint64);
    uint64 deleteChannel(uint64);
    void deleteAllChannels(void);
    uint64 getChannelID(uint16_t);
    uint64 getChannelID(double);
};

