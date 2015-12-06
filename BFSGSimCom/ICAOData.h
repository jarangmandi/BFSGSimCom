#pragma once

#include <string>
#include <vector>

#include <SQLiteCpp\Database.h>

using namespace ::std;

class ICAOData
{
private:
    string mIcaoDbFileName;
    SQLite::Database mIcaoDb;

    string determineIcaoDbFileName(void);

    static const string aGetChannelList;
    static const string aGetChannel;

public:
    struct Station
    {
        string icao;
        string type;
        int frequency;
        string name;
        double lat;
        double lon;

        ICAOData::Station::Station(string strIdent, string strType, int iFrequency, string strName, double dLat, double dLon);

    private:

    };

    vector<struct ICAOData::Station> ICAOData::getStationData(string strICAO);
    vector<struct ICAOData::Station> ICAOData::getStationData(string strICAO, string strType);

    ICAOData();
    ~ICAOData();
};

