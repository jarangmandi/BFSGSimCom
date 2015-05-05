#include "TS3Channels.h"

#include <regex>
#include <string>
#include <map>

using namespace std;

TS3Channels::TS3Channels()
{
    deleteAllChannels();
}

TS3Channels::~TS3Channels()
{
}

void TS3Channels::deleteAllChannels(void)
{
    channelMap.clear();
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
        frequency = uint16_t(100 * std::stod(matchedFrequency.str()));
    }

    return frequency;
}

uint16_t TS3Channels::addOrUpdateChannel(string channelName, uint64 channelID)
{
    uint16_t frequency;
    
    // Look for a frequency in the channel name we were passed
    frequency = getFrequencyFromString(channelName);

    // If we found one, then
    if (frequency != 0) {
        // Save the channel ID, indexed by the frequency that we found.
        channelMap[frequency] = channelID;
    }
    
    return frequency;
}

uint64 TS3Channels::deleteChannel(uint64 channelID)
{
    // Assume we're not going to find the channel
    uint64 retValue = CHANNEL_ID_NOT_FOUND;

    // Iterate across everything in the map
    for (map<uint16_t, uint64>::iterator it = channelMap.begin(); it != channelMap.end(); it++) {

        // Get the channel information from the iterator
        uint64 chInfo = uint64(it->second);

        // If this is the channel we're looking for, then remove it from the map and exit the loop.
        if (chInfo == channelID) {
            channelMap.erase(it);
            retValue = channelID;
            break;
        }
    }

    return retValue;
}

uint64 TS3Channels::getChannelID(uint16_t frequency)
{
    uint64 returnValue;

    try
    {
        returnValue = channelMap.at(frequency);
    }
    catch (const std::out_of_range&)
    {
        returnValue = CHANNEL_ID_NOT_FOUND;
    }
    
    return returnValue;
}

uint64 TS3Channels::getChannelID(double frequency)
{
    // Need to get the rounding right
    return getChannelID(uint16_t(0.1 * round(1000 * frequency)));
}