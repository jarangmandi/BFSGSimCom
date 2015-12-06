#include "ICAOData.h"

#include <vector>


string ICAOData::determineIcaoDbFileName(void)
{
    return ".\\plugins\\BFSGSimComFiles\\BFSGSimCom.db";
}

ICAOData::ICAOData() :
    mIcaoDbFileName(determineIcaoDbFileName()),
    mIcaoDb(ICAOData::mIcaoDbFileName, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE)

{
}

ICAOData::Station::Station(string strIdent, string strType, int iFrequency, string strName, double dLat, double dLon)
{
    icao = strIdent;
    type = strType;
    frequency = iFrequency;
    name = strName;
    lat = dLat;
    lon = dLon;
}

const string ICAOData::aGetChannelList = \
"select " \
"   af.airport_ident, " \
"   af.type, " \
"   af.frequency, " \
"   a.name, " \
"   a.latitude, " \
"   a.longitude " \
"from " \
"   airportfrequencies af " \
"   left join airports a " \
"   on af.airport_ref = a.id " \
"where " \
"   af.airport_ident = :icao " \
";" \
"";

vector<struct ICAOData::Station> ICAOData::getStationData(string strICAO)
{
    vector<struct ICAOData::Station> retValue;

    try
    {
        // Create the statement
        SQLite::Statement aStmt(mIcaoDb, aGetChannelList);

        // Bind the variables
        aStmt.bind(":icao", strICAO);

        while (aStmt.executeStep())
            // Execute the query, and if we get a result.
        {
            string ident;
            string type;
            int frequency;
            string name;
            double lat;
            double lon;

            ident = string(aStmt.getColumn(0).getText());
            type = string(aStmt.getColumn(1).getText());
            frequency = aStmt.getColumn(2).getInt();
            name = string(aStmt.getColumn(3).getText());
            lat = aStmt.getColumn(4).getDouble();
            lon = aStmt.getColumn(5).getDouble();

            Station st(ident, type, frequency, name, lat, lon);

            retValue.push_back(st);
        }

    }
    catch (SQLite::Exception& e)
    {
        e;
        int a = 1;
    }
    catch (exception& e)
    {
        e;
    }
    

    return retValue;
}

const string ICAOData::aGetChannel = \
"select " \
"   af.airport_ident, " \
"   af.type, " \
"   af.frequency, " \
"   a.name, " \
"   a.latitude, " \
"   a.longitude " \
"from " \
"   airportfrequencies af " \
"   left join airports a " \
"   on af.airport_ref = a.id " \
"where " \
"   af.airport_ident = ':icao' " \
"   and " \
"   af.type = ':type'";

vector<struct ICAOData::Station> ICAOData::getStationData(string strICAO, string strType)
{
    vector<struct ICAOData::Station> retValue;

    return retValue;
}


ICAOData::~ICAOData()
{
}
