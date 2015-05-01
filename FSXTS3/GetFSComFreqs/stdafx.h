// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define WIN32_LEAN_AND_MEAN
#define FSAPI __stdcall

#pragma once

#include "windows.h"
#include "SimConnect.h"
#include <process.h>
#include <math.h>
#include "resource.h"

#pragma comment(lib, "SimConnect.lib")

extern HANDLE hSimConnect;
extern HANDLE hMsgThread;
extern HRESULT hr;
extern HINSTANCE ThisModule;
extern char ThisModuleName[MAX_PATH];

static enum DEFINE
{
COM_REQUEST = 64,
COM_DEFINITION,
};

enum SIMCONNECT_EVENT_ID
	{
	EVENT_1sec,
	EVENT_4sec,
	EVENT_6Hz,
	EVENT_AircraftLoaded,
	EVENT_Crashed,
	EVENT_CrashReset,
	EVENT_FlightLoaded,
	EVENT_FlightSaved,
	EVENT_FlightPlanActivated,
	EVENT_FlightPlanDeactivated,
	EVENT_Frame,
	EVENT_Pause,
	EVENT_Paused,
	EVENT_PauseFrame,
	EVENT_PositionChanged,
	EVENT_Sim,
	EVENT_SimStart,
	EVENT_SimStop ,
	EVENT_Sound,
	EVENT_Unpaused,
	EVENT_View,
	EVENT_WeatherModeChanged,
	EVENT_ObjectAdded,
	EVENT_ObjectRemoved,
	EVENT_MissionCompleted,
	EVENT_CustomMissionActionExecuted,
	EVENT_FlightSegmentReadyForGrading,
	EVENT_MultiplayerClientStarted,
	EVENT_MultiplayerServerStarted,
	EVENT_MultiplayerSessionEnded,
	EVENT_RaceEnd,
	EVENT_RaceLap,
	EVENT_WeaponFired,
	EVENT_WeaponDetonated,
	EVENT_CountermeasureFired,
	EVENT_ObjectDamagedByWeapon,
	};

void FSAPI StartSimConnect();
void FSAPI StopSimConnect();
void CALLBACK SimConnectDispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
unsigned __stdcall MessageRoutine( void * p);


void OnRecvOpen (SIMCONNECT_RECV_OPEN *pData, DWORD cbData, void* pContext);
void OnRecvEvent (SIMCONNECT_RECV_EVENT *pData, DWORD cbData, void* pContext);
void OnRecvEventFrame (SIMCONNECT_RECV_EVENT_FRAME *pData, DWORD cbData, void* pContext);
void OnRecvSimobjectData(SIMCONNECT_RECV_SIMOBJECT_DATA *pData, DWORD cbData, void* pContext);
void OnRecvEventFileName (SIMCONNECT_RECV_EVENT_FILENAME* pData, DWORD cbData, void* pContext);
void OnRecvEventWeatherMode( SIMCONNECT_WEATHER_MODE* pData, DWORD cbData, void* pContext);
void OnRecvEventMultiPlayerServerStarted ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_SERVER_STARTED *pData, DWORD cbData, void* pContext);
void OnRecvEventMultiPlayerClientStarted ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_CLIENT_STARTED *pData, DWORD cbData, void* pContext);
void OnRecvEventMultiPlayerSessionEnded ( SIMCONNECT_RECV_EVENT_MULTIPLAYER_SESSION_ENDED *pData, DWORD cbData, void* pContext);
void OnRecvSimobjectDataByType(SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE *pData, DWORD cbData, void* pContext);
void OnRecvSimObjectFrame( SIMCONNECT_RECV_EVENT_FRAME *pData, DWORD cbData, void* pContext );
void OnRecvWeatherObservation(SIMCONNECT_RECV_WEATHER_OBSERVATION *pWeather, DWORD cbData, void* pContext);
void OnRecvCloudState(SIMCONNECT_RECV_CLOUD_STATE* pData, DWORD cbData, void* pContext);
void OnRecvException(SIMCONNECT_RECV_EXCEPTION *pData, DWORD cbData, void* pContext);
void OnRecvAssignedObjectID(SIMCONNECT_RECV_ASSIGNED_OBJECT_ID* pData, DWORD cbData, void* pContext);
void OnRecvReservedKey(SIMCONNECT_RECV_RESERVED_KEY* pData, DWORD cbData, void* pContext);
void OnRecvCustomAction( SIMCONNECT_RECV_CUSTOM_ACTION* pData, DWORD cbData, void* pContext);
void OnRecvSystemState( SIMCONNECT_RECV_SYSTEM_STATE* pData, DWORD cbData, void* pContext);
void OnRecvClientData( SIMCONNECT_RECV_CLIENT_DATA* pData, DWORD cbData, void* pContext);
void OnRecvAirportList( SIMCONNECT_RECV_AIRPORT_LIST* pData, DWORD cbData, void* pContext);
void OnRecvVORList( SIMCONNECT_RECV_VOR_LIST* pData, DWORD cbData, void* pContext);
void OnRecvNDBList( SIMCONNECT_RECV_NDB_LIST* pData, DWORD cbData, void* pContext);
void OnRecvWaypointList( SIMCONNECT_RECV_WAYPOINT_LIST* pData, DWORD cbData, void* pContext);
void OnRecvRaceEnd( SIMCONNECT_RECV_EVENT_RACE_END* pData, DWORD cbData, void* pContext);
void OnRecvRaceLap( SIMCONNECT_RECV_EVENT_RACE_LAP* pData, DWORD cbData, void* pContext);
/*
void OnRecvOberserverData( SIMCONNECT_RECV_OBSERVER_DATA* pData, DWORD cbData, void* pContext);
void OnRecvGroundInfo( SIMCONNECT_RECV_GROUND_INFO* pData, DWORD cbData, void* pContext);
void OnRecvSynchronousBlock( SIMCONNECT_RECV_SYNCHRONOUS_BLOCK* pData, DWORD cbData, void* pContext);
void OnRecvExternalSimCreate( SIMCONNECT_RECV_EXTERNAL_SIM_CREATE* pData, DWORD cbData, void* pContext);
void OnRecvExternalSimDestroy( SIMCONNECT_RECV_EXTERNAL_SIM_DESTROY* pData, DWORD cbData, void* pContext);
void OnRecvExternalSimSimulate( SIMCONNECT_RECV_EXTERNAL_SIM_SIMULATE* pData, DWORD cbData, void* pContext);
void OnRecvExternalSimLocationChanged( SIMCONNECT_RECV_EXTERNAL_SIM_LOCATION_CHANGED* pData, DWORD cbData, void* pContext);
void OnRecvExternalSimEvent( SIMCONNECT_RECV_EXTERNAL_SIM_EVENT* pData, DWORD cbData, void* pContext);
void OnRecvEventCounterMeasure( SIMCONNECT_RECV_EVENT_COUNTERMEASURE* pData, DWORD cbData, void* pContext);
void OnRecvEventObjectDamagedByWeapon( SIMCONNECT_RECV_EVENT_OBJECT_DAMAGED_BY_WEAPON* pData, DWORD cbData, void* pContext);
void OnRecvVersion( SIMCONNECT_RECV_VERSION* pData, DWORD cbData, void* pContext);
void OnRecvSceneryComplexity( SIMCONNECT_RECV_SCENERY_COMPLEXITY* pData, DWORD cbData, void* pContext);
void OnRecvShadowFlags( SIMCONNECT_RECV_SHADOW_FLAGS* pData, DWORD cbData, void* pContext);
void OnRecvTacanList( SIMCONNECT_RECV_TACAN_LIST* pData, DWORD cbData, void* pContext);
void OnRecvCamera6DoF( SIMCONNECT_RECV_CAMERA_6DOF* pData, DWORD cbData, void* pContext);
void OnRecvCameraFov( SIMCONNECT_RECV_CAMERA_FOV* pData, DWORD cbData, void* pContext);
void OnRecvCameraSensorMode( SIMCONNECT_RECV_CAMERA_SENSOR_MODE* pData, DWORD cbData, void* pContext);
void OnRecvCameraWindowPosition( SIMCONNECT_RECV_CAMERA_WINDOW_POSITION* pData, DWORD cbData, void* pContext);
void OnRecvCameraWindowSize( SIMCONNECT_RECV_CAMERA_WINDOW_SIZE* pData, DWORD cbData, void* pContext);
void OnRecvMissionObjectCount(  SIMCONNECT_RECV_MISSION_OBJECT_COUNT* pData, DWORD cbData, void* pContext);
void OnRecvGoalData( SIMCONNECT_RECV_GOAL* pData, DWORD cbData, void* pContext);
void OnRecvMissionObjective( SIMCONNECT_MISSION_OBJECT_TYPE* pData, DWORD cbData, void* pContext);
void OnRecvFlightSegment( SIMCONNECT_RECV_FLIGHT_SEGMENT* pData, DWORD cbData, void* pContext);
void OnRecvFlightSegmentForGrading( SIMCONNECT_RECV_FLIGHT_SEGMENT_READY_FOR_GRADING* pData, DWORD cbData, void* pContext);
void OnRecvParameterRange(SIMCONNECT_RECV_PARAMETER_RANGE* pData, DWORD cbData, void* pContext);
void OnRecvEventWeapon( SIMCONNECT_RECV_EVENT_WEAPON* pData, DWORD cbData, void* pContext);
*/
