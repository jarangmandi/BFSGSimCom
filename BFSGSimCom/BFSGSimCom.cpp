/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2014 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sstream>
#include <iomanip>
#include <unordered_set>

#include <QtWidgets/QMessageBox>

#include "teamspeak/public_errors.h"
#include "teamspeak/public_errors_rare.h"
#include "teamspeak/public_definitions.h"
#include "teamspeak/public_rare_definitions.h"
#include "teamspeak/clientlib_publicdefinitions.h"
#include "ts3_functions.h"

#include "BFSGSimCom.h"

#include "FSUIPCWrapper.h"
#include "RangeWrapper.h"
#include "TS3Channels.h"
#include "MetaDataUtils.h"

#include "config.h"

struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

static char* pluginID = NULL;
static char* callbackReturnCode = NULL;

char pluginPath[PATH_BUFSIZE];

static FSUIPCWrapper* fsuipc = NULL;
static RangeWrapper* range = NULL;
TS3Channels* ts3Channels = NULL;
FSUIPCWrapper::SimComData simComData;
Config* cfg;

bool initialising = true;

bool blConnectedToTeamspeak = false;
bool blExtendedLoggingEnabled = false;
anyID myTS3ID;
TS3Channels::StationInfo targetChannel(TS3Channels::CHANNEL_ID_NOT_FOUND);
TS3Channels::StationInfo currentChannel;
Config::ConfigMode lastMode;

PluginItemType infoDataType = PluginItemType(0);
uint64 infoDataId = 0;

QMessageBox qMsg;

struct pair_hash {
	template <class T1, class T2>
	std::size_t operator () (const std::pair<T1, T2> &p) const {
		auto h1 = std::hash<T1>{}(p.first);
		auto h2 = std::hash<T2>{}(p.second);

		return h1 ^ h2;
	}
};

std::unordered_set<std::pair<uint64,uint64>, pair_hash> channelUpdates;

void handleModeChange(Config::ConfigMode mode);

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
    int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
    *result = (char*)malloc(outlen);
    if (WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
        *result = NULL;
        return -1;
    }
    return 0;
}
#endif

void decodeChannel(std::ostringstream& ostr, string label, uint64 channel)
{
	ostr << label << ": ";

	switch (channel)
	{
	case TS3Channels::CHANNEL_ID_NOT_FOUND:
		ostr << "Not Found";
		break;
	case TS3Channels::CHANNEL_NOT_CHILD_OF_ROOT:
		ostr << "Not under root";
		break;
	default:
		ostr << channel;
		break;
	}
}


void callback(FSUIPCWrapper::SimComData data)
{
    uint64 serverConnectionHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();

    if (pluginID != NULL && callbackReturnCode == NULL)
    {
        callbackReturnCode = (char*)malloc(65 * sizeof(char));
        ts3Functions.createReturnCode(pluginID, callbackReturnCode, 64);
    }
    
    simComData = data;

	// Generate detailed logging if required...
	if (blExtendedLoggingEnabled)
	{
		std::ostringstream ostr;

		ostr << "Callback: ";
		ostr << ((blConnectedToTeamspeak) ? "Connected" : "Disconnected") << " | ";

		if (blConnectedToTeamspeak)
		{
			ostr << "Mode: " << cfg->getMode() << " | ";
			ostr << FSUIPCWrapper::toString(simComData);
		}

		ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
	}

    // Assuming we're connected, and we're supposed to be moving when the channel changes
    if (blConnectedToTeamspeak)
    {

		Config::ConfigMode operationMode = cfg->getMode();

        if (operationMode == Config::CONFIG_DISABLED)
        {

			if (blExtendedLoggingEnabled)
			{
				std::ostringstream ostr;

				decodeChannel(ostr, "    Current", currentChannel.ch);
				ostr << " | ";
				decodeChannel(ostr, "Last Target", targetChannel.ch);

				ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
			}


			// if we're connected, but disabled, keep an eye on where we are, and make it the last target.
			targetChannel = currentChannel;
        }
        else
        {
			uint32_t frequency;

			TS3Channels::StationInfo newTargetChannel;

			TS3Channels::StationInfo rootChannel(cfg->getRootChannel());

			TS3Channels::StationInfo adjustedRootChannel;
			TS3Channels::StationInfo adjustedCurrentChannel;

			//bool blToMove = false;
			//bool blTunedToRecognisedFrequency = false;
			//bool blLastChannelMoveManual = false;

			// In the unlikely event that someone starts up the plugin whilst connected we need to set
			// the current channel and last target channel correctly!
			// Find out our ID, and which channel we're presently in
			ts3Functions.getClientID(serverConnectionHandlerID, &myTS3ID);
			ts3Functions.getChannelOfClient(serverConnectionHandlerID, myTS3ID, &currentChannel.ch);

			targetChannel = currentChannel;

			// If something has changed that might trigger a change in tuned channel...
			// either a radio change, or a position change
			if (data.blComChanged || data.blPosChanged || data.blOtherChanged)
			{
					
				// And now into the processing...
				bool blConsiderAutoMove = (operationMode == Config::CONFIG_AUTO);
				bool blConsiderManualMove = (operationMode == Config::CONFIG_MANUAL && data.blComChanged);

				if (rootChannel == TS3Channels::StationInfo(TS3Channels::CHANNEL_ID_NOT_FOUND))
				{
					adjustedRootChannel = currentChannel;
				}
				else
				{
					adjustedRootChannel = rootChannel;
				}

				// Where the mode is automatic, we always want to be moving to somewhere under the root,
				// so if we're not presently under there, then assume we are at the root for the
				// purposes of working out which channel we need to move to.
				if (blConsiderAutoMove)
				{
					if (ts3Channels->channelIsUnderRoot(currentChannel.ch, adjustedRootChannel.ch))
						adjustedCurrentChannel = currentChannel;
					else
						adjustedCurrentChannel = rootChannel;
				}
				else
				{
					adjustedCurrentChannel = currentChannel;
				}

				// Work out which radio is active, and therefore the current tuned frequency.
				switch (data.selectedCom)
				{
				case FSUIPCWrapper::Com1:
					frequency = data.iCom1Freq;
					break;
				case FSUIPCWrapper::Com2:
					frequency = data.iCom2Freq;
					break;
				default:
					frequency = 0;
				}

				// Get the target channel based on relevant information
				newTargetChannel = ts3Channels->getChannelID(
					frequency,
					adjustedCurrentChannel.ch,
					adjustedRootChannel.ch,
					cfg->getConsiderRange(),
					cfg->getOutOfRangeUntuned(),
					data.bl833Capable,
					data.dLat,
					data.dLon
				);

				bool blTargetUnderCurrentRoot = (newTargetChannel.ch != TS3Channels::CHANNEL_NOT_CHILD_OF_ROOT);
				bool blTunedToRecognisedFrequency = (newTargetChannel.ch != TS3Channels::CHANNEL_ID_NOT_FOUND && newTargetChannel.ch != TS3Channels::CHANNEL_NOT_CHILD_OF_ROOT);
				bool blLastChannelMoveManual = (currentChannel != targetChannel);
				bool blTargetChannelChanged = (newTargetChannel != targetChannel);

				if (blExtendedLoggingEnabled)
				{
					std::ostringstream ostr;

					decodeChannel(ostr, "    Current", currentChannel.ch);
					ostr << " | ";
					decodeChannel(ostr, "Adjusted", adjustedCurrentChannel.ch);
					ostr << " | ";
					decodeChannel(ostr, "Root", rootChannel.ch);
					ostr << " | ";
					decodeChannel(ostr, "Adjusted Root", adjustedRootChannel.ch);
					ostr << " | ";
					decodeChannel(ostr, "New Target", newTargetChannel.ch);
					ostr << " | ";
					decodeChannel(ostr, "Last Target", targetChannel.ch);
					ostr << " | ";
					ostr << "TargetUnderCurrentRoot: " << blTargetUnderCurrentRoot << " | ";
					ostr << "TunedToRecognisedFrequency: " << blTunedToRecognisedFrequency << " | ";
					ostr << "LastMoveManual: " << blLastChannelMoveManual << " | ";
					ostr << "TargetChannelChanged: " << blTargetChannelChanged;

					ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
				}

				if	(
						// The operation mode says we can move...
						(
							// ...either AUTO, or MANUAL if the tuning has changed or the position has changed AND the last channel move was not manual
							operationMode == Config::CONFIG_AUTO ||
							operationMode == Config::CONFIG_MANUAL && (data.blComChanged || (data.blPosChanged && !blLastChannelMoveManual)) && blTargetUnderCurrentRoot
						) &&
						// ...We're only interested if the target channel has changed and is under the currently specified root
						blTargetChannelChanged
					)
					{
						// If the target channel is "untuned", then...
						if (!blTunedToRecognisedFrequency)
						{
							// Depending on the options, the target channel is either the defined "untuned" frequency
							// or the adjusted current channgel...
							if (cfg->getUntuned())
							{
								newTargetChannel.ch = cfg->getUntunedChannel();
							}
							else
							{
								newTargetChannel.ch = adjustedCurrentChannel.ch;
							}
						}

						// Work out whether the target channel is a valid TS channel
						bool blTargetChannelInTS = (newTargetChannel != TS3Channels::CHANNEL_ID_NOT_FOUND) && (newTargetChannel != TS3Channels::CHANNEL_ROOT);

						// If the target is not the current channel, and the target is a valid TS channel, then...
						if (newTargetChannel != currentChannel && blTargetChannelInTS)
						{
							// ..request a move to the target channel
							ts3Functions.requestClientMove(serverConnectionHandlerID, myTS3ID, newTargetChannel.ch, "", callbackReturnCode);

							// and if extended logging is enabled, record that we've requested the move.
							if (blExtendedLoggingEnabled)
							{
								std::ostringstream ostr;
								ostr << "    Requested Move To: " << newTargetChannel.ch;
								ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
							}
						}
						else
						{
							// if extended logging is enables, record that we don't need to move.
							if (blExtendedLoggingEnabled)
							{
								std::ostringstream ostr;
								ostr << "    Did not request move to: " << newTargetChannel.ch;
								ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
							}
						}

						// Remember the target channel
						targetChannel = newTargetChannel;
					}
				
				else
				{
					// otherwise, the target channel is unchanged - but data in it may not be...
					if (targetChannel.ch = newTargetChannel.ch)
						targetChannel = newTargetChannel;
				}
			}
			else
			{
				// Last target channel is unchanged
				targetChannel = targetChannel; // hopefully this gets optimised out!
			}
		}

		// Send our position to other clients...
		char* currentMetaData;
			
		//if (ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_META_DATA, &currentMetaData))
		//{
		//	std::ostringstream content;
		//	content << std::setprecision(10) << data.dLat << "|" << data.dLon;

		//	std::string newMetaData = MetaDataUtils::setMetaDataString(content.str(), currentMetaData);

		//	ts3Functions.setClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_META_DATA, newMetaData.c_str());
		//	ts3Functions.flushClientSelfUpdates(serverConnectionHandlerID, callbackReturnCode);

		//	ts3Functions.freeMemory(currentMetaData);
		//}

    }
    else
	{
        targetChannel = TS3Channels::CHANNEL_ID_NOT_FOUND;
    }

    // This is a frig to avoid trying to update client data from an unrelated thread. What we're waiting for is
    // for the onServerUpdatedEvent to fire so we can safely request the info update.
	ts3Functions.requestServerVariables(serverConnectionHandlerID);
}


/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
    /* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
    static char* result = NULL;  /* Static variable so it's allocated only once */
    if (!result) {
        const wchar_t* name = L"BFSGSimCom";
        if (wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
            result = "BFSGSimCom";  /* Conversion failed, fallback here */
        }
    }
    return result;
#else
    return "BFSGSimCom";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "0.13.0b";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
    return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
    /* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Andrew J. Parish";
}

/* Plugin description */
const char* ts3plugin_description() {
    /* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "This plugin enables automatic channel moving for simulator pilots as they tune their simulated COM radio.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

// Load a single channel on a given server connection, looking up the parent information if it wasn't supplied.
void loadChannel(uint64 serverConnectionHandlerID, uint64 channel, uint64 parent = UINT64_MAX)
{
    string strName;
    char* cName;
    char* cTopic;
    char* cDesc;
    uint64 order;

    string strComment;

    if (parent == UINT64_MAX)
    {
        ts3Functions.getParentChannelOfChannel(serverConnectionHandlerID, channel, &parent);
    }

    // For each channel in the list, get the name of the channel and its parent, and add it to the channel list.
	//unsigned int xx = ts3Functions.requestChannelDescription(serverConnectionHandlerID, channel, callbackReturnCode);
    ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channel, CHANNEL_NAME, &cName);
    ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channel, CHANNEL_TOPIC, &cTopic);
	ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channel, CHANNEL_DESCRIPTION, &cDesc);
	ts3Functions.getChannelVariableAsUInt64(serverConnectionHandlerID, channel, CHANNEL_ORDER, &order);
    ts3Channels->addOrUpdateChannel(strComment, cName, cTopic, cDesc, channel, parent, order);

    // Not forgetting to free up the memory we've used for the channel name.
	ts3Functions.freeMemory(cName);
    ts3Functions.freeMemory(cTopic);
    ts3Functions.freeMemory(cDesc);

    ts3Functions.logMessage(strComment.c_str(), LogLevel::LogLevel_INFO, "BFSGSimCom", serverConnectionHandlerID);
}

void loadChannelDescription(uint64 serverConnectionHandlerID, uint64 channel)
{
	char* cDesc;

	string strComment;

	// Then request that the channel description be updated
	//unsigned int recode = ts3Functions.requestChannelDescription(serverConnectionHandlerID, channel, callbackReturnCode);

	ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channel, CHANNEL_DESCRIPTION, &cDesc);
	ts3Channels->updateChannelDescription(strComment, channel, cDesc);

	// Not forgetting to free up the memory we've used...
	ts3Functions.freeMemory(cDesc);

	ts3Functions.logMessage(strComment.c_str(), LogLevel::LogLevel_INFO, "BFSGSimCom", serverConnectionHandlerID);
}

void updateServerName()
{
	uint64 serverConnectionHandlerID;

	serverConnectionHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();

	// Update the Root channel name to be that of the TS server.
	char* result;
	if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &result) == ERROR_ok)
	{
		ts3Channels->updateRootChannelName(result);
		ts3Functions.freeMemory(result);
	}
}

// Load ALL channels on a given server connection.
void loadChannels(uint64 serverConnectionHandlerID)
{
    uint64* channelList;

	updateServerName();

    if (ts3Functions.getChannelList(serverConnectionHandlerID, &channelList) == ERROR_ok)
    {
        for (int i = 0; channelList[i] != NULL; i++)
        {
//            loadChannel(serverConnectionHandlerID, channelList[i]);
			channelUpdates.emplace(serverConnectionHandlerID, channelList[i]);
			ts3Functions.requestChannelDescription(serverConnectionHandlerID, channelList[i], callbackReturnCode);
        }

        // And not forgetting to free up the memory we've used for the channel list.
        ts3Functions.freeMemory(channelList);
    }

//    ts3Functions.requestServerVariables(serverConnectionHandlerID);
}


/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {

	int retValue = 0;

    uint64 serverConnectionHandlerID;
    int connectionStatus;

    serverConnectionHandlerID = ts3Functions.getCurrentServerConnectionHandlerID();

#ifdef _DEBUG
	blExtendedLoggingEnabled = true;
#else
	blExtendedLoggingEnabled = false;
#endif // DEBUG

	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE, pluginID);

    // Initialise the TS3 server connection status
    if (ts3Functions.getConnectionStatus(serverConnectionHandlerID, &connectionStatus) == ERROR_ok)
    {
        blConnectedToTeamspeak = (connectionStatus != 0);

        // If we're already connected when the plugin starts, get my ID and the current channel
        if (blConnectedToTeamspeak)
        {
            if (ts3Functions.getClientID(serverConnectionHandlerID, &myTS3ID) == ERROR_ok)
            {
                ts3Functions.getChannelOfClient(serverConnectionHandlerID, myTS3ID, &currentChannel.ch);
            }
        }
    }
    else
    {
        blConnectedToTeamspeak = false;
    }

	// Initialise the channel information
	ts3Channels = new TS3Channels();

	// Initialise the plugin configuration dialog
	cfg = new Config(*ts3Channels);
	lastMode = cfg->getMode();

	// Load channel data from the server connection. This needs to be done before
	// we start looking at tuned channels once the simulator connection is started.
    loadChannels(serverConnectionHandlerID);

	// Establish what's required to connect to a simulator
    if (fsuipc == NULL)
    {
		if (fsuipc = new FSUIPCWrapper(&callback))
		{
			fsuipc->start();
		}
		else
		{
			retValue = 1;
		}
    }

	if (range == NULL)
	{
		if (range = new RangeWrapper(ts3Functions, cfg))
		{
			range->start();
		}
		else
		{
			retValue = 1;
		}
	}

	// Update the information display on load
	ts3Functions.requestInfoUpdate(serverConnectionHandlerID, infoDataType, infoDataId);

    return retValue;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */

    /* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
     * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
     * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {

    // Close down the connection to the simulator
    if (fsuipc)
    {
        fsuipc->stop();
        delete fsuipc;
    }

	// Close down the range calculation thread
	if (range)
	{
		range->stop();
		delete range;
	}

    // Close down the settings dialog
    if (cfg)
    {
        cfg->close();
        delete cfg;
    }

    // Free pluginID if we registered it
    if (pluginID) {
        free(pluginID);
        pluginID = NULL;
    }
}

/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */

/* Tell client if plugin offers a configuration window. If this function is not implemented, it's an assumed "does not offer" (PLUGIN_OFFERS_NO_CONFIGURE). */
int ts3plugin_offersConfigure() {

    return PLUGIN_OFFERS_CONFIGURE_QT_THREAD;  /* In this case ts3plugin_configure does not need to be implemented */
}

/* Plugin might offer a configuration window. If ts3plugin_offersConfigure returns 0, this function does not need to be implemented. */
void ts3plugin_configure(void* handle, void* qParentWidget) {

    /* Execute the config dialog */
    cfg->exec();

	handleModeChange(cfg->getMode());
}

/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
    const size_t sz = strlen(id) + 1;
    pluginID = (char*)malloc(sz * sizeof(char));
    if (pluginID != NULL)
        _strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
}

/* Plugin command keyword. Return NULL or "" if not used. */
const char* ts3plugin_commandKeyword() {
    return NULL;
}


/* Client changed current server connection handler */
void ts3plugin_currentServerConnectionChanged(uint64 serverConnectionHandlerID) {
    //printf("PLUGIN: currentServerConnectionChanged %llu (%llu)\n", (long long unsigned int)serverConnectionHandlerID, (long long unsigned int)ts3Functions.getCurrentServerConnectionHandlerID());
}

/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
    return "BFSGSimCom";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {

	bool blAdvancedInfo = cfg->getInfoDetailed();

	const char* strStation = "";

	const string strCGreen = "#006400";
	const string strCRed = "#ff0000";

	string strMode = "";
	string strUntuned = "";
	string strCom = "";

	double dCom1 = 0.0f;
    double dCom1s = 0.0f;
    double dCom2 = 0.0f;
    double dCom2s = 0.0f;
	double dRange = 0.0f;

    // Save this in case we need to do it adhoc...
    infoDataType = type;
    infoDataId = id;

	ostringstream ostr;
	
    // Must be allocated in the plugin
    *data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));  /* Must be allocated in the plugin! */

    if (*data != NULL)
    {
        bool connected = fsuipc->isConnected();
		bool detailed = cfg->getInfoDetailed();

        if (connected)
        {
			ostr << std::fixed << "[color=" << strCGreen << "]Connected to Sim.[/color]\n\nMode: ";
			
			strMode = "[color=";

            Config::ConfigMode mode = cfg->getMode();
            switch (mode)
            {
            case Config::CONFIG_DISABLED:
                strMode += strCRed + "]Disabled";
                break;
            case Config::CONFIG_MANUAL:
				strMode += strCGreen + "]Enabled";
				if (blAdvancedInfo) strMode += " but not locked";
                break;
            case Config::CONFIG_AUTO:
				if (blAdvancedInfo)
				{
					strMode += strCGreen + "]Enabled and locked";

					if (targetChannel == 0)
					{
						strMode += "  but to invalid channel";
					}
				}
				else
				{
					strMode += strCGreen + "]Auto";
				}
                break;
            default:
                strMode += strCRed + "]";
            }

			// Only build the "root" information when initialisation is complete, when
			// we want advanced info and the plugin isn't disabled.
			if (mode != Config::CONFIG_DISABLED && blAdvancedInfo && !initialising) strMode += " with root \"" + cfg->getRootChannelName() + "\"";

			ostr << strMode << "[/color]";

			// END OF TOP LEVEL MODE INFORMATION

			// START OF OPERATIONAL INFORMATION

			// Only required if the state is not Disabled
			if (mode != Config::CONFIG_DISABLED)
            {
				// Advanced info includes a detailed description of how the plugin will behave
				// as a result of the selections made in the Configuration dialog
				if (blAdvancedInfo)
				{
					string strUntuned = "";
					bool blUntuned = cfg->getUntuned();

					if (blUntuned & !initialising)
					{
						strUntuned = "to \"";
						strUntuned += cfg->getUntunedChannelName();
						strUntuned += "\" ";
					}

					ostr << ", [color=" << ((blUntuned) ? strCGreen + "]" : strCRed + "]not ") << "moving " << strUntuned << "with unmatched freq[/color]";
					ostr << ", [color=" << ((cfg->getConsiderRange()) ? strCGreen + "]" : strCRed + "]not ") << "considering station range[/color]";
					if (cfg->getConsiderRange() && blUntuned)
						ostr << ", [color=" << ((cfg->getOutOfRangeUntuned()) ? strCGreen + "]" : strCRed + "]not ") << "treating out of range station as unmatched[/color]";
					ostr << ".";

					// Advanced mode also tells about the cockpit radio selection
					switch (simComData.selectedCom)
					{
					case FSUIPCWrapper::Com1:
						strCom = "Com1";
						break;
					case FSUIPCWrapper::Com2:
						strCom = "Com2";
						break;
					case FSUIPCWrapper::None:
						strCom = "No";
						break;
					default:
						// Required because some aircraft allow multiple Com selections!
						strCom = "Unrecognised / More than one";
						break;
					}

					ostr << "\n\n[b]" << strCom << "[/b] radio selected.\n";
				}

				// Get the range to the tuned station if it's there to be got.
				if (targetChannel.ch != TS3Channels::CHANNEL_ID_NOT_FOUND && targetChannel.ch != TS3Channels::CHANNEL_NOT_CHILD_OF_ROOT)
				{
					strStation = targetChannel.id.c_str();

					if (strncmp(strStation, "", 10))
					{
						dRange = targetChannel.range;
					}
				}

				// Colour of information depends on whether it's selected or not.
				string strCom1Col = (simComData.selectedCom == FSUIPCWrapper::Com1) ? strCGreen : strCRed;
				string strCom2Col = (simComData.selectedCom == FSUIPCWrapper::Com2) ? strCGreen : strCRed;

				// Precision of frequency display
				int precision = 2 + ((simComData.bl833Capable) ? 1 : 0);

				// Com1 Information - frequency, tuned station and range
                dCom1 = 0.001 * simComData.iCom1Freq;
				ostr << "\n[b][color=" << strCom1Col << "]Com 1 Freq: " << std::setprecision(precision) << dCom1;
				if (simComData.selectedCom == FSUIPCWrapper::Com1 && strncmp(strStation, "", 10))
				{
					ostr << " - " << strStation;
					if (dRange < 10800.0)
						ostr << " @ " << std::setprecision(1) << dRange << "nm";
				}
				ostr << "[/color][/b]";

				// Com2 Information - frequency, tuned station and range
				dCom2 = 0.001 * simComData.iCom2Freq;
				ostr << "\n[b][color=" << strCom2Col << "]Com 2 Freq: " << std::setprecision(precision) << dCom2;
				if (simComData.selectedCom == FSUIPCWrapper::Com2 && strncmp(strStation, "", 10))
				{
					ostr << " - " << strStation;
					if (dRange < 10800.0)
						ostr << " @ " << std::setprecision(1) << dRange << "nm";
				}
				ostr << "[/color][/b]";

				// Standby radio frequencies only in the advanced info data
				if (blAdvancedInfo)
				{
					dCom1s = 0.001 * simComData.iCom1Sby;
					ostr << "\n[color=" << strCom1Col << "]Com 1 Stby: " << std::setprecision(precision) << dCom1s << "[/color]";

					dCom2s = 0.001 * simComData.iCom2Sby;
					ostr << "\n[color=" << strCom2Col << "]Com 2 Stby: " << std::setprecision(precision) << dCom2s << "[/color]";
				}

				// Temporary (?) code to display our current position.
				char* positionString;
				ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_META_DATA, &positionString);
				ostr << "\n\nPosition: " << positionString;
				ts3Functions.freeMemory(positionString);
            }
            else
            {
            }
        }
        else
        {
			ostr << "[color=" << strCRed << "]Not connected to Sim.[/color]";
        }

		_snprintf_s(
			*data,
			INFODATA_BUFSIZE,
			INFODATA_BUFSIZE,
			"%s",
			ostr.str().c_str()
		);

		//snprintf(
		//	*data,
		//	INFODATA_BUFSIZE,
		//	strFormat.c_str(),
		//	strConnected, strMode, strUntuned, strCom, dCom1, dCom2, dCom1s, dCom2s, dRange, strStation, strInOutRange
		//);


	}
}

/* Required to release the memory for parameter "data" allocated in ts3plugin_infoData and ts3plugin_initMenus */
void ts3plugin_freeMemory(void* data) {
    free(data);
}

/*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
    return 0;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
    struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));

    if (menuItem != NULL)
    {
        menuItem->type = type;
        menuItem->id = id;
        _strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
        _strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
    }

    return menuItem;
}

/* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) if (menuItems != NULL && *menuItems != NULL) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS if (menuItems != NULL && *menuItems != NULL) (*menuItems)[n++] = NULL; assert(n == sz);

/*
 * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
 * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
 * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
 */
enum {
    MENU_ID_SIMCOM_CONFIGURE = 1,
    MENU_ID_DUMMY,
    MENU_ID_SIMCOM_MODE_DISABLE,
    MENU_ID_SIMCOM_MODE_MANUAL,
    MENU_ID_SIMCOM_MODE_AUTO,
	MENU_ID_SIMCOM_DEBUG_ON,
	MENU_ID_SIMCOM_DEBUG_OFF,
	MENU_ID_SIMCOM_INFO_DETAIL_ON,
	MENU_ID_SIMCOM_INFO_DETAIL_OFF
};

/*
 * Initialize plugin menus.
 * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
 * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
 * If plugin menus are not used by a plugin, do not implement this function or return NULL.
 */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
    /*
     * Create the menus
     * There are three types of menu items:
     * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
     * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
     * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
     *
     * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
     *
     * The menu text is required, max length is 128 characters
     *
     * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
     * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
     * plugin filename, without dll/so/dylib suffix
     * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
     */

	bool blInfoDetailed = cfg->getInfoDetailed();

    BEGIN_CREATE_MENUS(11);  /* IMPORTANT: Number of menu items must be correct! */
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_CONFIGURE, "Configure", "");
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_DUMMY, "------------------", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_INFO_DETAIL_ON, "Enable Detailed Information", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_INFO_DETAIL_OFF, "Disable Detailed Information", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_DUMMY, "------------------", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_MODE_DISABLE, "Mode - Disabled", "");
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_MODE_MANUAL, "Mode - Semi-automatic", "");
    CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_MODE_AUTO, "Mode - Automatic", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_DUMMY, "------------------", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_DEBUG_ON, "Enable Detailed Logging", "");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_SIMCOM_DEBUG_OFF, "Disable Detailed Logging", "");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

    // Setup initial state of menu items
	handleModeChange(cfg->getMode());

	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_DEBUG_ON, blExtendedLoggingEnabled ? 0 : 1);
	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_DEBUG_OFF, blExtendedLoggingEnabled ? 1 : 0);

	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_ON, blInfoDetailed ? 0 : 1);
	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_OFF, blInfoDetailed ? 1 : 0);

    /*
     * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
     * If unused, set menuIcon to NULL
     */
    //	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
    //	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "t.png");

    /*
     * Menus can be enabled or disabled with: ts3Functions.setPluginMenuEnabled(pluginID, menuID, 0|1);
     * Test it with plugin command: /test enablemenu <menuID> <0|1>
     * Menus are enabled by default. Please note that shown menus will not automatically enable or disable when calling this function to
     * ensure Qt menus are not modified by any thread other the UI thread. The enabled or disable state will change the next time a
     * menu is displayed.
     */
    /* For example, this would disable MENU_ID_GLOBAL_2: */

    /* All memory allocated in this function will be automatically released by the TeamSpeak client later by calling ts3plugin_freeMemory */
}

/* Helper function to create a hotkey */
static struct PluginHotkey* createHotkey(const char* keyword, const char* description) {
    struct PluginHotkey* hotkey = (struct PluginHotkey*)malloc(sizeof(struct PluginHotkey));
    if (hotkey != NULL)
    {
        _strcpy(hotkey->keyword, PLUGIN_HOTKEY_BUFSZ, keyword);
        _strcpy(hotkey->description, PLUGIN_HOTKEY_BUFSZ, description);
    }
    return hotkey;
}

/* Some makros to make the code to create hotkeys a bit more readable */
#define BEGIN_CREATE_HOTKEYS(x) const size_t sz = x + 1; size_t n = 0; *hotkeys = (struct PluginHotkey**)malloc(sizeof(struct PluginHotkey*) * sz);
#define CREATE_HOTKEY(a, b) if (hotkeys != NULL && *hotkeys != NULL) (*hotkeys)[n++] = createHotkey(a, b);
#define END_CREATE_HOTKEYS if (hotkeys != NULL && *hotkeys != NULL) (*hotkeys)[n++] = NULL; assert(n == sz);

/*
 * Initialize plugin hotkeys. If your plugin does not use this feature, this function can be omitted.
 * Hotkeys require ts3plugin_registerPluginID and ts3plugin_freeMemory to be implemented.
 * This function is automatically called by the client after ts3plugin_init.
 */
void ts3plugin_initHotkeys(struct PluginHotkey*** hotkeys) {
    /* Register hotkeys giving a keyword and a description.
     * The keyword will be later passed to ts3plugin_onHotkeyEvent to identify which hotkey was triggered.
     * The description is shown in the clients hotkey dialog. */
    BEGIN_CREATE_HOTKEYS(5);  /* Create 3 hotkeys. Size must be correct for allocating memory. */
    CREATE_HOTKEY("BFSGSimCom_Off", "BFSGSimCom - Disabled");
    CREATE_HOTKEY("BFSGSimCom_Man", "BFSGSimCom - Manual");
    CREATE_HOTKEY("BFSGSimCom_Aut", "BFSGSimCom - Automatic");
	CREATE_HOTKEY("BFSGSimCom_Detail", "BFSGSimCom - Detailed Information On");
	CREATE_HOTKEY("BFSGSimCom_NoDetail", "BFSGSimCom - Detailed Information Off");
    END_CREATE_HOTKEYS;

    /* The client will call ts3plugin_freeMemory to release all allocated memory */
}

void handleModeChange(Config::ConfigMode mode)
{
	cfg->setMode(mode);

	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_MODE_DISABLE, (mode == Config::ConfigMode::CONFIG_DISABLED) ? 0 : 1);
	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_MODE_MANUAL, (mode == Config::ConfigMode::CONFIG_MANUAL) ? 0 : 1);
	ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_MODE_AUTO, (mode == Config::ConfigMode::CONFIG_AUTO) ? 0 : 1);

	targetChannel = currentChannel;
	lastMode = mode;

	callback(simComData);
}

/************************** TeamSpeak callbacks ***************************/
/*
 * Following functions are optional, feel free to remove unused callbacks.
 * See the clientlib documentation for details on each function.
 */

/* Clientlib */


// The following four functions manage the changing of channel data whilst connected to the server through updating of information.
void ts3plugin_onNewChannelCreatedEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 channelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
    loadChannel(serverConnectionHandlerID, channelID, channelParentID);
}

void ts3plugin_onDelChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
    ts3Channels->deleteChannel(channelID);
}

void ts3plugin_onChannelMoveEvent(uint64 serverConnectionHandlerID, uint64 channelID, uint64 newChannelParentID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
    loadChannel(serverConnectionHandlerID, channelID, newChannelParentID);
}

void ts3plugin_onUpdateChannelEditedEvent(uint64 serverConnectionHandlerID, uint64 channelID, anyID invokerID, const char* invokerName, const char* invokerUniqueIdentifier)
{
	// ... request the channel description information.
	channelUpdates.emplace(serverConnectionHandlerID, channelID);
	ts3Functions.requestChannelDescription(serverConnectionHandlerID, channelID, callbackReturnCode);
}

void ts3plugin_onUpdateChannelEvent(uint64 serverConnectionHandlerID, uint64 channelID)
{
	pair<uint64, uint64> sc(serverConnectionHandlerID, channelID);

	// if we're expecting the channel update (we should be!)
	if (channelUpdates.find(sc) != channelUpdates.end())
	{
		// Remove it from the list of those we're waiting for and load it up.
		channelUpdates.erase(sc);
		loadChannel(serverConnectionHandlerID, channelID);

		// This code handles the initial load and requests display of the information
		// pane when it's complete... Need to wait until all channel updates have
		// been received to populate the channel list for information display
		if (initialising && channelUpdates.empty())
		{
			initialising = false;
			cfg->populateChannelList();
			ts3Functions.requestServerVariables(serverConnectionHandlerID);
		}
	}
}

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, unsigned int errorNumber) {

    switch (newStatus)
    {
    // When we disconnect...
    case STATUS_DISCONNECTED:
		// Forget everything.

		// Delete our channel list and flag that we're no longer connected.
        ts3Channels->deleteAllChannels();
        blConnectedToTeamspeak = false;

		// Reset my ID to zero.
        myTS3ID = 0;

		// Reset the current channel and the last target channel.
		currentChannel = TS3Channels::CHANNEL_ROOT;
		targetChannel = TS3Channels::CHANNEL_ROOT;

		// Force the information window to update
		ts3Functions.requestServerVariables(serverConnectionHandlerID);
		
		break;

    // When we connect...
    case STATUS_CONNECTION_ESTABLISHED:
        // Get a list of all of the channels on the server, and iterate through it.
        loadChannels(serverConnectionHandlerID);
        blConnectedToTeamspeak = true;

		// Find out our ID, and which channel we're presently in
        ts3Functions.getClientID(serverConnectionHandlerID, &myTS3ID);
		ts3Functions.getChannelOfClient(serverConnectionHandlerID, myTS3ID, &currentChannel.ch);

		// Assume that our last target channel is were we started.
		targetChannel = currentChannel;

		// Force the information window to update
		ts3Functions.requestServerVariables(serverConnectionHandlerID);

		// Just in case we need to move when we first start...
		if (fsuipc->isConnected())
		{
			callback(simComData);
		}

		break;

	// Ignore any other status
    default:
        break;
    }


}

void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, uint64 oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage) {

	// This event is fired when any client moves. We're only interested in events where
	// we are the member who has moved.
    if (clientID == myTS3ID)
    {
		// This code will move us back to where we're supposed to be, if and only if...
		// 1. We're connected to a simulator
		// 2. We're in "AUTO" mode
        // 3. There's a valid channel to move to (>0), and
        // 4. We're not where we're supposed to be!

        if (
			fsuipc->isConnected() &&
			cfg->getMode() == Config::ConfigMode::CONFIG_AUTO &&
            targetChannel != TS3Channels::CHANNEL_ROOT &&
			targetChannel != TS3Channels::CHANNEL_NOT_CHILD_OF_ROOT &&
			targetChannel != TS3Channels::CHANNEL_ID_NOT_FOUND)
        {
			string debugMessage;

			//If we're here, then we want to be moving back to where we were before the move.

			if (newChannelID != targetChannel.ch)
			{
				// If the new channel is different to the last recorded target, then request a move back
				// to where we're supposed to be.
				debugMessage = "Requesting reutrn to";
				ts3Functions.requestClientMove(serverConnectionHandlerID, clientID, targetChannel.ch, "", NULL);
			}
			else
			{
				debugMessage = "Already in";
			}

			// Make detailed logs if enabled.
			if (blExtendedLoggingEnabled)
			{
				std::ostringstream ostr;
				ostr << "Move Event: Client has moved to " << newChannelID << " | " << debugMessage << " channel " << targetChannel.ch;
				ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
			}
			
			// Update our local record of where we are.
			currentChannel = targetChannel;
        }
        else
        {
			// Make detailed logs if enabled.
			if (blExtendedLoggingEnabled)
			{
				std::ostringstream ostr;
				ostr << "Move Event: Client has been moved to " << newChannelID;
				ts3Functions.logMessage(ostr.str().c_str(), LogLevel::LogLevel_DEBUG, "BFSGSimCom", serverConnectionHandlerID);
			}
			
			// If we've not been intercepted for any of the reasons above, then accept the channel move and update
			// our local record of where we are.
			if (newChannelID == targetChannel.ch)
			{
				currentChannel = targetChannel;
			}
			else
			{
				currentChannel = newChannelID;
			}
        }

	}
}

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage) {
    //printf("PLUGIN: onServerErrorEvent %llu %s %d %s\n", (long long unsigned int)serverConnectionHandlerID, errorMessage, error, (returnCode ? returnCode : ""));
    if (error == ERROR_ok)
    {
        // There's no error, so just say we handled it.
        return 1;
    }
    else
    {
        /* A plugin could now check the returnCode with previously (when calling a function) remembered returnCodes and react accordingly */
        /* In case of using a a plugin return code, the plugin can return:
        * 0: Client will continue handling this error (print to chat tab)
        * 1: Client will ignore this error, the plugin announces it has handled it */
        return 0;
    }

    /* If no plugin return code was used, the return value of this function is ignored */
}

/*
 * Called when a plugin menu item (see ts3plugin_initMenus) is triggered. Optional function, when not using plugin menus, do not implement this.
 *
 * Parameters:
 * - serverConnectionHandlerID: ID of the current server tab
 * - type: Type of the menu (PLUGIN_MENU_TYPE_CHANNEL, PLUGIN_MENU_TYPE_CLIENT or PLUGIN_MENU_TYPE_GLOBAL)
 * - menuItemID: Id used when creating the menu item
 * - selectedItemID: Channel or Client ID in the case of PLUGIN_MENU_TYPE_CHANNEL and PLUGIN_MENU_TYPE_CLIENT. 0 for PLUGIN_MENU_TYPE_GLOBAL.
 */
void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
    //    printf("PLUGIN: onMenuItemEvent: serverConnectionHandlerID=%llu, type=%d, menuItemID=%d, selectedItemID=%llu\n", (long long unsigned int)serverConnectionHandlerID, type, menuItemID, (long long unsigned int)selectedItemID);

	bool blInfoDetailed = cfg->getInfoDetailed();
	
	switch (type) {
    case PLUGIN_MENU_TYPE_GLOBAL:
        /* Global menu item was triggered. selectedItemID is unused and set to zero. */
        switch (menuItemID) {
        case MENU_ID_SIMCOM_CONFIGURE:
            /* Menu global 1 was triggered */
            cfg->exec();
			handleModeChange(cfg->getMode());
            break;
        case MENU_ID_SIMCOM_MODE_DISABLE:
            /* Menu global 1 was triggered */
            handleModeChange(Config::ConfigMode::CONFIG_DISABLED);
            break;
        case MENU_ID_SIMCOM_MODE_MANUAL:
            /* Menu global 1 was triggered */
			handleModeChange(Config::ConfigMode::CONFIG_MANUAL);
            break;
        case MENU_ID_SIMCOM_MODE_AUTO:
            /* Menu global 1 was triggered */
			handleModeChange(Config::ConfigMode::CONFIG_AUTO);
            break;
		case MENU_ID_SIMCOM_DEBUG_ON:
			blExtendedLoggingEnabled = true;
			break;
		case MENU_ID_SIMCOM_DEBUG_OFF:
			blExtendedLoggingEnabled = false;
			break;
		case MENU_ID_SIMCOM_INFO_DETAIL_ON:
			blInfoDetailed = true;
			cfg->setInfoDetailed(true);
			break;
		case MENU_ID_SIMCOM_INFO_DETAIL_OFF:
			blInfoDetailed = false;
			cfg->setInfoDetailed(false);
			break;
		default:
            break;
        }

		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_DEBUG_ON, blExtendedLoggingEnabled ? 0 : 1);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_DEBUG_OFF, blExtendedLoggingEnabled ? 1 : 0);

		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_ON, blInfoDetailed ? 0 : 1);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_OFF, blInfoDetailed ? 1 : 0);

        ts3Functions.requestServerVariables(ts3Functions.getCurrentServerConnectionHandlerID());
        break;
    default:
        break;
    }
}

/* This function is called if a plugin hotkey was pressed. Omit if hotkeys are unused. */
void ts3plugin_onHotkeyEvent(const char* keyword) {
    //printf("PLUGIN: Hotkey event: %s\n", keyword);
    /* Identify the hotkey by keyword ("keyword_1", "keyword_2" or "keyword_3" in this example) and handle here... */
    string strKeyword(keyword);

    if (strKeyword == "BFSGSimCom_Off")
    {
		handleModeChange(Config::ConfigMode::CONFIG_DISABLED);
    }
    else if (strKeyword == "BFSGSimCom_Man")
    {
		handleModeChange(Config::ConfigMode::CONFIG_MANUAL);
    }
    else if (strKeyword == "BFSGSimCom_Aut")
    {
		handleModeChange(Config::ConfigMode::CONFIG_AUTO);
    }
	else if (strKeyword == "BFSGSimCom_Detail")
	{
		cfg->setInfoDetailed(true);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_ON, 0);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_OFF, 1);
	}
	else if (strKeyword == "BFSGSimCom_NoDetail")
	{
		cfg->setInfoDetailed(false);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_ON, 1);
		ts3Functions.setPluginMenuEnabled(pluginID, MENU_ID_SIMCOM_INFO_DETAIL_OFF, 0);
	}

    ts3Functions.requestServerVariables(ts3Functions.getCurrentServerConnectionHandlerID());
}

void ts3plugin_onServerUpdatedEvent(uint64 serverConnectionHandlerID)
{
    ts3Functions.requestInfoUpdate(serverConnectionHandlerID, infoDataType, infoDataId);
}

