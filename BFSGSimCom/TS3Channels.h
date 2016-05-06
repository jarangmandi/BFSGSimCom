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
    static const string aChannelIsParentOfChild;
    static const string aInitChannelList;
    static const string aGetChannelList;

    // Ordering of these two is important... it defines what order they're initialized in by the constructor.
    string mChanDbFileName;
    SQLite::Database mChanDb;


    string determineChanDbFileName(void);

    int initDatabase(void);

    uint16_t getFrequencyFromString(string);
    uint16_t getFrequencyFromStrings(string, string, string);
    uint16_t getFrequencyFromStrings(const vector<string>&);
    string TS3Channels::getAirportIdentFromString(string);
    string TS3Channels::getAirportIdentFromStrings(string, string, string);
    string TS3Channels::getAirportIdentFromStrings(const vector<string>&);
    tuple<double, double> TS3Channels::getLatLonFromString(string);
    tuple<double, double> TS3Channels::getLatLonFromStrings(string, string, string);
    tuple<double, double> TS3Channels::getLatLonFromStrings(const vector<string>&);

public:
    struct ChannelInfo
    {
        uint64 channelID;
        int depth;
        string name;

    public:
        ChannelInfo(uint64, int, string);
    };

    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ROOT = 0;
    static const uint64 CHANNEL_NOT_CHILD_OF_ROOT = UINT64_MAX - 1;
    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    int deleteChannel(uint64);
    void deleteAllChannels(void);
    uint16_t addOrUpdateChannel(string& str, string, string, string, uint64, uint64 parentChannel = 0, uint64 order = 0);
    uint64 getChannelID(uint16_t frequency, uint64 current = 0, uint64 root = 0, bool blConsiderRange = false, bool blOutOfRangeUntuned = false, double lat = -999.9, double lon = -999.0);
    uint64 getChannelID(double frequency, uint64 current = 0, uint64 root = 0, bool blConsiderRange = false, bool blOutOfRangeUntuned = false, double lat = -999.9, double lon = -999.0);

    vector<ChannelInfo> getChannelList(uint64 root = 0);

    static void TS3Channels::distanceFunc(sqlite3_context *context, int argc, sqlite3_value **argv);
};

