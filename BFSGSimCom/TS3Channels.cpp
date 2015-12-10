#include <regex>
#include <string>
#include <map>

#include <ShlObj.h>

#include <SQLiteCpp\Transaction.h>

#include "TS3Channels.h"
#include "ICAOData.h"

using namespace std;

ICAOData icaoData;

string TS3Channels::determineChanDbFileName(void)
{
    string retValue = "";

#if defined(_DEBUG)
    WCHAR* wpath = NULL;
    char cpath[_MAX_PATH];
    char defChar = ' ';

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &wpath)))
    {
        int rc = WideCharToMultiByte(CP_ACP, 0, wpath, -1, cpath, _MAX_PATH, &defChar, NULL);
        retValue = string(cpath) + "\\bfsgsimcom.db3";

        CoTaskMemFree(static_cast<void*>(wpath));
    }

#else
    retValue = ":memory:";
#endif

    return retValue;
}

// Constructor for the TS3 channel class
TS3Channels::TS3Channels() :
    mChanDbFileName(determineChanDbFileName()),
    mChanDb(TS3Channels::mChanDbFileName, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE)
{
    initDatabase();
}

TS3Channels::~TS3Channels()
{
}

int TS3Channels::initDatabase()
{
    int retValue = SQLITE_OK;

    static const string aInitDatabase = \
        "DROP TABLE IF EXISTS channels;" \
        "DROP TABLE IF EXISTS closure;" \
        "CREATE TABLE IF NOT EXISTS channels(" \
        "   channelId UNSIGNED BIG INT PRIMARY KEY NOT NULL, "  \
        "   latitude DOUBLE, " \
        "   longitude DOUBLE, " \
        "   range DOUBLE, " \
        "   frequency INT, "    \
        "   parent UNSIGNED BIG INT, "  \
        "   ordering UNSIGNED BIG INT, "  \
        "   name CHAR(256)," \
        "   topic CHAR(256)," \
        "   description TEXT," \
        "   FOREIGN KEY (parent) REFERENCES channels (channelId)" \
        "); " \
        "create table if not exists closure(" \
        "   parent unsigned big int not null," \
        "   child unsigned big int not null," \
        "   depth int not null," \
        "   frequency" \
        ");" \
        "create unique index if not exists closureprntchld on closure(parent, child, depth);" \
        "create unique index if not exists closurechldprnt on closure(child, parent, depth);"
        "create index parent_idx on channels(parent);" \
        "insert into channels(channelId, latitude, longitude, range, frequency, parent, ordering, name, topic, description) values (0, null, null, null, null, 0, 0, 'Root', 'Root Channel', 'Root Channel');"
        "insert into closure(parent, child, depth) values (0, 0, 0);" \
        "";

    try
    {
        mChanDb.exec(aInitDatabase);
    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mChanDb.getErrorCode();
    }

    sqlite3_create_function(mChanDb.getHandle(), "distance", 4, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &distanceFunc, NULL, NULL);

    return retValue;
}


uint16_t TS3Channels::getFrequencyFromString(string str1)
{
    const vector<string> strs = { str1 };
    return getFrequencyFromStrings(strs);
}


uint16_t TS3Channels::getFrequencyFromStrings(string str1, string str2, string str3)
{
    const vector<string> strs = { str1, str2, str3 };
    return getFrequencyFromStrings(strs);
}


uint16_t TS3Channels::getFrequencyFromStrings(const vector<string> &strs)
{
    // A valid frequency is three digits (of which the first is a "1", and the second is a "1", a "2" or a "3"), a decimal point,
    // and then two more digits, the last of which is a "0", a "2", a "5" or a "7".
    static regex r("1[123]\\d\\.\\d[0257]");

    smatch matchedFrequency;
    uint16_t frequency = 0;

    for (string str : strs)
    {
        // Look for a frequency in the string we were passed.
        if (std::regex_search(str, matchedFrequency, r)) {
            string str = matchedFrequency.str();
            frequency = uint16_t(100 * std::stod(matchedFrequency.str()) + 0.5);
            break;
        }
    }

    return frequency;
}

string TS3Channels::getAirportIdentFromString(string str1)
{
    const vector<string> strs = { str1 };
    return getAirportIdentFromStrings(strs);
}


string TS3Channels::getAirportIdentFromStrings(string str1, string str2, string str3)
{
    const vector<string> strs = { str1, str2, str3 };
    return getAirportIdentFromStrings(strs);
}

string TS3Channels::getAirportIdentFromStrings(const vector<string> &strs)
{
    static regex r("[0-9A-Z]{3,7}_(TWR|GND|APP|DEP|CNTR|ATIS|INFO|CLD)");

    smatch matchedIdent;
    string ident = "";

    for (string str : strs)
    {
        if (std::regex_search(str, matchedIdent, r)) {
            ident = matchedIdent.str();
            break;
        }
    }

    return ident;
}

tuple<double, double> TS3Channels::getLatLonFromString(string str1)
{
    const vector<string> strs = { str1 };
    return getLatLonFromStrings(strs);
}

tuple<double, double> TS3Channels::getLatLonFromStrings(string str1, string str2, string str3)
{
    const vector<string> strs = { str1, str2, str3 };
    return getLatLonFromStrings(strs);
}

tuple<double, double> TS3Channels::getLatLonFromStrings(const vector<string>& strs)
{
    static regex r("([+-]?(?:\\d+\\.?\\d*|\\d*\\.?\\d+))\\s+([+-]?(?:\\d+\\.?\\d*|\\d*\\.?\\d+))");

    tuple<double, double> retVal(999.9, 999.9);
    double tLat;
    double tLon;

    smatch matchedLatLon;

    for (string str : strs)
    {
        if (std::regex_search(str, matchedLatLon, r)) {
            string strLat = matchedLatLon[1]; // .str().substr(0, matchedLatLon[1]);
            string strLon = matchedLatLon[2]; // .str().substr(0, matchedLatLon[2]);
            tLat = ::stod(strLat, NULL);
            tLon = ::stod(strLon, NULL);

            retVal = ::make_tuple(tLat, tLon);

            break;
        }
    }

    return retVal;
}



// Define queries here - they get resolved at compile time...
const string TS3Channels::aAddInsertChannel = \
"insert into channels (channelId, latitude, longitude, range, frequency, parent, ordering, name, topic, description)" \
"values" \
"(:channelId, :latitude, :longitude, :range, :frequency, :parent, :order, :name, :topic, :desc)" \
";" \
"";

const string TS3Channels::aAddInsertClosure1 = \
"insert into closure (parent, child, depth, frequency)" \
"values (:child, :child, 0, :frequency);";

const string TS3Channels::aAddInsertClosure2 = \
"insert into closure (parent, child, depth, frequency)" \
"select p.parent, c.child, p.depth + c.depth + 1, :frequency " \
"from closure p, closure c " \
"where p.child = :parent and c.parent = :child" \
";" \
"";

uint16_t TS3Channels::addOrUpdateChannel(string& strC, string cName, string cTopic, string cDesc, uint64 channelID, uint64 parentChannel, uint64 order)
{
    double lat;
    double lon;
    double range;
    string type;

    bool blIdentFromTS = false;
    bool blFreqFromTS = false;
    bool blLatLonFromTS = false;
    bool blFreqFromDb = false;
    bool blLatLonFromDb = false;

    string ident = "";
    uint16_t frequency = 0;
    tuple<double, double> latlon;

    // First, delete the channel from the list
    deleteChannel(channelID);

    // Look for an ident, a frequency and a location, in the data we were passed
    ident = getAirportIdentFromStrings(cName, cTopic, cDesc);
    blIdentFromTS = (ident != "");

    frequency = getFrequencyFromStrings(cName, cTopic, cDesc);
    blFreqFromTS = (frequency != 0);

    latlon = getLatLonFromStrings(cName, cTopic, cDesc);
    ::tie(lat, lon) = latlon;
    blLatLonFromTS = (lat != 999.9) && (lon != 999.9);
    
    vector<ICAOData::Station> station = icaoData.getStationData(ident);

    if (station.size() > 0)
    {
        if (station[0].type == "TWR")
            range = 50.0;
        else if (station[0].type == "GND")
            range = 10.0;
        else if (station[0].type == "APP")
            range = 400.0;
        else if (station[0].type == "DEP")
            range = 400.0;
        else if (station[0].type == "CNTR")
            range = 1000.0;
        else if (station[0].type == "ATIS")
            range = 400.0;
        else if (station[0].type == "INFO")
            range = 50.0;
        else if (station[0].type == "CLD")
            range = 10.0;
        else
            range = 10800.0;

        if (!blFreqFromTS)
        {
            frequency = station[0].frequency;
            blFreqFromDb = true;
        }
            
        if (!blLatLonFromTS)
        {
            if (station[0].lat != 999.9 && station[0].lon != 999.9)
            {
                lat = station[0].lat;
                lon = station[0].lon;
                blLatLonFromDb = true;
            }
        }
    }
    else
    {
        range = 10800.0;
    }

    int retValue = SQLITE_OK;

    try
    {
        SQLite::Transaction aTrans(mChanDb);

        SQLite::Statement aChannelStmt(mChanDb, aAddInsertChannel);
        SQLite::Statement aClosureStmt1(mChanDb, aAddInsertClosure1);
        SQLite::Statement aClosureStmt2(mChanDb, aAddInsertClosure2);

        aChannelStmt.bind(":channelId", sqlite3_int64(channelID));
        (lon != 999.9) ? aChannelStmt.bind(":latitude", lat) : aChannelStmt.bind(":latitude");
        (lat != 999.9) ? aChannelStmt.bind(":longitude", lon) : aChannelStmt.bind(":longitude");
        aChannelStmt.bind(":range", range);
        (frequency) ? aChannelStmt.bind(":frequency", frequency) : aChannelStmt.bind(":frequency");
        aChannelStmt.bind(":parent", sqlite3_int64(parentChannel));
        aChannelStmt.bind(":order", sqlite3_int64(order));
        aChannelStmt.bind(":name", cName);
        aChannelStmt.bind(":topic", cTopic);
        aChannelStmt.bind(":desc", cDesc);
        aChannelStmt.exec();

        aClosureStmt1.bind(":child", sqlite3_int64(channelID));
        (frequency) ? aClosureStmt1.bind(":frequency", frequency) : aClosureStmt1.bind(":frequency");
        aClosureStmt1.exec();

        aClosureStmt2.bind(":parent", sqlite3_int64(parentChannel));
        aClosureStmt2.bind(":child", sqlite3_int64(channelID));
        (frequency) ? aClosureStmt2.bind(":frequency", frequency) : aClosureStmt2.bind(":frequency");
        aClosureStmt2.exec();

        aTrans.commit();

        string strCommentary = "";
        strCommentary = strCommentary + "ChannelID: " + to_string(channelID);
        strCommentary = strCommentary + " | Name: " + cName;
        strCommentary = strCommentary + " | Topic: " + cTopic;
        strCommentary = strCommentary + " | Desc: " + cDesc;

        if (blFreqFromDb || blLatLonFromDb)
            strCommentary = strCommentary + " | Ident: " + ident;

        if (blFreqFromDb)
            strCommentary = strCommentary + " | DBFreq: " + to_string(frequency);
        else if (blFreqFromTS)
            strCommentary = strCommentary + " | TSFreq: " + to_string(frequency);

        if (blLatLonFromDb)
            strCommentary = strCommentary + " | DBLatLon: " + to_string(lat) + "/" + to_string(lon);
        else if (blLatLonFromTS)
            strCommentary = strCommentary + " | TSLatLon: " + to_string(lat) + "/" + to_string(lon);

        strC = strCommentary;
    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mChanDb.getErrorCode();
    }
    catch (exception& e)
    {
        e;
        retValue = UINT16_MAX;
    }

    return retValue;
}

const string TS3Channels::aDeleteChannels = \
"delete from channels where channelId in (" \
"    select child from closure where parent = :delete" \
");" \
"";

const string TS3Channels::aDeleteClosure = \
"delete from closure where rowid in (" \
"    select distinct link.rowid " \
"    from closure p, closure link, closure c, closure to_delete " \
"    where " \
"        p.parent = link.parent and " \
"        c.child = link.child and " \
"        p.child = to_delete.parent and " \
"        c.parent = to_delete.child and " \
"        (to_delete.parent = :delete or to_delete.child = :delete) " \
"    union " \
"    select rowid " \
"    from closure " \
"    where  " \
"        parent in (" \
"        select child from closure where parent = :delete " \
"        ) " \
");" \
"";

int TS3Channels::deleteChannel(uint64 channelID)
{
    int retValue = SQLITE_OK;

    try
    {
        SQLite::Transaction aTrans(mChanDb);

        SQLite::Statement aChannelStmt(mChanDb, aDeleteChannels);
        aChannelStmt.bind(":delete", sqlite3_int64(channelID));
        aChannelStmt.exec();

        SQLite::Statement aClosureStmt(mChanDb, aDeleteClosure);
        aClosureStmt.bind(":delete", sqlite3_int64(channelID));
        aClosureStmt.exec();

        aTrans.commit();
    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mChanDb.getErrorCode();
    }

    return retValue;

}

void TS3Channels::deleteAllChannels(void)
{
    // Easier to just do this than play around...
    initDatabase();
}

const string TS3Channels::aGetChannelFromFreqCurrPrnt = \
//"select down.child from " \
//"(select * from closure where child = :current and parent in (select child from closure where parent = :root)) as up," \
//"(select * from closure where frequency = :frequency) as down "
//"where " \
//"up.parent = down.parent " \
//"order by up.depth + down.depth, down.depth desc " \
//"limit 1" \
//";"\
//"";
"with stations as( " \
"select down.child as channel, up.depth + down.depth as distance, down.depth as removed from " \
"(select * from closure where child = :current and parent in(select child from closure where parent = :root)) as up, " \
"(select * from closure where frequency = :frequency) as down " \
"where " \
"up.parent = down.parent " \
"order by up.depth + down.depth, down.depth desc " \
") " \
"select s.channel, s.distance, s.removed, c.latitude, c.longitude " \
"from stations as s " \
"left join channels as c " \
"on s.channel = c.channelId " \
";" \
"";


uint64 TS3Channels::getChannelID(uint16_t frequency, uint64 current, uint64 root)
{
    // Define this here - it gets resolved at compile time...
    // Default scenario is that we don't find a result
    uint64 retValue = CHANNEL_ID_NOT_FOUND;

    try
    {
        // Create the statement
        SQLite::Statement aStmt(mChanDb, aGetChannelFromFreqCurrPrnt);

        // Bind the variables
        aStmt.bind(":frequency", frequency);
        aStmt.bind(":current", sqlite3_int64(current));
        aStmt.bind(":root", sqlite3_int64(root));

        // Execute the query, and if we get a result.
        if (aStmt.executeStep())
        {
            // The query is written to return a single value in a single row...
            retValue = aStmt.getColumn(0).getInt64();
        }
        else
        {
            // With no results, reiterate the return value.
            retValue = CHANNEL_ID_NOT_FOUND;
        }
    }
    catch (SQLite::Exception&)
    {
        // If anything goes wrong (it shouldn't) then we've not found a frequency.
        uint64 retValue = CHANNEL_ID_NOT_FOUND;
    }

    return retValue;

}

// Returns the ID of the channel corresponding to a frequency provided as a double.
uint64 TS3Channels::getChannelID(double frequency, uint64 current, uint64 root)
{
    // Need to get the rounding right
    // Try 128.300 to see why this is needed!
    return getChannelID(uint16_t(0.1 * round(1000 * frequency)), current, root);
}


TS3Channels::ChannelInfo::ChannelInfo(uint64 ch, int d, string str)
{
    channelID = ch;
    depth = d;
    name = str;
}


const string TS3Channels::aGetChannelList = \
"with recursive sort(channelId, parentId, indx, ordering, name) as " \
"( " \
"    select channelId, parent, 0, ordering, name from channels where channelId = 0 " \
"    union " \
"    select ch.channelId, ch.parent, sort.indx + 1, ch.ordering, ch.name " \
"    from channels ch " \
"    inner join sort on ch.ordering = sort.channelId " \
"    where ch.channelId <> 0 " \
"    limit 1000000 " \
"), " \
"tree(channelId, depth, path, name) as " \
"( " \
"    select channelId, 0, printf(\"%08p\", indx), name from sort where channelId = :root " \
"    union " \
"    select ch.channelId, tree.depth + 1, tree.path || printf(\" %08p\", ch.indx), ch.name " \
"    from " \
"    sort ch " \
"    inner join tree on ch.parentId = tree.channelId " \
"    where ch.channelId <> 0 " \
"    limit 1000000 " \
") " \
"select channelId, depth, name from tree order by path; " \
"";

vector<TS3Channels::ChannelInfo> TS3Channels::getChannelList(uint64 root)
{
    vector<TS3Channels::ChannelInfo> retValue;

    try
    {
        // Create the statement
        SQLite::Statement aStmt(mChanDb, aGetChannelList);

        // Bind the variables
        aStmt.bind(":root", sqlite3_int64(root));

        while (aStmt.executeStep())
        // Execute the query, and if we get a result.
        {
            uint64 channelID;
            int depth;
            string name;

            // The query is written to return a single value in a single row...
            channelID = aStmt.getColumn(0).getInt64();
            depth = aStmt.getColumn(1).getInt();
            name = aStmt.getColumn(2).getText();

            ChannelInfo ch(channelID, depth, name);

            retValue.push_back(ch);
        }
    }
    catch (SQLite::Exception& e)
    {
        e;
    }
    catch (exception& e)
    {
        e;
    }
    return retValue;
}

#define DEG2RAD(degrees) (degrees * 0.01745327) // degrees * pi over 180

void TS3Channels::distanceFunc(sqlite3_context *context, int argc, sqlite3_value **argv)
{
    // check that we have four arguments (lat1, lon1, lat2, lon2)
//    assert(argc == 4);
    // check that all four arguments are non-null
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || sqlite3_value_type(argv[1]) == SQLITE_NULL || sqlite3_value_type(argv[2]) == SQLITE_NULL || sqlite3_value_type(argv[3]) == SQLITE_NULL) {
        sqlite3_result_null(context);
        return;
    }
    // get the four argument values
    double lat1 = sqlite3_value_double(argv[0]);
    double lon1 = sqlite3_value_double(argv[1]);
    double lat2 = sqlite3_value_double(argv[2]);
    double lon2 = sqlite3_value_double(argv[3]);
    // convert lat1 and lat2 into radians now, to avoid doing it twice below
    double lat1rad = DEG2RAD(lat1);
    double lat2rad = DEG2RAD(lat2);
    // apply the spherical law of cosines to our latitudes and longitudes, and set the result appropriately
    // 6378.1 is the approximate radius of the earth in kilometres
    sqlite3_result_double(context, acos(sin(lat1rad) * sin(lat2rad) + cos(lat1rad) * cos(lat2rad) * cos(DEG2RAD(lon2) - DEG2RAD(lon1))) * 6378.1);
}
