#pragma once

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>

#include <SQLiteCpp\Database.h>

#include "public_definitions.h"

using namespace ::std;

class TS3Channels
{
private:
    static const string aAddInsertChannel;
    static const string aAddInsertClosure1;
    static const string aAddInsertClosure2;
    static const string aDeleteChannels;
    static const string aDeleteClosure;
    static const string aGetChannelFromFreqCurrPrnt;
    static const string aGetChannelList;

    // Ordering of these two is important... it defines what order they're initialized in by the constructor.
    string mDbFileName;
    SQLite::Database mDb;

    string determineDbFileName(void);
    int initDatabase(void);

    uint16_t getFrequencyFromString(string);

public:
    struct ChannelInfo
    {
        uint64 channelID;
        int depth;
        string description;

    public:
        ChannelInfo(uint64, int, string);
    };

    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    int deleteChannel(uint64);
    void deleteAllChannels(void);
    uint16_t addOrUpdateChannel(string, uint64, uint64 parentChannel = 0);
    uint64 getChannelID(uint16_t frequency, uint64 current = 0, uint64 root = 0);
    uint64 getChannelID(double frequency, uint64 current = 0, uint64 root = 0);

    vector<ChannelInfo> getChannelList(uint64 root = 0);

};

