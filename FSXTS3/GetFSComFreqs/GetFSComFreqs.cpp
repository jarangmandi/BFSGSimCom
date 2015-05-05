// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

HANDLE hSimConnect = 0;
HANDLE hMsgThread = 0;
HINSTANCE ThisModule = 0;
HRESULT hr = 0;
char ThisModuleName[MAX_PATH];

void FSAPI StartSimConnect()
{
    if (NULL == hSimConnect)
    {
        GetModuleFileName(ThisModule, ThisModuleName, MAX_PATH);
        hr = SimConnect_Open(&hSimConnect, ThisModuleName, NULL, NULL, NULL, 0);
        hr = SimConnect_CallDispatch(hSimConnect, SimConnectDispatch, NULL);
        if (FAILED(hr))
            hSimConnect = 0;
        else
            hMsgThread = (HANDLE)_beginthreadex(NULL, NULL, MessageRoutine, NULL, NULL, NULL);
    }
}

void FSAPI StopSimConnect()
{
    if (NULL != hSimConnect)
    {
        hr = SimConnect_Close(hSimConnect);
        hSimConnect = 0;
    }
}

void FSAPI	DLLStart(void)
{
    if (NULL == hSimConnect)
    {
        GetModuleFileName(ThisModule, ThisModuleName, MAX_PATH);
        hr = SimConnect_Open(&hSimConnect, ThisModuleName, NULL, NULL, NULL, 0);
        hr = SimConnect_CallDispatch(hSimConnect, SimConnectDispatch, NULL);
        if (FAILED(hr)) hSimConnect = 0;
        else hMsgThread = (HANDLE)_beginthreadex(NULL, NULL, MessageRoutine, NULL, NULL, NULL);
    }
}

void FSAPI	DLLStop(void)
{
    if (NULL != hSimConnect)
    {
        hr = SimConnect_Close(hSimConnect);
        hSimConnect = 0;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        ThisModule = hModule;
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


void CALLBACK SimConnectDispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_NULL:
        break;
    case SIMCONNECT_RECV_ID_EXCEPTION:
        OnRecvException((SIMCONNECT_RECV_EXCEPTION*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_OPEN:
        OnRecvOpen((SIMCONNECT_RECV_OPEN*)pData, cbData, hSimConnect);
        break;
    case SIMCONNECT_RECV_ID_QUIT:
        break;
    case SIMCONNECT_RECV_ID_EVENT:
        OnRecvEvent((SIMCONNECT_RECV_EVENT*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_EVENT_OBJECT_ADDREMOVE:
        break;
    case SIMCONNECT_RECV_ID_EVENT_FILENAME:
        OnRecvEventFileName((SIMCONNECT_RECV_EVENT_FILENAME*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_EVENT_FRAME:
        OnRecvEventFrame((SIMCONNECT_RECV_EVENT_FRAME*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        OnRecvSimobjectData((SIMCONNECT_RECV_SIMOBJECT_DATA*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA_BYTYPE:
        OnRecvSimobjectDataByType((SIMCONNECT_RECV_SIMOBJECT_DATA_BYTYPE*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_WEATHER_OBSERVATION:
        OnRecvWeatherObservation((SIMCONNECT_RECV_WEATHER_OBSERVATION*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_CLOUD_STATE:
        OnRecvCloudState((SIMCONNECT_RECV_CLOUD_STATE*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
        OnRecvAssignedObjectID((SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_RESERVED_KEY:
        OnRecvReservedKey((SIMCONNECT_RECV_RESERVED_KEY*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_CUSTOM_ACTION:
        OnRecvCustomAction((SIMCONNECT_RECV_CUSTOM_ACTION*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_SYSTEM_STATE:
        OnRecvSystemState((SIMCONNECT_RECV_SYSTEM_STATE*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_CLIENT_DATA:
        OnRecvClientData((SIMCONNECT_RECV_CLIENT_DATA*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_EVENT_WEATHER_MODE:
        OnRecvEventWeatherMode((SIMCONNECT_WEATHER_MODE*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_AIRPORT_LIST:
        OnRecvAirportList((SIMCONNECT_RECV_AIRPORT_LIST*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_VOR_LIST:
        OnRecvVORList((SIMCONNECT_RECV_VOR_LIST*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_NDB_LIST:
        OnRecvNDBList((SIMCONNECT_RECV_NDB_LIST*)pData, cbData, pContext);
        break;
    case SIMCONNECT_RECV_ID_WAYPOINT_LIST:
        OnRecvWaypointList((SIMCONNECT_RECV_WAYPOINT_LIST*)pData, cbData, pContext);
        break;
        //		case SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SERVER_STARTED:
        //			OnRecvEventMultiPlayerServerStarted ( (SIMCONNECT_RECV_EVENT_MULTIPLAYER_SERVER_STARTED*)pData, cbData, pContext);
        //		break;
        //		case SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_CLIENT_STARTED:
        //			OnRecvEventMultiPlayerClientStarted ( (SIMCONNECT_RECV_EVENT_MULTIPLAYER_CLIENT_STARTED*)pData, cbData, pContext);
        //		break;
        //		case SIMCONNECT_RECV_ID_EVENT_MULTIPLAYER_SESSION_ENDED:
        //			OnRecvEventMultiPlayerSessionEnded ( (SIMCONNECT_RECV_EVENT_MULTIPLAYER_SESSION_ENDED*)pData, cbData, pContext);
        //		break;
        //		case SIMCONNECT_RECV_ID_EVENT_RACE_END:
        //			OnRecvRaceEnd( (SIMCONNECT_RECV_EVENT_RACE_END*)pData, cbData, pContext);
        //		break;
        //		case SIMCONNECT_RECV_ID_EVENT_RACE_LAP:
        //			OnRecvRaceLap( (SIMCONNECT_RECV_EVENT_RACE_LAP*)pData, cbData, pContext);
        //		break;
        /*
                case SIMCONNECT_RECV_ID_OBSERVER_DATA:
                OnRecvOberserverData( (SIMCONNECT_RECV_OBSERVER_DATA*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_GROUND_INFO:
                OnRecvGroundInfo( (SIMCONNECT_RECV_GROUND_INFO*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_SYNCHRONOUS_BLOCK:
                OnRecvSynchronousBlock( (SIMCONNECT_RECV_SYNCHRONOUS_BLOCK*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EXTERNAL_SIM_CREATE:
                OnRecvExternalSimCreate( (SIMCONNECT_RECV_EXTERNAL_SIM_CREATE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EXTERNAL_SIM_DESTROY:
                OnRecvExternalSimDestroy( (SIMCONNECT_RECV_EXTERNAL_SIM_DESTROY*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EXTERNAL_SIM_SIMULATE:
                OnRecvExternalSimSimulate( (SIMCONNECT_RECV_EXTERNAL_SIM_SIMULATE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EXTERNAL_SIM_LOCATION_CHANGED:
                OnRecvExternalSimLocationChanged( (SIMCONNECT_RECV_EXTERNAL_SIM_LOCATION_CHANGED*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EXTERNAL_SIM_EVENT:
                OnRecvExternalSimEvent( (SIMCONNECT_RECV_EXTERNAL_SIM_EVENT*)pData, cbData, pContext);
                break;

                case SIMCONNECT_RECV_ID_EVENT_COUNTERMEASURE:
                OnRecvEventCounterMeasure( (SIMCONNECT_RECV_EVENT_COUNTERMEASURE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_EVENT_OBJECT_DAMAGED_BY_WEAPON:
                OnRecvEventObjectDamagedByWeapon( (SIMCONNECT_RECV_EVENT_OBJECT_DAMAGED_BY_WEAPON*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_VERSION:
                OnRecvVersion( (SIMCONNECT_RECV_VERSION*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_SCENERY_COMPLEXITY:
                OnRecvSceneryComplexity( (SIMCONNECT_RECV_SCENERY_COMPLEXITY*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_SHADOW_FLAGS:
                OnRecvShadowFlags( (SIMCONNECT_RECV_SHADOW_FLAGS*)pData, cbData, pContext);
                break;

                case SIMCONNECT_RECV_ID_EVENT_WEAPON:
                OnRecvEventWeapon( (SIMCONNECT_RECV_EVENT_WEAPON*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_TACAN_LIST:
                OnRecvTacanList( (SIMCONNECT_RECV_TACAN_LIST*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_CAMERA_6DOF:
                OnRecvCamera6DoF( (SIMCONNECT_RECV_CAMERA_6DOF*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_CAMERA_FOV:
                OnRecvCameraFov( (SIMCONNECT_RECV_CAMERA_FOV*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_CAMERA_SENSOR_MODE:
                OnRecvCameraSensorMode( (SIMCONNECT_RECV_CAMERA_SENSOR_MODE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_CAMERA_WINDOW_POSITION:
                OnRecvCameraWindowPosition( (SIMCONNECT_RECV_CAMERA_WINDOW_POSITION*)pData, cbData,pContext);
                break;
                case SIMCONNECT_RECV_ID_CAMERA_WINDOW_SIZE:
                OnRecvCameraWindowSize( (SIMCONNECT_RECV_CAMERA_WINDOW_SIZE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_MISSION_OBJECT_COUNT:
                OnRecvMissionObjectCount( (SIMCONNECT_RECV_MISSION_OBJECT_COUNT*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_GOAL:
                OnRecvGoalData( (SIMCONNECT_RECV_GOAL*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_MISSION_OBJECTIVE:
                OnRecvMissionObjective( (SIMCONNECT_MISSION_OBJECT_TYPE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_FLIGHT_SEGMENT:
                OnRecvFlightSegment( (SIMCONNECT_RECV_FLIGHT_SEGMENT*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_PARAMETER_RANGE:
                OnRecvParameterRange( (SIMCONNECT_RECV_PARAMETER_RANGE*)pData, cbData, pContext);
                break;
                case SIMCONNECT_RECV_ID_FLIGHT_SEGMENT_READY_FOR_GRADING:
                OnRecvFlightSegmentForGrading( (SIMCONNECT_RECV_FLIGHT_SEGMENT_READY_FOR_GRADING*)pData, cbData, pContext);
                break;
                */

    }
}

