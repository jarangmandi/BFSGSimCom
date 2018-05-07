#pragma once

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <string>

#include <SQLiteCpp\Database.h>
#include <sqlite3.h>

#include "teamspeak/public_definitions.h"

#include "ICAOData.h"

using namespace ::std;

extern ICAOData* icaoData;

class TS3Channels
{
private:
	static const string aAddInsertChannelFrequency;
    static const string aAddInsertChannel;
    static const string aAddInsertClosure1;
    static const string aAddInsertClosure2;
	static const string aUpdateChannelDescription;
	static const string TS3Channels::aDeleteChannelFrequencies;
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
	vector<uint16_t> getFrequenciesFromString(string);
	vector<uint16_t> getFrequenciesFromStrings(string, string, string);
	vector<uint16_t> getFrequenciesFromStrings(const vector<string>&);
	string TS3Channels::getAirportIdentFromString(string);
    string TS3Channels::getAirportIdentFromStrings(string, string, string);
    string TS3Channels::getAirportIdentFromStrings(const vector<string>&);
    tuple<double, double> TS3Channels::getLatLonFromString(string);
    tuple<double, double> TS3Channels::getLatLonFromStrings(string, string, string);
    tuple<double, double> TS3Channels::getLatLonFromStrings(const vector<string>&);
	string TS3Channels::concatFreqs(const vector<uint16_t>& freqs);

public:
    struct ChannelInfo
    {
        uint64 channelID;
        int depth;
        string name;

    public:
        ChannelInfo(uint64, int, string);
	};

	struct StationInfo
	{
		uint64 ch;
		double lat;
		double lon;
		double range;
		double maxRange;
		bool in_range;
		string id;

	public:
		StationInfo();
		StationInfo(uint64);
		StationInfo(uint64, double, double, double, double, bool, string);
		bool operator==(const StationInfo&) const;
		bool operator!=(const StationInfo&) const;
	};

    TS3Channels();
    ~TS3Channels();

    static const uint64 CHANNEL_ROOT = 0;
    static const uint64 CHANNEL_NOT_CHILD_OF_ROOT = UINT64_MAX - 1;
    static const uint64 CHANNEL_ID_NOT_FOUND = UINT64_MAX;

    int deleteChannel(uint64);
    void deleteAllChannels(void);
    uint16_t addOrUpdateChannel(string& str, string, string, string, uint64, uint64 parentChannel = 0, uint64 order = 0);
	int updateChannelDescription(string& str, uint64, string);
	TS3Channels::StationInfo getChannelID(uint16_t frequency, uint64 current = 0, uint64 root = 0, bool blConsiderRange = false, bool blOutOfRangeUntuned = false, double lat = -999.9, double lon = -999.0);
	TS3Channels::StationInfo getChannelID(double frequency, uint64 current = 0, uint64 root = 0, bool blConsiderRange = false, bool blOutOfRangeUntuned = false, double lat = -999.9, double lon = -999.0);
	bool TS3Channels::channelIsUnderRoot(uint64 current, uint64 root);

    vector<ChannelInfo> getChannelList(uint64 root = 0);

    static void TS3Channels::distanceFunc(sqlite3_context *context, int argc, sqlite3_value **argv);
	static double TS3Channels::getDistanceBetweenLatLonInNm(double lat1, double lon1, double lat2, double lon2);
};
