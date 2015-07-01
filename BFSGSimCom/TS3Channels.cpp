#include <regex>
#include <string>
#include <map>

#include <ShlObj.h>

#include <SQLiteCpp\Transaction.h>

#include "TS3Channels.h"

using namespace std;

string TS3Channels::determineDbFileName(void)
{
    string retValue = "";

#if defined(_DEBUG)
    WCHAR* wpath;
    char cpath[_MAX_PATH];
    char defChar = ' ';

    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &wpath)))
    {
        int rc = WideCharToMultiByte(CP_ACP, 0, wpath, -1, cpath, _MAX_PATH, &defChar, NULL);
        retValue = string(cpath) + "\\bfsgsimcom.db3";
    }

    CoTaskMemFree(static_cast<void*>(wpath));
#else
    retValue = ":memory:";
#endif

    return retValue;
}


// Constructor for the TS3 channel class
TS3Channels::TS3Channels() : mDbFileName(determineDbFileName()), mDb(TS3Channels::mDbFileName, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE)
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
        "   frequency INT, "    \
        "   parent UNSIGNED BIG INT, "  \
        "   description CHAR(256)," \
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
        "insert into channels(channelId, frequency, parent, description) values (0, null, 0, 'Root');"
        "insert into closure(parent, child, depth) values (0, 0, 0);" \
        "";

    try
    {
        mDb.exec(aInitDatabase);
    }
    catch (SQLite::Exception& e)
    {
        e;
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
        frequency = uint16_t(100 * std::stod(matchedFrequency.str()) + 0.5);
    }

    return frequency;
}

// Define queries here - they get resolved at compile time...
const string TS3Channels::aAddInsertChannel = \
"insert into channels (channelId, frequency, parent, description)" \
"values" \
"(:channelId, :frequency, :parent, :description)" \
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
        (frequency) ? aChannelStmt.bind(":frequency", frequency) : aChannelStmt.bind(":frequency");
        aChannelStmt.bind(":parent", sqlite3_int64(parentChannel));
        aChannelStmt.bind(":description", channelName);
        aChannelStmt.exec();

        aClosureStmt1.bind(":child", sqlite3_int64(channelID));
        (frequency) ? aClosureStmt1.bind(":frequency", frequency) : aClosureStmt1.bind(":frequency");
        aClosureStmt1.exec();

        aClosureStmt2.bind(":parent", sqlite3_int64(parentChannel));
        aClosureStmt2.bind(":child", sqlite3_int64(channelID));
        (frequency) ? aClosureStmt2.bind(":frequency", frequency) : aClosureStmt2.bind(":frequency");
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
        SQLite::Transaction aTrans(mDb);

        SQLite::Statement aChannelStmt(mDb, aDeleteChannels);
        aChannelStmt.bind(":delete", sqlite3_int64(channelID));
        aChannelStmt.exec();

        SQLite::Statement aClosureStmt(mDb, aDeleteClosure);
        aClosureStmt.bind(":delete", sqlite3_int64(channelID));
        aClosureStmt.exec();

        aTrans.commit();
    }
    catch (SQLite::Exception& e)
    {
        e;
        retValue = mDb.getErrorCode();
    }

    return retValue;

}

void TS3Channels::deleteAllChannels(void)
{
    // Easier to just do this than play around...
    initDatabase();
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
    // Define this here - it gets resolved at compile time...
    // Default scenario is that we don't find a result
    uint64 retValue = CHANNEL_ID_NOT_FOUND;

    try
    {
        // Create the statement
        SQLite::Statement aStmt(mDb, aGetChannelFromFreqCurrPrnt);

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
    description = str;
}

const string TS3Channels::aGetChannelList = \
"with recursive r(channelId, depth, path, description) as " \
"( " \
"select channelId, 0, '', description from channels where channelId = :root " \
"union all " \
"select cl.child, r.depth + 1, r.path || printf(\" % 08p\", cl.child), ch.description " \
"from " \
"closure cl " \
"inner join channels ch on cl.child = ch.channelId " \
"inner join r on cl.parent = r.channelId " \
"where cl.depth = 1 " \
") " \
"select channelId, depth, description from r order by path; " \
";";

vector<TS3Channels::ChannelInfo> TS3Channels::getChannelList(uint64 root)
{
    vector<TS3Channels::ChannelInfo> retValue;

    try
    {
        // Create the statement
        SQLite::Statement aStmt(mDb, aGetChannelList);

        // Bind the variables
        aStmt.bind(":root", sqlite3_int64(root));

        while (aStmt.executeStep())
        // Execute the query, and if we get a result.
        {
            uint64 channelID;
            int depth;
            string description;

            // The query is written to return a single value in a single row...
            channelID = aStmt.getColumn(0).getInt64();
            depth = aStmt.getColumn(1).getInt();
            description = aStmt.getColumn(2).getText();

            ChannelInfo ch(channelID, depth, description);

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