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
#include "CoreAPI.h"
#include <Softpub.h>
#include <wintrust.h>


stClientInfo* stClientInfo::pClientInfo = 0;


INT CoreWriteString(CStreamBuffer &sb, const TCHAR *pStr, INT iLen)
{
#ifdef UCHAR_AS_UTF8

	INT iUtf8Len = 0;
	BYTE byLen[MAX_BYTE_COUNT_FOR_VAR_INT];
	uChar ubuf[1024], *pUCharBuf = ubuf;

	if(iLen < 0)
		iLen = _tcslen(pStr);

	if(iLen)
	{
		iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, _countof(ubuf), NULL, NULL);
		if(iUtf8Len == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, 0, NULL, NULL);
			if((pUCharBuf = (uChar*)malloc(iUtf8Len)) == NULL) // No need to include null-terminator here.
				return -1;
			iUtf8Len = WideCharToMultiByte(CP_UTF8, 0, pStr, iLen, pUCharBuf, iUtf8Len, NULL, NULL);
		}
	}

	UINT ByteCount = EncodeVarInt(iUtf8Len, byLen);
	sb.Write(byLen, ByteCount);
	sb.Write(pUCharBuf, iUtf8Len);
	ASSERT(sb.GetDataSize() <= sb.GetBufferSize());
	if(pUCharBuf != ubuf)
		free(pUCharBuf);

	return iUtf8Len;

#else
	ASSERT(0);
#endif
}

INT CoreReadString(CStreamBuffer &sb, TCHAR *pBuf, INT *iBufLen)
{
#ifdef UCHAR_AS_UTF8

	uChar ubuf[1024], *pUCharBuf = ubuf;
	UINT n, ByteCount = DecodeVarInt(n, sb.GetCurrentBuffer(), MAX_BYTE_COUNT_FOR_VAR_INT);

	if(n >= _countof(ubuf))
		if((pUCharBuf = (uChar*)malloc(n + sizeof(uChar))) == NULL)
			return -1;

	sb.Skip(ByteCount);

	if(n != 0)
	{
		sb.Read(pUCharBuf, sizeof(uChar) * n);
		pUCharBuf[n] = 0;

		INT iNewLen = MultiByteToWideChar(CP_UTF8, 0, pUCharBuf, n + 1, pBuf, *iBufLen);
		if(iNewLen != 0)
			iNewLen--;
		else
		{
			if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
				*iBufLen = MultiByteToWideChar(CP_UTF8, 0, pUCharBuf, n + 1, NULL, 0);
			iNewLen = -1;
		}

		if(pUCharBuf != ubuf)
			free(pUCharBuf);

		return iNewLen; // Not include null-terminator if no error.
	}

	pBuf[0] = '\0';
	return 0;

#else
	ASSERT(0);
#endif
}

INT CoreReadString(CStreamBuffer &sb, CString &out)
{
#ifdef UCHAR_AS_UTF8

	uChar ubuf[1024], *pUCharBuf = ubuf;
	UINT n, ByteCount = DecodeVarInt(n, sb.GetCurrentBuffer(), MAX_BYTE_COUNT_FOR_VAR_INT);

	if(n >= _countof(ubuf))
		if((pUCharBuf = (uChar*)malloc(n + sizeof(uChar))) == NULL)
			return -1;

	sb.Skip(ByteCount);

	if (n == 0)
		out = _T("");
	else
	{
		sb.Read(pUCharBuf, sizeof(uChar) * n);
		pUCharBuf[n] = 0;

		LPTSTR pBuf = out.GetBuffer(n + 1);
		INT iNewLen = MultiByteToWideChar(CP_UTF8, 0, pUCharBuf, n + 1, pBuf, n + 1);
		out.ReleaseBuffer(iNewLen);

		if(pUCharBuf != ubuf)
			free(pUCharBuf);

		return iNewLen;
	}

	return 0;

#else
	ASSERT(0);
#endif
}

stGUIEventMsg* BuildDataStream(stVPNet *pJoinNet)
{
	CNetworkManager *pNM = AppGetNetManager();
	stGUIEventMsg *pMsg = AllocGUIEventMsg();
	CStreamBuffer sb;
	USHORT NetCount;
	UINT SizeRequired;

	pNM->UpdateGUILinkState();

	if(pJoinNet)
	{
		NetCount = 1;
		SizeRequired = sizeof(NetCount) + pJoinNet->ExportGUIDataStream(0);
	}
	else
	{
		NetCount = pNM->GetNetCount();
		SizeRequired = sizeof(NetCount) + pNM->ExportGUIDataStream(0);
	}

	sb.AttachBuffer((BYTE*)pMsg->Heap(SizeRequired), SizeRequired);
	sb << NetCount;
	if(pJoinNet)
		pJoinNet->ExportGUIDataStream(&sb);
	else
		pNM->ExportGUIDataStream(&sb);
	ASSERT(sb.GetDataSize() == SizeRequired);
	sb.DetachBuffer();

	return pMsg;
}

stGUIEventMsg* BuildRelayInfo()
{
	CStreamBuffer sb;
	CNetworkManager *pNM = AppGetNetManager();
	stGUIEventMsg *pMsg = AllocGUIEventMsg();
	UINT nSize = pNM->ExportRelayDataStream(0);

	sb.AttachBuffer(pMsg->Heap(nSize), nSize);
	pNM->ExportRelayDataStream(&sb);
	sb.DetachBuffer();

	return pMsg;
}

stGUIEventMsg* SetupGUIUpdateData(CMemberUpdateList &MemberUpdateList)
{
	CStreamBuffer sb;
	stGUIEventMsg *pMsg = AllocGUIEventMsg();
	if(pMsg)
	{
		UINT nHeapSize = MemberUpdateList.GetStreamSize();
		if(sb.AttachBuffer(pMsg->Heap(nHeapSize), nHeapSize))
		{
			MemberUpdateList.WriteStream(sb);
			ASSERT(sb.GetDataSize() == nHeapSize);
			sb.DetachBuffer();
		}
		else
		{
			ReleaseGUIEventMsg(pMsg);
			pMsg = 0;
		}
	}
	return pMsg;
}


void CoreGetSystemInfo(IPV4 ip, CStreamBuffer &sb)
{
	stClientSystemInfo SystemInfo = {0};
	SystemInfo.dwSystemInfoSize = sizeof(SystemInfo);

	OSVERSIONINFOEX VerInfo = {0};
	VerInfo.dwOSVersionInfoSize = sizeof(VerInfo);
	GetVersionEx((OSVERSIONINFO*)&VerInfo);

	SystemInfo.dwMajorVersion = VerInfo.dwMajorVersion;
	SystemInfo.dwMinorVersion = VerInfo.dwMinorVersion;
	SystemInfo.dwBuildNumber = VerInfo.dwBuildNumber;
	SystemInfo.IsNTServer = (VerInfo.wProductType != VER_NT_WORKSTATION) ? TRUE : FALSE;
	SystemInfo.Is64Bit = Is64BitsOs();
	SystemInfo.nMaxUpload = AppGetClientInfo()->ConfigData.DataOutLimit;
	SystemInfo.nMaxDownload = AppGetClientInfo()->ConfigData.DataInLimit;

	IP_ADAPTER_INFO *pAdptInfo = 0, *pNextAd = 0;
	ULONG ulLen	= 0, error;
	BOOL bFound = FALSE;
	error = ::GetAdaptersInfo(pAdptInfo, &ulLen);
	if(error == ERROR_BUFFER_OVERFLOW)
	{
		pAdptInfo = (IP_ADAPTER_INFO*)malloc(ulLen);
		error = ::GetAdaptersInfo(pAdptInfo, &ulLen);

		if(error == ERROR_SUCCESS)
		{
			pNextAd = pAdptInfo;
			CStringA AdapterDescA, VersionA;
			CString AdapterDescW, VersionW;

			while(pNextAd)
			{
		//		printx("Adapter Desc: %s. Index: %d\n", pNextAd->Description, pNextAd->Index);
				IP_ADDR_STRING *pList = &pNextAd->IpAddressList;
				while(pList)
				{
					if(inet_addr(pList->IpAddress.String) == ip)
					{
						bFound = TRUE;
						AdapterDescA = pNextAd->Description;
						INT pos = AdapterDescA.Find(" - Packet"); // Skip " - Packet Scheduler Miniport".
						if(pos != -1)
							AdapterDescA = AdapterDescA.Left(pos);
						pos = AdapterDescA.GetLength();
						if(pos >= sizeof(SystemInfo.AdapterDesc))
							pos = sizeof(SystemInfo.AdapterDesc) - 1;
						if(pos > 0)
							memcpy(SystemInfo.AdapterDesc, AdapterDescA.GetBuffer(), pos);
						SystemInfo.Len1 = pos;

						AdapterDescW = AdapterDescA;
						if(GetAdapterDriverVersion(AdapterDescW, VersionW))
						{
							VersionA = VersionW;
							pos = VersionA.GetLength();
							if(pos >= sizeof(SystemInfo.DriverVersion))
								pos = sizeof(SystemInfo.DriverVersion) - 1;
							if(pos > 0)
								memcpy(SystemInfo.DriverVersion, VersionA.GetBuffer(), pos);
							SystemInfo.Len2 = pos;
						//	printx("Driver version: %s\n", VersionA);
						}

						break;
					}
				//	printx("IPV4: %s\n", pList->IpAddress.String);
					pList = pList->Next;
				}

				if(bFound)
					break;
				pNextAd = pNextAd->Next;
			}
		}
		free(pAdptInfo);
	}

	sb.Write(&SystemInfo, sizeof(SystemInfo));
}


typedef LONG (WINAPI *WinVerifyTrustFn)(HWND, GUID *, LPVOID);


struct CHybridClass
{
	union
	{
		CHAR c[16];
		UINT ui[4];
	};
};


void PrintSignatureVerifyResult(LONG lStatus, TCHAR *FilePath)
{
	//DWORD dwLastError;

	//switch (lStatus)
	//{
	//	case ERROR_SUCCESS:
	//		/*
	//		Signed file:
	//			- Hash that represents the subject is trusted.

	//			- Trusted publisher without any verification errors.

	//			- UI was disabled in dwUIChoice. No publisher or time stamp chain errors.

	//			- UI was enabled in dwUIChoice and the user clicked "Yes" when asked to install and run the signed subject.
	//		*/
	//		printx(L"The file \"%s\" is signed and the signature was verified.\n", FilePath);
	//		break;

	//	case TRUST_E_NOSIGNATURE:
	//		// The file was not signed or had a signature that was not valid.

	//		dwLastError = GetLastError();
	//		if (TRUST_E_NOSIGNATURE == dwLastError || TRUST_E_SUBJECT_FORM_UNKNOWN == dwLastError || TRUST_E_PROVIDER_UNKNOWN == dwLastError)
	//			printx(L"The file \"%s\" is not signed.\n", FilePath); // The file was not signed.
	//		else
	//			// The signature was not valid or there was an error opening the file.
	//			printx(L"An unknown error occurred trying to verify the signature of the \"%s\" file.\n", FilePath);
	//		break;

	//	case TRUST_E_EXPLICIT_DISTRUST:
	//		// The hash that represents the subject or the publisher is not allowed by the admin or user.
	//		printx(L"The signature is present, but specifically disallowed.\n");
	//		break;

	//	case TRUST_E_SUBJECT_NOT_TRUSTED:
	//		// The user clicked "No" when asked to install and run.
	//		printx(L"The signature is present, but not trusted.\n");
	//		break;

	//	case CRYPT_E_SECURITY_SETTINGS:
	//		/*
	//			The hash that represents the subject or the publisher was not explicitly trusted by the admin and the
	//			admin policy has disabled user trust. No signature, publisher or time stamp errors.
	//		*/
	//		printx(L"CRYPT_E_SECURITY_SETTINGS - The hash representing the subject or the publisher wasn't explicitly trusted "
	//			L"by the admin and admin policy has disabled user trust. No signature, publisher or timestamp errors.\n");
	//		break;

	//	default:
	//		// The UI was disabled in dwUIChoice or the admin policy has disabled user trust. lStatus contains the publisher or time stamp chain error.
	//		printx(L"Error code: 0x%x.\n", lStatus);
	//		/*
	//			80096001 TRUST_E_SYSTEM_ERROR A system-level error occured while verifying trust.
	//			80096002 TRUST_E_NO_SIGNER_CERT The certificate for the signer of the message is invalid or not found.
	//			80096003 TRUST_E_COUNTER_SIGNER Invalid counter signer.
	//			80096004 TRUST_E_CERT_SIGNATURE The signature of the certificate cannot be verified.
	//			80096005 TRUST_E_TIME_STAMP The time stamp signer and or certificate could not be verified or is malformed.
	//			80096010 TRUST_E_BAD_DIGEST The objects digest did not verify.
	//			80096019 TRUST_E_BASIC_CONSTRAINTS Certificate basic constraints invalid or missing.
	//			8009601E TRUST_E_FINANCIAL_CRITERIA The certificate does not meet or contain the Authenticode financial extensions.
	//		*/
	//		break;
	//}
}

DWORD CoreVerifyEmbeddedSignature(DWORD &dwSignatureVerifyResult)
{
	LONG lStatus;
	BOOL bResult = FALSE;
	TCHAR FilePath[MAX_PATH];
	CHybridClass a;
	DWORD dwErrorCode = 0;
	dwSignatureVerifyResult = 0;

//	memcpy(&a.c, "Wintrust.dll", 13);
	a.ui[0] = 0x98C203B6 ^ 0xECAC6AE1;
	a.ui[1] = 0x9513075F ^ 0xE160722D;
	a.ui[2] = 0x321E04D3 ^ 0x5E7260FD;
	a.ui[3] = 0x854A1249 ^ 0x4986DE49;

	HMODULE hMod = LoadLibraryA(a.c); // Wintrust.dll
	if(!hMod)
		return GetLastError();

//	memcpy(&a.c, "WinVerifyTrust", 15);
	a.ui[0] = 0x98C203B6 ^ 0xCEAC6AE1;
	a.ui[1] = 0x9513075F ^ 0xF37A753A;
	a.ui[2] = 0x321E04D3 ^ 0x476C50AA;
	a.ui[3] = 0x854A1249 ^ 0x494A663A;

	WinVerifyTrustFn WinVerifyTrustFp = (WinVerifyTrustFn)GetProcAddress(hMod, a.c); // WinVerifyTrust
	if(WinVerifyTrustFp)
	{
		GetModuleFileName(GetModuleHandle(0), FilePath, sizeof(FilePath) / sizeof(TCHAR));

		WINTRUST_FILE_INFO FileData;
		memset(&FileData, 0, sizeof(FileData));
		FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
		FileData.pcwszFilePath = FilePath;
		FileData.hFile = NULL;
		FileData.pgKnownSubject = NULL;

		GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

		WINTRUST_DATA WinTrustData;
		memset(&WinTrustData, 0, sizeof(WinTrustData));
		WinTrustData.cbStruct = sizeof(WinTrustData);
		WinTrustData.pPolicyCallbackData = NULL; // Use default code signing EKU.
		WinTrustData.pSIPClientData = NULL;      // No data to pass to SIP.
		WinTrustData.dwUIChoice = WTD_UI_NONE;   // Disable WVT UI.
		WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE; // No revocation checking.
		WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;       // Verify an embedded signature on a file.
		WinTrustData.dwStateAction = 0;          // Default verification.
		WinTrustData.hWVTStateData = NULL;       // Not applicable for default verification of embedded signature.
		WinTrustData.pwszURLReference = NULL;

		// This is not applicable if there is no UI because it changes
		// the UI to accommodate running applications instead of installing applications.
		WinTrustData.dwUIContext = 0;
		WinTrustData.pFile = &FileData; // Set pFile.

		lStatus = WinVerifyTrustFp(NULL, &WVTPolicyGUID, &WinTrustData);
		PrintSignatureVerifyResult(lStatus, FilePath);
		dwSignatureVerifyResult = lStatus;
	}
	else
	{
		dwErrorCode = GetLastError();
	}

	FreeLibrary(hMod);

	return dwErrorCode;
}



TCHAR *GCRCString[CRC_ALL] = { _T("ClientInfo"), _T("config"), _T("GUI") };


HKEY CoreOpenReg(DWORD dwCategory)
{
	ASSERT(dwCategory < CRC_ALL);

	DWORD dwError;
	HKEY hKey, hAppKey = 0;
	TCHAR *pRegDefaultPath = _T("Software\\DigiStar Studio\\nMatrixClient");

	CString Path;
	Path.Format(_T("%s\\%s"), pRegDefaultPath, GCRCString[dwCategory]);

	dwError = RegOpenCurrentUser(KEY_ALL_ACCESS, &hKey);
	if(dwError != ERROR_SUCCESS)
		printx("RegOpenCurrentUser failed! ec: %d\n", dwError);
	else
	{
		dwError = RegOpenKey(hKey, Path, &hAppKey);
		if(dwError != ERROR_SUCCESS)
		{
			printx("RegOpenKey failed! ec: %d\n", dwError);
			if(dwError == ERROR_FILE_NOT_FOUND)
			{
				dwError = RegCreateKey(hKey, Path, &hAppKey);
				if(dwError != ERROR_SUCCESS)
					printx("RegCreateKey failed! ec: %d\n", dwError);
			}
		}
		RegCloseKey(hKey);
	}

	return hAppKey;
}

BOOL CoreReadRegINT(DWORD dwCategory, TCHAR *pSection, INT *pValue)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD size = sizeof(*pValue), type, dwError;
	dwError = RegQueryValueEx(hKey, pSection, 0, &type, (BYTE*)pValue, &size);
	if(dwError != ERROR_SUCCESS)
		printx("RegQueryValueEx failed! ec:%d\n", dwError);
	else
	{
		ASSERT(type == REG_DWORD);
	}
	RegCloseKey(hKey);

	return !dwError;
}

BOOL CoreWriteRegINT(DWORD dwCategory, TCHAR *pSection, INT iValue)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD dwError;
	dwError = RegSetValueEx(hKey, pSection, 0, REG_DWORD, (BYTE*)&iValue, sizeof(iValue));
	if(dwError != ERROR_SUCCESS)
		printx("RegSetValueEx failed! ec:%d\n", dwError);
	RegCloseKey(hKey);

	return !dwError;
}

BOOL CoreReadRegString(DWORD dwCategory, TCHAR *pSection, TCHAR *pBuffer, UINT *pBufferSize)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD type, dwError;
	dwError = RegQueryValueEx(hKey, pSection, 0, &type, (BYTE*)pBuffer, (DWORD*)pBufferSize);
	if(dwError != ERROR_SUCCESS)
		printx("RegQueryValueEx failed! ec:%d\n", dwError);
	else
	{
		ASSERT(type == REG_SZ);
	}
	RegCloseKey(hKey);

	return !dwError;
}

BOOL CoreWriteRegString(DWORD dwCategory, TCHAR *pSection, TCHAR *pString)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD dwError, dwSize = (_tcslen(pString) + 1) * sizeof(TCHAR);
	dwError = RegSetValueEx(hKey, pSection, 0, REG_SZ, (BYTE*)pString, dwSize);
	if(dwError != ERROR_SUCCESS)
		printx("RegSetValueEx failed! ec:%d\n", dwError);
	RegCloseKey(hKey);

	return !dwError;
}

BOOL CoreReadRegData(DWORD dwCategory, TCHAR *pSection, void *pData, UINT *pBufferSize)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD type, dwError;
	dwError = RegQueryValueEx(hKey, pSection, 0, &type, (BYTE*)pData, (DWORD*)pBufferSize);
	if(dwError != ERROR_SUCCESS)
		printx("RegQueryValueEx failed! ec:%d\n", dwError);
	else
	{
		ASSERT(type == REG_BINARY);
	}
	RegCloseKey(hKey);

	return !dwError;
}

BOOL CoreWriteRegData(DWORD dwCategory, TCHAR *pSection, void *pData, UINT nDataSize)
{
	HKEY hKey = CoreOpenReg(dwCategory);
	if(!hKey)
		return FALSE;

	DWORD dwError;
	dwError = RegSetValueEx(hKey, pSection, 0, REG_BINARY, (BYTE*)pData, nDataSize);
	if(dwError != ERROR_SUCCESS)
		printx("RegSetValueEx failed! ec:%d\n", dwError);
	RegCloseKey(hKey);

	return !dwError;
}

// Temp method.
#include "VPN Client.h"

BOOL CoreSaveRegisterID(stRegisterID *pID)
{
	return AppSaveRegisterID(pID);
}

BOOL CoreLoadRegisterID(stRegisterID *pID)
{
	return AppLoadRegisterID(pID);
}

void CoreReadConfigData(stConfigData *pConfig)
{
	theApp.ReadConfigData(pConfig);
}

void CoreSaveConfigData(stConfigData *pConfig)
{
	theApp.SaveConfigData(pConfig);
}

void CoreSaveClosedMsgID(UINT nID)
{
	return theApp.SaveClosedMsgID(nID);
}

UINT CoreLoadClosedMsgID()
{
	return theApp.LoadClosedMsgID();
}

void CoreSaveSvcLoginState(BOOL bOnline)
{
	theApp.SaveSvcLoginState(bOnline);
}

BOOL CoreLoadSvcLoginState()
{
	return theApp.LoadSvcLoginState();
}


