#include "windows.h"
#include "psapi.h"
#pragma comment(lib,"psapi.lib")
#pragma comment(lib,"version.lib")

void __stdcall GetFSVersionInfo(TCHAR *szFileNameIn, WORD* wMajor, WORD* wMinor, WORD* wBuild)
	{
	TCHAR szWorkingFileName[MAX_PATH] = {0};
	DWORD dwFileVersionSize = NULL;
	DWORD dwVersionMS = NULL;
	DWORD dwVersionLS = NULL;
	DWORD dwFixedFileInfo = NULL;
	LPVOID lpFixedFileInfo = &dwFixedFileInfo;
	LPVOID lpFileVersionInfo = NULL;
	UINT uFixedFileInfoSize = NULL;

	if (NULL == szFileNameIn) GetModuleFileName( NULL, szWorkingFileName, MAX_PATH );
	else strcpy_s(szWorkingFileName,szFileNameIn);

	dwFileVersionSize = GetFileVersionInfoSize( szWorkingFileName, NULL );
	lpFileVersionInfo = new BYTE[dwFileVersionSize];
	ZeroMemory( lpFileVersionInfo, sizeof(BYTE)*dwFileVersionSize );

	GetFileVersionInfo( szWorkingFileName, NULL, dwFileVersionSize, lpFileVersionInfo );
	VerQueryValue( lpFileVersionInfo, TEXT("\\"), &lpFixedFileInfo, &uFixedFileInfoSize );

	dwVersionMS = ((VS_FIXEDFILEINFO*)lpFixedFileInfo)->dwFileVersionMS;
	dwVersionLS = ((VS_FIXEDFILEINFO*)lpFixedFileInfo)->dwFileVersionLS;
	*wMajor =  HIWORD(dwVersionMS);
	*wMinor =  LOWORD(dwVersionMS);
	if ( HIWORD(dwVersionLS) != 0 ) *wBuild =  HIWORD(dwVersionLS);
	else *wBuild =  (WORD)dwVersionLS;

	delete[] lpFileVersionInfo;
	}