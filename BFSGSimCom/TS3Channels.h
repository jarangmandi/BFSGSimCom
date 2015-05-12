#pragma once

#include <cstdlib>
#include <cstdint>

#include <map>
#include <string>

#include "public_definitions.h"

using namespace std;

class TS3Channels
{
    struct channelInfo
    {
        uint64 channelID;
        uint64 parentChannelID;
        string channelName;
        uint16_t frequency;
        map<uint64, channelInfo*> childChannels;

        channelInfo(uint16_t channelFrequency, uint64 channelID, uint64 parentChannelID, string channelName);
        ~channelInfo();
    };

private:
    map<uint16_t, channelInfo*> channelFrequencyMap;
    map<uint64, channelInfo*> channelIDMap;

    uint16_t getFrequencyFromString(string);

public:
    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    uint16_t addOrUpdateChannel(string, uint64, uint64 parentChannel = 0);
    uint64 deleteChannel(uint64);
    void deleteAllChannels(void);
    uint64 getChannelID(uint16_t);
    uint64 getChannelID(double);
};

