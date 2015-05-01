/*
 * TeamSpeak 3 SIMCOM plugin is based on the Teamspeak SDK example.
 ******Credits*****
 *FSX Code supplied by D Dawson of FSDEVELOPER.COM
 *Teamspeak code supplied by Chris at Teamspeak.com
 *Thank you to all that helped on both the above forums.
 *Without the above this Plugin would not be here.
 *
 *You can normally find me (ATC_ROO) AT MSFlights.net
 */

#ifdef _WIN32
#pragma comment(lib,"SimConnect.lib")
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"
#include "stdafx.h"

static struct TS3Functions ts3Functions;
int iExit = 0;


#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 20

#define PATH_BUFSIZE 1024
#define COMMAND_BUFSIZE 1024
#define INFODATA_BUFSIZE 1024
#define SERVERINFO_BUFSIZE 1024
#define CHANNELINFO_BUFSIZE 1024
#define RETURNCODE_BUFSIZE 1024
char FREQ[32];
char FREQ2[8];
char FREQS[8];
char FREQ2S[8];
char SQUAWK[8];
char QNH[8];
char QNHA[8];
char ALT[8];
static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"SIMCOM";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "SIMCOM";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "SIMCOM";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1.8a";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "ATC ROO";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "This plugin allows Teamspeak 3 to auto switch channels when a frequency is selected on the flight simulator Com 1 radio.";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);
	StartSimConnect();
    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");
	iExit = 1;
	StopSimConnect();

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	
	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

unsigned int uiMsgThread = 0;
SIMCONNECT_RECV *pData = 0;
DWORD cbData = 0;

struct _ComData
	{
	double Com1;
	double Com2;
	double Com1s;
	double Com2s;
	double Transponder;
	double qnh;
	double qnha;
	double altitude;
	
	};

_ComData sCD = {0.,0.};


double FSAPI GetCom1()
	{
	if (NULL == hSimConnect) return 999.999;
	else return sCD.Com1;
	}

double FSAPI GetCom2()
	{
	if (NULL == hSimConnect) return 999.999;
	else return sCD.Com2;
	}

double FSAPI GetCom1s()
	{
	if (NULL == hSimConnect) return 999.999;
	else return sCD.Com1s;
	}

double FSAPI GetCom2s()
	{
	if (NULL == hSimConnect) return 999.999;
	else return sCD.Com2s;
	}

double FSAPI Transponder()
	{
	if (NULL == hSimConnect) return 0000;
	else return sCD.Transponder;
	}

double FSAPI qnh()
	{
	if (NULL == hSimConnect) return 1013;
	else return sCD.qnh;
	}

double FSAPI qnha()
	{
	if (NULL == hSimConnect) return 29.92;
	else return sCD.qnha;
	}
double FSAPI altitude()
	{
	if (NULL == hSimConnect) return 0;
	else return sCD.altitude;
	}

unsigned __stdcall MessageRoutine( void * p)
{
while (iExit == 0)
	{
	pData = 0;
	if (NULL == hSimConnect)
		{
		GetModuleFileName(ThisModule,ThisModuleName,MAX_PATH);
		hr = SimConnect_Open(&hSimConnect,ThisModuleName,NULL,NULL,NULL,0);
		}
	if (NULL != hSimConnect) hr = SimConnect_GetNextDispatch(hSimConnect,&pData,&cbData);
	if (NULL != pData) SimConnectDispatch(pData, cbData, NULL);
//	if (NULL == hSimConnect) _endthreadex(0);
	Sleep(5);
	}
return 0;
}

void OnRecvOpen (SIMCONNECT_RECV_OPEN *pData, DWORD cbData, void* pContext)
	{
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"COM ACTIVE FREQUENCY:1","Mhz",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"COM STANDBY FREQUENCY:1","Mhz",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"COM ACTIVE FREQUENCY:2","Mhz",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"COM STANDBY FREQUENCY:2","Mhz",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"TRANSPONDER CODE:1","Hz",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"KOHLSMAN SETTING MB","Millibars",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"KOHLSMAN SETTING HG","inHg",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_AddToDataDefinition(hSimConnect,COM_DEFINITION,"INDICATED ALTITUDE","feet",SIMCONNECT_DATATYPE_FLOAT64);
	hr = SimConnect_RequestDataOnSimObject(hSimConnect,COM_REQUEST,COM_DEFINITION,SIMCONNECT_OBJECT_ID_USER,SIMCONNECT_PERIOD_SECOND,SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);
	}

void OnRecvEvent (SIMCONNECT_RECV_EVENT *pData, DWORD cbData, void* pContext)
	{
	}

void OnRecvEventFrame (SIMCONNECT_RECV_EVENT_FRAME *pData, DWORD cbData, void* pContext)
{
}

void OnRecvSimobjectData(SIMCONNECT_RECV_SIMOBJECT_DATA *pData, DWORD cbData, void* pContext)
{
switch(pData->dwDefineID)
	{
	case COM_DEFINITION:
		CopyMemory(&sCD,&pData->dwData,sizeof(sCD));
	break;
	}
}

void OnRecvSimobjectDataByType(SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pData, DWORD cbData, void* pContext)
{
}

void OnRecvClientData( SIMCONNECT_RECV_CLIENT_DATA* p, DWORD cbData, void* pContext)
{
}

void OnRecvEventFileName (SIMCONNECT_RECV_EVENT_FILENAME* pData, DWORD cbData, void* pContext){}
void OnRecvEventWeatherMode( SIMCONNECT_WEATHER_MODE* pData, DWORD cbData, void* pContext){}
void OnRecvEventMultiPlayerServerStarted ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_SERVER_STARTED *pData, DWORD cbData, void* pContext){}
void OnRecvEventMultiPlayerClientStarted ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_CLIENT_STARTED *pData, DWORD cbData, void* pContext){}
void OnRecvEventMultiPlayerSessionEnded ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_SESSION_ENDED *pData, DWORD cbData, void* pContext){}
void OnRecvSimObjectFrame( SIMCONNECT_RECV_EVENT_FRAME *pData, DWORD cbData, void* pContext ){}
void OnRecvWeatherObservation(SIMCONNECT_RECV_WEATHER_OBSERVATION *pWeather, DWORD cbData, void* pContext){}
void OnRecvCloudState(SIMCONNECT_RECV_CLOUD_STATE* pData, DWORD cbData, void* pContext){}
void OnRecvException(SIMCONNECT_RECV_EXCEPTION *pData, DWORD cbData, void* pContext){}
void OnRecvAssignedObjectID(SIMCONNECT_RECV_ASSIGNED_OBJECT_ID* pData, DWORD cbData, void* pContext){}
void OnRecvReservedKey(SIMCONNECT_RECV_RESERVED_KEY* pData, DWORD cbData, void* pContext){}
void OnRecvCustomAction( SIMCONNECT_RECV_CUSTOM_ACTION* pData, DWORD cbData, void* pContext){}
void OnRecvSystemState( SIMCONNECT_RECV_SYSTEM_STATE* pData, DWORD cbData, void* pContext){}
void OnRecvAirportList( SIMCONNECT_RECV_AIRPORT_LIST* pData, DWORD cbData, void* pContext){}
void OnRecvVORList( SIMCONNECT_RECV_VOR_LIST* pData, DWORD cbData, void* pContext){}
void OnRecvNDBList( SIMCONNECT_RECV_NDB_LIST* pData, DWORD cbData, void* pContext){}
void OnRecvWaypointList( SIMCONNECT_RECV_WAYPOINT_LIST* pData, DWORD cbData, void* pContext){}
void OnRecvRaceEnd( SIMCONNECT_RECV_EVENT_RACE_END* pData, DWORD cbData, void* pContext){}
void OnRecvRaceLap( SIMCONNECT_RECV_EVENT_RACE_LAP* pData, DWORD cbData, void* pContext){}



/****************************** Optional functions ********************************/
/*
 * Following functions are optional, if not needed you don't need to implement them.
 */
/*
 * If the plugin wants to use error return codes, plugin commands, hotkeys or menu items, it needs to register a command ID. This function will be
 * automatically called after the plugin was initialized. This function is optional. If you don't use these features, this function can be omitted.
 * Note the passed pluginID parameter is no longer valid after calling this function, so you must copy it and store it in the plugin.
 */
void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}


/*
 * Implement the following three functions when the plugin should display a line in the server/channel/client info.
 * If any of ts3plugin_infoTitle, ts3plugin_infoData or ts3plugin_freeMemory is missing, the info text will not be displayed.
 */

/* Static title shown in the left column in the info frame */
const char* ts3plugin_infoTitle() {
	return "Your Simulator info";
}

/*
 * Dynamic content shown in the right column in the info frame. Memory for the data string needs to be allocated in this
 * function. The client will call ts3plugin_freeMemory once done with the string to release the allocated memory again.
 * Check the parameter "type" if you want to implement this feature only for specific item types. Set the parameter
 * "data" to NULL to have the client ignore the info data.
 */
void ts3plugin_infoData(uint64 serverConnectionHandlerID, uint64 id, enum PluginItemType type, char** data) {
	char* name;

	/* For demonstration purpose, display the name of the currently selected server, channel or client. */
	switch(type) {
		case PLUGIN_SERVER:
			if(ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &name) != ERROR_ok) {
				printf("Error getting info\n");
				return;
			}
			break;
		case PLUGIN_CHANNEL:
			if(ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, id, CHANNEL_NAME, &name) != ERROR_ok) {
				printf("Error getting info\n");
				return;
			}
			break;
		case PLUGIN_CLIENT:
			if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, (anyID)id, CLIENT_NICKNAME, &name) != ERROR_ok) {
				printf("Error getting info\n");
				return;
			}
			break;
		default:
			printf("Invalid item type: %d\n", type);
			data = NULL;  /* Ignore */
			return;
	}

	*data = (char*)malloc(INFODATA_BUFSIZE * sizeof(char));  /* Must be allocated in the plugin! */
	sprintf_s(FREQ, "%3.3f", GetCom1());
	sprintf_s(FREQ2, "%3.3f", GetCom2());
	sprintf_s(FREQS, "%3.3f", GetCom1s());
	sprintf_s(FREQ2S, "%3.3f", GetCom2s());
	sprintf_s(ALT, "%3.0f", altitude());
	sprintf_s(QNH, "%2.0f", qnh());
	sprintf_s(QNHA, "%4.2f", qnha());
	sprintf_s(SQUAWK, "%4.0f", Transponder());

			 if(GetCom1() > 136.999 || GetCom1() < 118.000) {
			snprintf(*data, INFODATA_BUFSIZE, "[COLOR=#E42217][B]IS NOT AVAILABLE![/B][/COLOR]");
		 } else if(qnh() == 0 && qnha() == 0.00) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz\nCom 2:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz \n[COLOR=#151B8D][B]You are an Air Traffic Controller[/B][/COLOR]", FREQ, FREQS);
		} else if(Transponder() < 10) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking: [COLOR=#151B8D][B]000%s[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA, SQUAWK);
		} else if(Transponder() < 100) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking: [COLOR=#151B8D][B]00%s[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA, SQUAWK);
		} else if(Transponder() < 1000) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking: [COLOR=#151B8D][B]0%s[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA, SQUAWK);
		} else if(Transponder() == 1200) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#151B8D][B]  1200 US VFR[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else if(Transponder() == 7000) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#151B8D][B]  7000 EU General Conspicuity[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else if(Transponder() == 0000) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#151B8D][B]  SSR Data Unreliable[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else if(Transponder() == 7500) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#E42217][B]  7500 HIJACK![/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else if(Transponder() == 7600) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#E42217][B]  7600 RADIO FAILURE![/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else if(Transponder() == 7700) {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#E42217][B]  7700 EMERGENCY![/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA);
		} else {
			snprintf(*data, INFODATA_BUFSIZE, "Com 1 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nCom 2 Active:[COLOR=#437C17][B] %s[/B][/COLOR] Mhz Standby:[COLOR=#E56717][B]  %s[/B][/COLOR] Mhz\nAltitude is:[COLOR=#151B8D][B]  %s[/B][/COLOR] ft \nQNH:[COLOR=#151B8D][B]  %s[/B][/COLOR] or [COLOR=#151B8D][B]  %s[/B][/COLOR] Altimeter Setting\nSquawking:[COLOR=#151B8D][B]  %s[/B][/COLOR]", FREQ, FREQ2, FREQS, FREQ2S, ALT, QNH, QNHA, SQUAWK);
		}
	ts3Functions.freeMemory(FREQ);
	ts3Functions.freeMemory(FREQ2);
	ts3Functions.freeMemory(FREQS);
	ts3Functions.freeMemory(FREQ2S);
	ts3Functions.freeMemory(QNH);
	ts3Functions.freeMemory(QNHA);
	ts3Functions.freeMemory(ALT);
	ts3Functions.freeMemory(SQUAWK);
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
	return 1;  /* 1 = request autoloaded, 0 = do not request autoload */
}

/************************** TeamSpeak callbacks ***************************/

void ts3plugin_onTalkStatusChangeEvent(uint64 serverConnectionHandlerID, int status, int isReceivedWhisper, anyID clientID) {
    /*Start Channel Switch */
    if (NULL == hSimConnect) StartSimConnect();
        uint64 channelId = 0;
		sprintf_s(FREQ, "%3.3f", GetCom1());
	char *chpath[] = {"Air Traffic Control", FREQ, ""}; // Channel Structure
        anyID myId = 0;
        // Get the ID from the name
		        if (ts3Functions.getChannelIDFromChannelNames(serverConnectionHandlerID, chpath, &channelId) == ERROR_ok && channelId > 0) {
            ts3Functions.getClientID(serverConnectionHandlerID, &myId);
            if (myId == clientID)
                {
                uint64 uiMyChannel = 0;
			ts3Functions.getChannelOfClient(serverConnectionHandlerID, myId, &uiMyChannel);
                if (uiMyChannel != channelId) ts3Functions.requestClientMove(serverConnectionHandlerID, myId, channelId, "", NULL);
                }
        }

        /* End Channel Switch */
}

