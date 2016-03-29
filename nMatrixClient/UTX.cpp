//=================================================================================================
//
//  Copyright (C) 2012-2016, DigiStar Studio
//
//  This file is part of nMatrix client app (http://feather1231.myweb.hinet.net/)
//
//  All rights reserved.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
//  Above are some template claims copied from somewhere on the Internet that don't matter.
//  The code is written by c++ language and I know you have no fear of it. :D
//  Hope that you use the code to do good things like google's slogan, don't be evil.
//  Any feedback is welcomed.
//
//  Author: Pointer Huang <digistarstudio@google.com>
//
//
//=================================================================================================


#include "StdAfx.h"
#include "UTX.h"
#include <conio.h>


CCriticalSectionUTX GPrintxCs;


DOUBLE CPerformanceCounter::factor = 0.0;
LARGE_INTEGER CPerformanceCounter::freq;


BOOL UTXLibraryInit()
{
	ASSERT(CPerformanceCounter::factor == 0.0);

	LARGE_INTEGER li;
	::QueryPerformanceFrequency(&li);
	CPerformanceCounter::factor = 1000.0 / li.QuadPart;
	CPerformanceCounter::freq = li;

	return TRUE;
}

void UTXLibraryEnd()
{
}

int printx(const char *format, ...)
{
//#ifndef _DEBUG
//	#pragma message(FILE_LOC "Release build printx disabled.")
//	return 0;
//#else
	CHAR buffer[1024];
	INT nCharacters;
	va_list vlist;

	va_start(vlist, format);
	nCharacters = vsprintf(buffer, format, vlist);
	va_end(vlist);

	ASSERT(nCharacters <= sizeof(buffer));

	GPrintxCs.EnterCriticalSection();
	_cprintf(buffer);
	GPrintxCs.LeaveCriticalSection();

	return nCharacters;
//#endif
}

int printx(const wchar_t *format, ...)
{
//#ifndef _DEBUG
//	#pragma message(FILE_LOC "Release build printx disabled.")
//	return 0;
//#else
	WCHAR buffer[1024];
	INT nCharacters;
	va_list vlist;

	va_start(vlist, format);
	nCharacters = vswprintf(buffer, _countof(buffer) - 1, format, vlist);
	va_end(vlist);

	ASSERT(nCharacters <= sizeof(buffer));

	GPrintxCs.EnterCriticalSection();
	_cwprintf(buffer);
	GPrintxCs.LeaveCriticalSection();

	return nCharacters;
//#endif
}

void PrintMACaddress(BYTE *MacAddress)
{
	printx("Mac: %02x-%02x-%02x-%02x-%02x-%02x\n", MacAddress[0], MacAddress[1], MacAddress[2], MacAddress[3], MacAddress[4], MacAddress[5]);
}

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
BOOL Is64BitsOs()
{
#ifdef WIN64
#error The implementation of the function is for 32bits only.
#endif

	LPFN_ISWOW64PROCESS fnIsWow64Process = 0;
	BOOL bIsWow64 = FALSE;

	// IsWow64Process is not available on all supported versions of Windows.
	// Use GetModuleHandle to get a handle to the DLL that contains the function and GetProcAddress to get a pointer to the function if available.
	fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress( GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if(NULL != fnIsWow64Process)
		if(fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
			return bIsWow64;

	return bIsWow64;
}

void ParseDateTime(CHAR *pDate, CHAR *pTime, INT &year, INT &month, INT &date, INT &hour, INT &minute)
{
	CHAR *pMonth[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	for(INT i = 0; i < 12; ++i)
		if(pDate[0] == pMonth[i][0] && pDate[1] == pMonth[i][1] && pDate[2] == pMonth[i][2])
		{
			month = ++i;
			break;
		}

	date = 0;
	if(pDate[4] != ' ')
		date = (pDate[4] - '0') * 10;
	date += (pDate[5] - '0');

	year = 0;
	for(INT i = 7, time = 1000; i < 11; ++i, time /= 10)
		year += (pDate[i] - '0') * time;

	hour = 0;
	hour += (pTime[0] - '0') * 10;
	hour += (pTime[1] - '0');

	minute = 0;
	minute += (pTime[3] - '0') * 10;
	minute += (pTime[4] - '0');
}

BOOL AppAutoStart(const TCHAR *pItemName, const TCHAR *pExePath, const TCHAR *pParam, BOOL bStart)
{
	DWORD len;
	TCHAR buffer[512];

	if(pExePath)
	{
		len = _tcslen(pExePath);
		memcpy(buffer, pExePath, sizeof(TCHAR) * len);
		buffer[len] = 0;

		if(pParam)
		{
			buffer[len]	= ' ';
			buffer[len + 1]	= 0;
			_tcscat(buffer, pParam);

			ASSERT(_tcslen(buffer) == (len + 1 + _tcslen(pParam)));
			len += (1 + _tcslen(pParam));
		}
	}

	HKEY hKey;
	//LONG hr = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey);
	LONG hr = RegOpenKey(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), &hKey);

	if(hr == ERROR_SUCCESS)
	{
		if(bStart)
			hr = RegSetValueEx(hKey, pItemName, 0, REG_SZ, (BYTE*)buffer, len * sizeof(TCHAR));
		else
			hr = RegDeleteValue(hKey, pItemName);

		RegCloseKey(hKey);
	}

	if(hr != ERROR_SUCCESS)
	{
		printx("AppAutoStart() Failed! hr: %08x, Error code: %d\n", hr, GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL SaveResourceToFile(const TCHAR *strPath, WORD ResID, HMODULE hInstance)
{
	if(!hInstance)
		hInstance = GetModuleHandle(0);
	HRSRC hrsrc = FindResource(hInstance, MAKEINTRESOURCE(ResID), RT_RCDATA);
//	HRSRC hrsrc = FindResource(hInstance, MAKEINTRESOURCE(ResID), RT_BITMAP);
	if(!hrsrc)
		return FALSE;

	DWORD size = SizeofResource(hInstance, hrsrc);
	HGLOBAL hglob = LoadResource(hInstance, hrsrc);
	LPVOID rdata = LockResource(hglob);

	HANDLE hFile = CreateFile(strPath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile)
	{
		DWORD writ;
		WriteFile(hFile, rdata, size, &writ, NULL);
		ASSERT(writ == size);
		CloseHandle(hFile);
		if(writ == size)
			return TRUE;
	}

	return FALSE;
}


