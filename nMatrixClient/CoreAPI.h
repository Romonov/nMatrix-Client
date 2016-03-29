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


#pragma once


#include "NetManager.h"


enum APP_STATE
{
	AS_DEFAULT,
	AS_CONNECTING,
	AS_LOGIN,
	AS_QUERY_SERVER_TIME,
	AS_DETECT_NAT_FIREWALL,
	AS_QUERY_ADDRESS,
	AS_RETRIEVE_NET_LIST,

	AS_READY
};


struct stClientInfo
{
	stClientInfo()
	:UDPPort(DEFAULT_TUNNEL_PORT), CID(0), QueryCount(0)
	{
		ASSERT(!pClientInfo);
		pClientInfo = this;
		SyncTime = 0;
		SyncTimeScale = 1;
	//	bUseUPNP = TRUE;
		bUseUPNP = FALSE;
		bLastLoginError = FALSE;
		bUseServerSocketEvent = TRUE;
		bOpenFirewall = TRUE;
		bDebugReserveTunnel = FALSE;
		hThreadHandle = 0;
		nTempBufferSize = 0;
		pTemp = NULL;

		iSendLimit = iRecvLimit = 0;
		bSendParamChanged = bRecvParamChanged = 0;

		hRecvThread = NULL;
		hSendThread = NULL;
		hUMRecvEvent = NULL;
		hUMSendEvent = NULL;

		QueryPerformanceFrequency(&QpcFreq);
		InvFreq = 1000 / (DOUBLE)QpcFreq.QuadPart;
	//	printx("QPC frequency: %d\n", (UINT)QpcFreq.QuadPart);

		dwAdapterWaitTime = 0;
		dwNextAutoConnectTime = 0;
	}
	~stClientInfo()
	{
		ASSERT(pClientInfo == NULL || pClientInfo == this);
		pClientInfo = NULL;

		ASSERT(hRecvThread == NULL && hSendThread == NULL);
		ASSERT(hUMRecvEvent == NULL && hUMSendEvent == NULL);
	}


	DWORD  ServerCtrlFlag, AppState, SyncTime, SyncTimeScale; // Period to sync time with server.
	BYTE   bInNat, bInFirewall, bOpenFirewall, bUseUPNP, bLastLoginError, bUseServerSocketEvent, bEipFailed, bDebugReserveTunnel;
	CIpAddress ClientInternalIP, ClientExternalIP, ServerIP;
	USHORT UDPPort, ServerUDPPort1, ServerUDPPort2;
	UINT CID, QueryCount; // Data for querying external address.

	DWORD ID1, ID2; // Client ID as key to communicate with server.
	TCHAR LoginName[LOGIN_ID_LEN + 1], LoginPassword[LOGIN_PASSWORD_LEN + 1];
	DWORD vip;
	BYTE  vmac[6];
	stRegisterID RegID;
	CMapTable MapTable;

	volatile HANDLE hThreadHandle; // Handle of the connecting thread.

	// Data for retrieve net list.
	UINT nTempBufferSize; // Current data size in temp buffer.
	BYTE *pTemp; // The pointer to temp buffer.

	// Aes data.
	CHAR ClientKey[AES_KEY_LENGTH / 8 + 1];
	stConfigData ConfigData;

	// Data for bandwitdh control.
	UINT iSendLimit, iRecvLimit;
	BOOL bSendParamChanged, bRecvParamChanged;

	// Data for user mode socket.
	volatile BOOL bEnableUM;
	HANDLE hRecvThread, hSendThread;
	HANDLE hUMRecvEvent, hUMSendEvent;

	// Server synchronization data.
	DWORD QueryServerTimeCounter;
	LARGE_INTEGER QpcFreq;
	DOUBLE ServerTimeOffset, InvFreq;

	DWORD dwAdapterWaitTime, dwNextAutoConnectTime;

	DWORD dwServerMsgID, dwClosedServerMsgID;
	CString csServerNews;

	LARGE_INTEGER PingHostStartTime;


	DOUBLE GetServerTime()
	{
		LARGE_INTEGER ctime;
		QueryPerformanceCounter(&ctime);
		return ctime.QuadPart * InvFreq - ServerTimeOffset;
	}
	void SetDefault()
	{
		if (pTemp != NULL)
		{
			free(pTemp);
			pTemp = NULL;
			nTempBufferSize = 0;
		}
		QueryServerTimeCounter = 0;
		QueryCount = 0;
	}


protected:

	friend stClientInfo* AppGetClientInfo();
	static stClientInfo* pClientInfo;


};


struct stClientSystemInfo
{
	DWORD dwSystemInfoSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	BYTE IsNTServer, Is64Bit;

	BYTE Len1, Len2;
	CHAR AdapterDesc[64];
	CHAR DriverVersion[40];

	UINT nMaxUpload, nMaxDownload;
	DWORD dwReserved[100];
};


inline stClientInfo* AppGetClientInfo() { return stClientInfo::pClientInfo; }
inline void AppSetState(DWORD state) { AppGetClientInfo()->AppState = state; }
inline DWORD AppGetState() { return AppGetClientInfo()->AppState; }


template <class T>
INT CoreWriteString(CStreamBuffer &sb, T LenSize, const TCHAR *pStr, INT iLen = -1)
{
#ifdef UCHAR_AS_UTF8

	INT iUtf8Len = 0;
	uChar ubuf[1024], *pUCharBuf = ubuf;

	if (iLen < 0)
		iLen = _tcslen(pStr);

	if (iLen)
	{
		iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, _countof(ubuf), NULL, NULL);
		if (iUtf8Len == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, 0, NULL, NULL);
			if ((pUCharBuf = (uChar*)malloc(iUtf8Len)) != NULL)
				iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, iUtf8Len, NULL, NULL);
			else
				iUtf8Len = 0;
		}
	}

	LenSize = iUtf8Len;
	sb.WriteString(LenSize, pUCharBuf);
	ASSERT(sb.GetDataSize() <= sb.GetBufferSize());
	if (pUCharBuf != ubuf)
		free(pUCharBuf);

	return iUtf8Len;

#else
	ASSERT(sizeof(uChar) == sizeof(wchar_t));
	return sb.WriteString(LenSize, pStr, iLen);
#endif
}

INT CoreWriteString(CStreamBuffer &sb, const TCHAR *pStr, INT iLen = -1);

INT CoreReadString(CStreamBuffer &sb, TCHAR *pBuf, INT *iBufLen); // Return charater length excluding null terminator.
INT CoreReadString(CStreamBuffer &sb, CString &out);

stGUIEventMsg* BuildDataStream(stVPNet *pJoinNet);
stGUIEventMsg* BuildRelayInfo();
stGUIEventMsg* SetupGUIUpdateData(CMemberUpdateList &MemberUpdateList);


enum CLIENT_REGISTRY_CATEGORY
{
	CRC_CLIENT_INFO,
	CRC_CONFIG,
	CRC_GUI,
	CRC_ALL
};


void CoreGetSystemInfo(IPV4 ip, CStreamBuffer &sb);
DWORD CoreVerifyEmbeddedSignature(DWORD &dwSignatureVerifyResult);

BOOL CoreReadRegINT(DWORD dwCategory, TCHAR *pSection, INT *pValue);
BOOL CoreWriteRegINT(DWORD dwCategory, TCHAR *pSection, INT iValue);
BOOL CoreReadRegString(DWORD dwCategory, TCHAR *pSection, TCHAR *pBuffer, UINT *pBufferSize); // To get string size just passes null pBuffer and non-null pBufferSize.
BOOL CoreWriteRegString(DWORD dwCategory, TCHAR *pSection, TCHAR *pString);
BOOL CoreReadRegData(DWORD dwCategory, TCHAR *pSection, void *pData, UINT *pBufferSize);
BOOL CoreWriteRegData(DWORD dwCategory, TCHAR *pSection, void *pData, UINT nDataSize);

BOOL CoreSaveRegisterID(stRegisterID *pID);
BOOL CoreLoadRegisterID(stRegisterID *pID);
void CoreReadConfigData(stConfigData *pConfig);
void CoreSaveConfigData(stConfigData *pConfig);
void CoreSaveClosedMsgID(UINT nID);
UINT CoreLoadClosedMsgID();
void CoreSaveSvcLoginState(BOOL bOnline);
BOOL CoreLoadSvcLoginState();


