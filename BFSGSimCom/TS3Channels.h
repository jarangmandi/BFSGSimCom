#pragma once

#include <cstdlib>
#include <cstdint>
#include <map>
#include <string>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>

#include "public_definitions.h"

using namespace ::std;

using namespace ::boost;
using namespace ::boost::multi_index;



class TS3Channels
{

    struct channelInfo
    {
        uint64 channelID;
        uint64 parentChannelID;
        string channelName;
        uint16_t frequency;

        channelInfo(uint16_t channelFrequency, uint64 channelID, uint64 parentChannelID, string channelName);

        bool operator<(const channelInfo& c)const{ return channelID < c.channelID; }
    };

    typedef multi_index_container<
        channelInfo,
        indexed_by<
        ordered_unique<::boost::multi_index::identity<channelInfo>>,
        ordered_non_unique<member<channelInfo,uint16_t, &channelInfo::frequency>>
        >
    > channelInfo_set;

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

