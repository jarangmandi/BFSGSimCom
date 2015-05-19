#pragma once

#include <cstdlib>
#include <cstdint>
#include <map>
#include <string>

#include <SQLiteCpp\Database.h>

#include "public_definitions.h"

using namespace ::std;

class TS3Channels
{
private:
    //struct channelInfo
    //{
    //    uint64 channelID;
    //    uint64 parentChannelID;
    //    string channelName;
    //    uint16_t frequency;

    //    channelInfo(uint16_t channelFrequency, uint64 channelID, uint64 parentChannelID, string channelName);

    //    bool operator <(const channelInfo& c)const { return channelID < c.channelID; }

    //};

    //map<uint16_t, channelInfo*> channelFrequencyMap;
    //map<uint64, channelInfo*> channelIDMap;

    uint16_t getFrequencyFromString(string);
    uint64 getParentChannel(uint64);

    SQLite::Database mDb;

    static string mDbFile;
    static const string aAddInsertChannel;
    static const string aAddInsertAncestor;
    static const string aAddInsertClosure1;
    static const string aAddInsertClosure2;
    static const string aGetChannelFromFreq;
    static const string aGetChannelFromFreqCurr;
    static const string aGetChannelFromFreqCurrPrnt;

    int TS3Channels::initDatabase();

public:
    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    uint16_t addOrUpdateChannel(string, uint64, uint64 parentChannel = 0);
    int deleteChannel(uint64);
    int deleteAllChannels(void);
    uint64 getChannelID(uint16_t frequency, uint64 current = 0, uint64 root = 0);
    uint64 getChannelID(double frequency, uint64 current = 0, uint64 root = 0);

    static class _init
    {
    public:
        _init();
    } _initialiser;
};

