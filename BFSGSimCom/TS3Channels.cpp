#include "TS3Channels.h"

#include <regex>
#include <string>
#include <map>

using namespace std;

TS3Channels::channelInfo::channelInfo(uint16_t frequency, uint64 channelID, uint64 parentID, string channelName)
{
    this->frequency = frequency;
    this->channelID = channelID;
    this->parentChannelID = parentID;
    this->channelName = channelName;
    this->childChannels.empty();
}

TS3Channels::channelInfo::~channelInfo()
{
    this->childChannels.empty();
}

TS3Channels::TS3Channels()
{
    deleteAllChannels();
    addOrUpdateChannel("Root", 0, CHANNEL_ID_NOT_FOUND);
}

TS3Channels::~TS3Channels()
{
}

void TS3Channels::deleteAllChannels(void)
{
    channelFrequencyMap.clear();
    channelIDMap.clear();
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

uint16_t TS3Channels::addOrUpdateChannel(string channelName, uint64 channelID, uint64 parentChannel)
{
    uint64 currentParent;
    uint16_t frequency;
    channelInfo* chInfo;

    // First, delete the channel from the lists
    chInfo = channelIDMap[channelID];
    currentParent = chInfo->parentChannelID;

    if (currentParent != CHANNEL_ID_NOT_FOUND)
    {
        map<uint64, channelInfo*> chList = channelIDMap[parentChannel]->childChannels;
        chList.erase(chList.find(channelID));
    }

    deleteChannel(channelID);

    // Look for a frequency in the channel name we were passed
    frequency = getFrequencyFromString(channelName);

    // Create a channel information entity
    chInfo = new channelInfo(frequency, channelID, parentChannel, channelName);

    // Save the channel info indexed by the channel ID
    channelIDMap[channelID] = chInfo;

    // Add this channel to the list of its parent's children
    if (parentChannel != CHANNEL_ID_NOT_FOUND)
        channelIDMap[parentChannel]->childChannels[channelID] = chInfo;

    // If we found one, then
    if (frequency != 0) {
        // Save the channel info, indexed by the frequency that we found.
        channelFrequencyMap[frequency] = chInfo;
    }

    return frequency;
}

uint64 TS3Channels::deleteChannel(uint64 channelID)
{
    // Assume we're not going to find the channel
    uint64 retValue = CHANNEL_ID_NOT_FOUND;
    channelInfo* chInfo = NULL;
    uint16_t frequency = 0;

    map<uint64, channelInfo*>::iterator chId = channelIDMap.end();
    map<uint16_t, channelInfo*>::iterator chFreq = channelFrequencyMap.end();
        
    chId = channelIDMap.find(channelID);

    if (chId != channelIDMap.end())
    {
        chInfo = chId->second;

        chFreq = channelFrequencyMap.find(chInfo->frequency);

        if (chFreq != channelFrequencyMap.end())
        {
            channelFrequencyMap.erase(chFreq);
        }

        channelIDMap.erase(chId);

        retValue = chInfo->channelID;
        delete chInfo;
    }


    //// Iterate across everything in the frequency map
    //for (map<uint16_t, channelInfo>::iterator it = channelFrequencyMap.begin(); it != channelFrequencyMap.end(); it++) {

    //    // Get the channel information from the iterator
    //    channelInfo chInfo = channelInfo(it->second);

    //    // If this is the channel we're looking for, then remove it from the map and exit the loop.
    //    if (chInfo.channelID == channelID) {
    //        channelMap.erase(it);
    //        retValue = channelID;
    //        break;
    //    }
    //}

    return retValue;
}

uint64 TS3Channels::getChannelID(uint16_t frequency)
{
    uint64 returnValue;

    // Get the channel ID from the map - throws an exception if it's not foun.
    try
    {
        returnValue = channelFrequencyMap.at(frequency)->channelID;
    }
    catch (const std::out_of_range&)
    {
        returnValue = CHANNEL_ID_NOT_FOUND;
    }

    return returnValue;
}

// Returns the ID of the channel corresponding to a frequency provided as a double.
uint64 TS3Channels::getChannelID(double frequency)
{
    // Need to get the rounding right
    // Try 128.300 to see why this is needed!
    return getChannelID(uint16_t(0.1 * round(1000 * frequency)));
}