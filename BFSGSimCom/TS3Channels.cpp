#include <regex>
#include <string>
#include <map>
#include <sstream>

#include <ShlObj.h>

#include <SQLiteCpp\Transaction.h>

#include "TS3Channels.h"
#include "ICAOData.h"

using namespace std;

ICAOData* icaoData = NULL;

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
	icaoData = new ICAOData();
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
		"   depth int not null" \
		");" \
		"create table if not exists channelFrequency(" \
		"   channel UNSIGNED BIG INT NOT NULL," \
		"   frequency INT," \
		"   PRIMARY KEY (channel, frequency)," \
		"   FOREIGN KEY (channel) REFERENCES channels (channelId)"
		");" \
		"create unique index if not exists closureprntchld on closure(parent, child, depth);" \
		"create unique index if not exists closurechldprnt on closure(child, parent, depth);" \
		"create index parent_idx on channels(parent);" \
		"delete from channels;" \
		"delete from closure;" \
		"delete from channelFrequency;" \
        "insert into channels(channelId, latitude, longitude, range, parent, ordering, name, topic, description) values (0, null, null, null, 0, 0, 'Root', 'Root Channel', 'Root Channel');"
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

    sqlite3_create_function(mChanDb.getHandle(), "range", 4, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, &distanceFunc, NULL, NULL);

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

vector<uint16_t> TS3Channels::getFrequenciesFromString(string str1)
{
	const vector<string> strs = { str1 };
	return getFrequenciesFromStrings(strs);
}


vector<uint16_t> TS3Channels::getFrequenciesFromStrings(string str1, string str2, string str3)
{
	const vector<string> strs = { str1, str2, str3 };
	return getFrequenciesFromStrings(strs);
}


vector<uint16_t> TS3Channels::getFrequenciesFromStrings(const vector<string> &strs)
{
	// A valid frequency is three digits (of which the first is a "1", and the second is a "1", a "2" or a "3"), a decimal point,
	// and then two more digits, the last of which is a "0", a "2", a "5" or a "7".
	static regex r("1[123]\\d\\.\\d[0257]");

	smatch matchedFrequency;
	vector<uint16_t> frequency;

	//Iterating through each string in the vector we were sent.
	for (string str : strs)
	{
		// Look for all frequencies in the string.
		while (std::regex_search(str, matchedFrequency, r)) {
			frequency.push_back( uint16_t(100 * std::stod(matchedFrequency.str()) + 0.5) );
			str = matchedFrequency.suffix().str();
		}

		// If we found frequencies in the string we've just looked at then
		// don't look at any more.
		if (frequency.size() > 0) break;
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
    static regex r("[0-9A-Z]{3,7}_(GND|CLD|RCO|CTAF|TWR|RDO|ATF|AWOS|AFIS|ATIS|APP|ARR|DEP|CNTR)");

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
	static regex r("([NS](?:[1-8]?\\d(?:\\.\\d+)?|90(?:\\.0+)?))\\s*([EW](?:180(?:\\.0+)?|(?:(?:1[0-7]\\d)|(?:[1-9]?\\d))(?:\\.\\d+)?))");
    //static regex r("([+-]?(?:\\d+\\.?\\d*|\\d*\\.?\\d+))\\s+([+-]?(?:\\d+\\.?\\d*|\\d*\\.?\\d+))");

    tuple<double, double> retVal(999.9, 999.9);
    double tLat;
    double tLon;

    smatch matchedLatLon;

    for (string str : strs)
    {
        if (std::regex_search(str, matchedLatLon, r)) {
            string strLat = matchedLatLon[1]; // .str().substr(0, matchedLatLon[1]);
            string strLon = matchedLatLon[2]; // .str().substr(0, matchedLatLon[2]);
			tLat = ::stod(strLat.substr(1), NULL) * ((strLat.at(0) == 'S') ? -1 : 1);
            tLon = ::stod(strLon.substr(1), NULL) * ((strLon.at(0) == 'W') ? -1 : 1);

            retVal = ::make_tuple(tLat, tLon);

            break;
        }
    }

    return retVal;
}

string TS3Channels::concatFreqs(const vector<uint16_t>& freqs)
{
	stringstream retValue;

	for (uint16_t freq : freqs)
	{
		if (retValue.tellp() > 0) retValue << ", ";
		retValue << freq;
	}

	return retValue.str();
}



// Define queries here - they get resolved at compile time...
const string TS3Channels::aAddInsertChannelFrequency = \
"insert into channelFrequency(channel, frequency)" \
"values" \
"(:channel, :frequency)" \
";" \
"";

const string TS3Channels::aAddInsertChannel = \
"insert into channels (channelId, latitude, longitude, range, parent, ordering, name, topic, description)" \
"values" \
"(:channelId, :latitude, :longitude, :range, :parent, :order, :name, :topic, :desc)" \
";" \
"";

const string TS3Channels::aAddInsertClosure1 = \
"insert into closure (parent, child, depth)" \
"values (:child, :child, 0);";

const string TS3Channels::aAddInsertClosure2 = \
"insert into closure (parent, child, depth)" \
"select p.parent, c.child, p.depth + c.depth + 1 " \
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
    vector<uint16_t> frequencies;
    tuple<double, double> latlon;

    // First, delete the channel from the list
    deleteChannel(channelID);

    // Look for an ident, a frequency and a location, in the data we were passed
    ident = getAirportIdentFromStrings(cName, cTopic, cDesc);
    blIdentFromTS = (ident != "");

    frequencies = getFrequenciesFromStrings(cName, cTopic, cDesc);
    blFreqFromTS = (frequencies.size() != 0);

    latlon = getLatLonFromStrings(cName, cTopic, cDesc);
    ::tie(lat, lon) = latlon;
    blLatLonFromTS = (lat != 999.9) && (lon != 999.9);
    
    vector<ICAOData::Station> station = icaoData->getStationData(ident);

    if (station.size() > 0)
    {
		if (station[0].type == "GND")
			range = 10.0;
		else if (station[0].type == "CLD")
			range = 10.0;
		else if (station[0].type == "RCO")
			range = 10.0;
		else if (station[0].type == "CTAF")
			range = 50.0;
		else if (station[0].type == "TWR")
			range = 50.0;
		else if (station[0].type == "RDO")
			range = 50.0;
		else if (station[0].type == "ATF")
			range = 50.0;
		else if (station[0].type == "AWOS")
			range = 50.0;
		else if (station[0].type == "AFIS")
			range = 50.0;
		else if (station[0].type == "ATIS")
			range = 400.0;
		else if (station[0].type == "APP")
			range = 400.0;
		else if (station[0].type == "ARR")
			range = 400.0;
		else if (station[0].type == "DEP")
			range = 400.0;
		else if (station[0].type == "CNTR")
			range = 1000.0;
		else if (station[0].type == "OPS")
			range = 50.0;
		else if (station[0].type == "AFIS")
			range = 50.0;
        else
            range = 10800.0;

        if (!blFreqFromTS)
        {
			// Prepare a list of valid frequencies from all returned stations
			for (ICAOData::Station st : station)
			{
				if (st.frequency >= 11000 && st.frequency <= 13999) frequencies.push_back(st.frequency);
			}
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
        aChannelStmt.bind(":parent", sqlite3_int64(parentChannel));
        aChannelStmt.bind(":order", sqlite3_int64(order));
        aChannelStmt.bind(":name", cName);
        aChannelStmt.bind(":topic", cTopic);
        aChannelStmt.bind(":desc", cDesc);
        aChannelStmt.exec();

		// Add all of the frequencies into the channel frequency table.
		for (uint16_t frequency : frequencies)
		{
			SQLite::Statement aChannelFrequencyStmt(mChanDb, aAddInsertChannelFrequency);
			aChannelFrequencyStmt.bind(":channel", sqlite3_int64(channelID));
			(frequency) ? aChannelFrequencyStmt.bind(":frequency", frequency) : aChannelFrequencyStmt.bind(":frequency");

			try
			{
				aChannelFrequencyStmt.exec();
			}
			catch (SQLite::Exception e)
			{
				// The statement above will cause a primary key constraint violation if the same frequency is specified twice.
				// It's safe to ignore that, but anything else should be propagated.
				if (e.getErrorCode() != SQLITE_CONSTRAINT && e.getExtendedErrorCode() != SQLITE_CONSTRAINT_PRIMARYKEY)
				{
					throw(e);
				}
			}
		}

        aClosureStmt1.bind(":child", sqlite3_int64(channelID));
        aClosureStmt1.exec();

        aClosureStmt2.bind(":parent", sqlite3_int64(parentChannel));
        aClosureStmt2.bind(":child", sqlite3_int64(channelID));
		aClosureStmt2.exec();

        aTrans.commit();

        stringstream ssCommentary;
        ssCommentary << "ChannelID: " << channelID;
        ssCommentary << " | Name: " << cName;
        ssCommentary << " | Topic: " << cTopic;
        ssCommentary << " | Desc: " << cDesc;

        if (blFreqFromDb || blLatLonFromDb)
            ssCommentary << " | Ident: " << ident;

        if (blFreqFromDb)
            ssCommentary << " | DBFreq: " << concatFreqs(frequencies);
        else if (blFreqFromTS)
            ssCommentary << " | TSFreq: " << concatFreqs(frequencies);

        if (blLatLonFromDb)
            ssCommentary << " | DBLatLon: " << lat << "/" << lon;
        else if (blLatLonFromTS)
            ssCommentary << " | TSLatLon: " << lat << "/" << lon;

        strC = ssCommentary.str();
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


const string TS3Channels::aDeleteChannelFrequencies = \
"delete from channelFrequency where channel in (" \
"    select child from closure where parent = :delete" \
");" \
"";

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

		SQLite::Statement aChannelFrequencyStmt(mChanDb, aDeleteChannelFrequencies);
		aChannelFrequencyStmt.bind(":delete", sqlite3_int64(channelID));
		aChannelFrequencyStmt.exec();

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

const string TS3Channels::aChannelIsParentOfChild = \
"select parent from closure where parent = :root and child = :current;";

bool TS3Channels::channelIsUnderRoot(uint64 current, uint64 root)
{
	bool retValue = false;
	SQLite::Statement aStmt2(mChanDb, aChannelIsParentOfChild);

	aStmt2.bind(":current", sqlite3_int64(current));
	aStmt2.bind(":root", sqlite3_int64(root));

	if (aStmt2.executeStep())
	{
		// If the current channel is a child of the selected root, and we
		// got nothing back from the first query, we're untuned.
		retValue = true;
	}

	return retValue;
}


const string TS3Channels::aGetChannelFromFreqCurrPrnt = \
"with stations as( " \
"select down.child as channel, up.depth + down.depth as distance, down.depth as removed from " \
"(select * from closure where child = :current and parent in(select child from closure where parent = :root)) as up, " \
"(select * from closure as c inner join channelFrequency as cf on c.child = cf.channel and frequency = :frequency) as down " \
"where " \
"up.parent = down.parent " \
"order by up.depth + down.depth, down.depth desc " \
"), " \
"ranges as( " \
"select s.channel, s.distance, s.removed, c.range as max_range, ifnull(range(c.latitude, c.longitude, :lat, :lon), c.range) as range " \
"from stations as s " \
"left join channels as c " \
"on s.channel = c.channelId " \
") " \
"select r.channel, r.distance, r.removed, r.max_range, r.range, (r.range <= r.max_range) as in_range "
"from ranges r" \
"";

uint64 TS3Channels::getChannelID(uint16_t frequency, uint64 current, uint64 root, bool blConsiderRange, bool blOutOfRangeUntuned, double aLat, double aLon)
{
    // Define this here - it gets resolved at compile time...
    // Default scenario is that we don't find a result
    uint64 retValue = CHANNEL_ID_NOT_FOUND;

    try
    {
        string strQuery = aGetChannelFromFreqCurrPrnt +
            string ((blConsiderRange && blOutOfRangeUntuned) ? " where in_range = 1" : "") +
            " order by " +
            string ((blConsiderRange) ? "range, " : "") +
            "distance, removed;";

        // Create the statement
        SQLite::Statement aStmt(mChanDb, strQuery);

        // Bind the variables
        aStmt.bind(":frequency", frequency);
        aStmt.bind(":current", sqlite3_int64(current));
        aStmt.bind(":root", sqlite3_int64(root));
        aStmt.bind(":lat", aLat);
        aStmt.bind(":lon", aLon);

        // Execute the query, and if we get a result.
        if (aStmt.executeStep())
        {
            // The query is written to return the station to be tuned in the first row.
            retValue = aStmt.getColumn(0).getInt64();
        }
        else
        {
            if (channelIsUnderRoot(current, root))
            {
                // If the current channel is a child of the selected root, then
				// we can't find a matching channel ID, so say so.
                retValue = CHANNEL_ID_NOT_FOUND;
            }
            else
            {
				// If the current channel is not a child of the root, then flag
				// us as being outide of the root.
				retValue = CHANNEL_NOT_CHILD_OF_ROOT;
            }
        }
    }
    catch (SQLite::Exception&)
    {
        // If anything goes wrong (it shouldn't) then we've not found a frequency.
        retValue = CHANNEL_ID_NOT_FOUND;
    }

    return retValue;

}

// Returns the ID of the channel corresponding to a frequency provided as a double.
uint64 TS3Channels::getChannelID(double frequency, uint64 current, uint64 root, bool blConsiderRange, bool blOutOfRangeUntuned, double aLat, double aLon)
{
    // Need to get the rounding right
    // Try 128.300 to see why this is needed!
    return getChannelID(uint16_t(0.1 * round(1000 * frequency)), current, root, blConsiderRange, blOutOfRangeUntuned, aLat, aLon);
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

	double distance = getDistanceBetweenLatLonInNm(lat1, lon1, lat2, lon2);

	sqlite3_result_double(context, distance);

}

double TS3Channels::getDistanceBetweenLatLonInNm(double lat1, double lon1, double lat2, double lon2)
{
	// convert lat1 and lat2 into radians now, to avoid doing it twice below
	double lat1rad = DEG2RAD(lat1);
	double lat2rad = DEG2RAD(lat2);
	
	// apply the spherical law of cosines to our latitudes and longitudes, and set the result appropriately
	// 3437.746 is the approximate radius of the earth in nautical miles
	double result = acos(sin(lat1rad) * sin(lat2rad) + cos(lat1rad) * cos(lat2rad) * cos(DEG2RAD(lon2) - DEG2RAD(lon1))) * 3437.746;

	return result;
}
