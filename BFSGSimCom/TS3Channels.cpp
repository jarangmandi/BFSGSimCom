#include <regex>
#include <string>
#include <map>

#include <ShlObj.h>

#include <SQLiteCpp\Transaction.h>

#include "TS3Channels.h"

using namespace std;

string TS3Channels::mDbFile;
TS3Channels::_init TS3Channels::_initialiser;

TS3Channels::_init::_init()
{
#if defined(_DEBUG)
    WCHAR wpath[_MAX_PATH];
    char cpath[_MAX_PATH];
    char defChar = ' ';

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, wpath)))
    {
        int rc = WideCharToMultiByte(CP_ACP, 0, wpath, -1, cpath, _MAX_PATH, &defChar, NULL);
        TS3Channels::mDbFile = string(cpath) + "\\bfsgsimcom.db3";
    }
#else
    TS3Channels::mDbFile = ":memory:";
#endif
}

int TS3Channels::initDatabase()
{
    int retValue = SQLITE_OK;

    static const string createTables = \
        "DROP TABLE IF EXISTS channels;" \
        "DROP TABLE IF EXISTS closure;" \
        "CREATE TABLE IF NOT EXISTS channels(" \
                "   channelID UNSIGNED BIG INT PRIMARY KEY NOT NULL, "  \
                "   frequency INT, "    \
                "   parent UNSIGNED BIG INT, "  \
                "   description CHAR(256)" \
                "); " \
        "CREATE TABLE IF NOT EXISTS closure(" \
                "   parent UNSIGNED BIG INT NOT NULL," \
                "   child UNSIGNED BIG INT NOT NULL," \
                "   depth INT NOT NULL," \
                "   frequency" \
                ");" \
                "INSERT INTO closure (parent, child, depth) VALUES (0, 0, 0);"
        "";

    try
    {
        mDb.exec(createTables);
    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mDb.getErrorCode();
    }

    return retValue;
}


//TS3Channels::channelInfo::channelInfo(uint16_t frequency, uint64 channelID, uint64 parentID, string channelName)
//{
//    this->frequency = frequency;
//    this->channelID = channelID;
//    this->parentChannelID = parentID;
//    this->channelName = channelName;
//
//}

TS3Channels::TS3Channels() : mDb(TS3Channels::mDbFile, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE)
{
    initDatabase();
    deleteAllChannels();
}

TS3Channels::~TS3Channels()
{
}

int TS3Channels::deleteAllChannels(void)
{
    int retValue = SQLITE_OK;

    static const string deleteChannels = \
        "DELETE FROM channels;" \
        "";

    try
    {
        SQLite::Statement aStmt(mDb, deleteChannels);
        aStmt.exec();
    }
    catch (SQLite::Exception&)
    {
        retValue = mDb.getErrorCode();
    }

    return retValue;
}

uint16_t TS3Channels::getFrequencyFromString(string str)
{
    // A valid frequency is three digits (of which the first is a "1", and the second is a "1", a "2" or a "3"), a decimal point,
    // and then two more digits, the last of which is a "0", a "2", a "5" or a "7".
    static regex r("1[123]\\d\\.\\d[0257]");

    smatch matchedFrequency;
    uint16_t frequency = 0;

    // Look for a frequency in the string we were passed.
    if (std::regex_search(str, matchedFrequency, r)) {
        string str = matchedFrequency.str();
        frequency = uint16_t(100 * std::stod(matchedFrequency.str()));
    }

    return frequency;
}


uint64 TS3Channels::getParentChannel(uint64 channelID)
{
    uint64 retValue;

    static const string aSelectChFromFreq = \
        "SELECT parent FROM channels WHERE channelID = :channelID;" \
        "";

    try
    {
        SQLite::Statement aStmt(mDb, aSelectChFromFreq);
        aStmt.bind(":channelID", sqlite3_int64(channelID));
        if (aStmt.executeStep())
        {
            retValue = aStmt.getColumn(0).getInt64();
        }
        else
        {
            retValue = CHANNEL_ID_NOT_FOUND;
        }
    }
    catch (SQLite::Exception&)
    {
        uint64 retValue = CHANNEL_ID_NOT_FOUND;
    }

    return retValue;
}

const string TS3Channels::aAddInsertChannel = \
    "INSERT INTO channels (channelID, frequency, parent, description)" \
    "VALUES" \
    "(:channelId, :frequency, :parent, :description)" \
    ";" \
    "";

const string TS3Channels::aAddInsertClosure1 = \
"INSERT INTO closure (parent, child, depth, frequency)" \
"VALUES (:child1, :child2, 0, :frequency);";

const string TS3Channels::aAddInsertClosure2 = \
    "INSERT INTO closure (parent, child, depth, frequency)" \
    "SELECT p.parent, c.child, p.depth + c.depth + 1, :frequency " \
    "FROM closure p, closure c " \
    "WHERE p.child = :parent and c.parent = :child" \
    ";" \
    "";


uint16_t TS3Channels::addOrUpdateChannel(string channelName, uint64 channelID, uint64 parentChannel)
{

    uint16_t frequency;

    // First, delete the channel from the list
    deleteChannel(channelID);

    // Look for a frequency in the channel name we were passed
    frequency = getFrequencyFromString(channelName);

    int retValue = SQLITE_OK;

    try
    {
        SQLite::Transaction aTrans(mDb);

        SQLite::Statement aChannelStmt(mDb, aAddInsertChannel);
        SQLite::Statement aClosureStmt1(mDb, aAddInsertClosure1);
        SQLite::Statement aClosureStmt2(mDb, aAddInsertClosure2);

        aChannelStmt.bind(":channelId", sqlite3_int64(channelID));
        aChannelStmt.bind(":frequency", frequency);
        aChannelStmt.bind(":parent", sqlite3_int64(parentChannel));
        aChannelStmt.bind(":description", channelName);
        aChannelStmt.exec();

        aClosureStmt1.bind(":child1", sqlite3_int64(channelID));
        aClosureStmt1.bind(":child2", sqlite3_int64(channelID));
        aClosureStmt1.bind(":frequency", frequency);
        aClosureStmt1.exec();
        aClosureStmt2.bind(":parent", sqlite3_int64(parentChannel));
        aClosureStmt2.bind(":child", sqlite3_int64(channelID));
        aClosureStmt2.bind(":frequency", frequency);
        aClosureStmt2.exec();

        aTrans.commit();

    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mDb.getErrorCode();
    }
    catch (exception& e)
    {
        e;
        retValue = UINT16_MAX;
    }

    return retValue;

    //// Create a channel information entity
    //chInfo = new channelInfo(frequency, channelID, parentChannel, channelName);

    //// Save the channel info indexed by the channel ID
    //channelIDMap[channelID] = chInfo;

    //// If we found one, then
    //if (frequency != 0) {
    //    // Save the channel info, indexed by the frequency that we found.
    //    channelFrequencyMap[frequency] = chInfo;
    //}

    //return frequency;
}

int TS3Channels::deleteChannel(uint64 channelID)
{
    static const string deleteChannels = \
        "DELETE FROM channels WHERE channelID = :delete;" \
        "";

    int retValue = SQLITE_OK;

    try
    {
        SQLite::Statement aStmt(mDb, deleteChannels);
        aStmt.bind(":delete", sqlite3_int64(channelID));
        aStmt.exec();
    }
    catch (SQLite::Exception&)
    {
        retValue = mDb.getErrorCode();
    }

    return retValue;

    //// Assume we're not going to find the channel
    //uint64 retValue = CHANNEL_ID_NOT_FOUND;
    //channelInfo* chInfo = NULL;
    //uint16_t frequency = 0;

    //map<uint64, channelInfo*>::iterator chId = channelIDMap.end();
    //map<uint16_t, channelInfo*>::iterator chFreq = channelFrequencyMap.end();
    //    
    //chId = channelIDMap.find(channelID);

    //if (chId != channelIDMap.end())
    //{
    //    chInfo = chId->second;

    //    chFreq = channelFrequencyMap.find(chInfo->frequency);

    //    if (chFreq != channelFrequencyMap.end())
    //    {
    //        channelFrequencyMap.erase(chFreq);
    //    }

    //    channelIDMap.erase(chId);

    //    retValue = chInfo->channelID;
    //    delete chInfo;
    //}


    ////// Iterate across everything in the frequency map
    ////for (map<uint16_t, channelInfo>::iterator it = channelFrequencyMap.begin(); it != channelFrequencyMap.end(); it++) {

    ////    // Get the channel information from the iterator
    ////    channelInfo chInfo = channelInfo(it->second);

    ////    // If this is the channel we're looking for, then remove it from the map and exit the loop.
    ////    if (chInfo.channelID == channelID) {
    ////        channelMap.erase(it);
    ////        retValue = channelID;
    ////        break;
    ////    }
    ////}

    //return retValue;
}

const string TS3Channels::aGetChannelFromFreqCurrPrnt = \
    "select down.child from " \
    "(select * from closure where child = :current and parent in (select child from closure where parent = :root)) as up," \
    "(select * from closure where frequency = :frequency) as down "
    "where " \
    "up.parent = down.parent " \
    "order by up.depth + down.depth, down.depth desc " \
    "limit 1" \
    ";"\
    "";

uint64 TS3Channels::getChannelID(uint16_t frequency, uint64 current, uint64 root)
{
    uint64 retValue = CHANNEL_ID_NOT_FOUND;

    try
    {
        SQLite::Statement aStmt(mDb, aGetChannelFromFreqCurrPrnt);
        aStmt.bind(":frequency", frequency);
        aStmt.bind(":current", sqlite3_int64(current));
        aStmt.bind(":root", sqlite3_int64(root));

        if (aStmt.executeStep())
        {
            retValue = aStmt.getColumn(0).getInt64();
        }
        else
        {
            retValue = CHANNEL_ID_NOT_FOUND;
        }
    }
    catch (SQLite::Exception&)
    {
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