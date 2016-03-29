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


#include "stdafx.h"
#include <setupapi.h>
#include <devguid.h>
#include <newdev.h>
#include <netlistmgr.h>
#include "DriverAPI.h"
#include "UTX.h"
#include "VPN Client.h"


#pragma comment(lib, "Newdev.lib")
#pragma comment(lib, "Setupapi.lib")


#define MAX_CLASS_NAME_LEN 128


struct stTemp
{
	union
	{
		DWORD dw;
		struct
		{
			BYTE b1, b2, b3, b4;
		};
	};
};

void StringToByte(TCHAR *pString, BYTE *pOut)
{
	INT i;
	TCHAR temp[3] = {0};
	DWORD value;

	for(i = 0; i < 6; i++)
	{
		temp[0] = pString[i * 2];
		temp[1] = pString[i * 2 + 1];

		_stscanf(temp, _T("%x"), &value);

		pOut[i] = (BYTE)value;
	}
}

HANDLE OpenAdapter(BOOL bOverlapped)
{
	DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	if(bOverlapped)
		dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
	return CreateFile(_T(ADAPTER_NAME), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, dwFlagsAndAttributes, 0);
}

BOOL CloseAdapter(HANDLE hAdapter)
{
	return CloseHandle(hAdapter);
}

HKEY GetAdapterRegKey(CString str, BOOL bComponentID)
{
	HKEY hKey, hSubKey = 0;
	LONG hr = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"), &hKey);
	if(hr != ERROR_SUCCESS)
		return 0;

	DWORD i, keyCount, type, size;
	TCHAR buffer[64], buffer2[256];

	if(bComponentID)
		str.MakeLower();
	RegQueryInfoKey(hKey, 0, 0, 0, &keyCount, 0, 0, 0, 0, 0, 0, 0);

	for(i = 0; i < keyCount; i++)
	{
		RegEnumKey(hKey, i, buffer, sizeof(buffer));
		hr = RegOpenKey(hKey, buffer, &hSubKey);
		if(hr == ERROR_SUCCESS)
		{
			size = sizeof(buffer2);
			//RegGetValue(hKey, buffer, _T("ComponentId"), RRF_RT_REG_SZ, 0, buffer2, 0); // Xp not supported.
			if(bComponentID)
				hr = RegQueryValueEx(hSubKey, _T("ComponentId"), 0, &type, (BYTE*)buffer2, &size);
			else
				hr = RegQueryValueEx(hSubKey, _T("DriverDesc"), 0, &type, (BYTE*)buffer2, &size);

			if(hr == ERROR_SUCCESS && str == buffer2)
				break;
			RegCloseKey(hSubKey);
			hSubKey = 0;
		}
	}
	RegCloseKey(hKey);

	return hSubKey;
}

BOOL GetAdapterDriverVersion(CString DriverDesc, CString &version)
{
	HKEY hKey = GetAdapterRegKey(DriverDesc, FALSE);
	if(!hKey)
		return FALSE;

	TCHAR buffer[128];
	DWORD type, size = sizeof(buffer);
	BOOL  bResult = FALSE;

	if(RegQueryValueEx(hKey, _T("DriverVersion"), 0, &type, (BYTE*)buffer, &size) == ERROR_SUCCESS && type == REG_SZ)
	{
		version = buffer;
		bResult = TRUE;
	}
	RegCloseKey(hKey);

	return bResult;
}

void GetAdapterRegInfo(BYTE Mac[], DWORD *ip, DWORD *TaskOffload)
{
	HKEY hKey = GetAdapterRegKey(_T(ADAPTER_HARDWARE_ID), TRUE);
	if(!hKey)
		return;

	TCHAR buffer[64];
	DWORD type, size, vip;
	LONG result;

	size = sizeof(buffer);
	result = RegQueryValueEx(hKey, _T("NetworkAddress"), 0, &type, (BYTE*)buffer, &size);
	if(result ==  ERROR_SUCCESS && size == 13 * sizeof(TCHAR) && Mac)
	{
		StringToByte(buffer, Mac);
		//memcpy(Mac, buffer, size);
	}

	size = sizeof(vip);
	result = RegQueryValueEx(hKey, _T("vip"), 0, &type, (BYTE*)&vip, &size);
	if(result ==  ERROR_SUCCESS && size == 4 && ip)
		*ip = vip;

	if(TaskOffload)
	{
		*TaskOffload = 0; // Turn off task offload by default.
		size = sizeof(*TaskOffload);
		RegQueryValueEx(hKey, _T("TaskOffload"), 0, &type, (BYTE*)TaskOffload, &size);
	}
	RegCloseKey(hKey);
}

BOOL SetAdapterRegInfo(BYTE Mac[], DWORD vip, DWORD TaskOffload)
{
	CString strMac;
	BOOL bResult = FALSE;
	HKEY hKey = GetAdapterRegKey(_T(ADAPTER_HARDWARE_ID), TRUE);
	if(!hKey)
		return bResult;

	if(Mac && vip)
	{
		strMac.Format(_T("%02x%02x%02x%02x%02x%02x"), Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);
		RegSetValueEx(hKey, _T("NetworkAddress"), 0, REG_SZ, (BYTE*)strMac.GetBuffer(), 12 * sizeof(TCHAR));
		RegSetValueEx(hKey, _T("vip"), 0, REG_DWORD, (BYTE*)&vip, sizeof(vip));
		RegSetValueEx(hKey, _T("TaskOffload"), 0, REG_DWORD, (BYTE*)&TaskOffload, sizeof(TaskOffload));
	}

//	RegFlushKey(hKey);
	RegCloseKey(hKey);
	bResult = TRUE;

	return bResult;
}

BOOL OpenUDP(HANDLE hAdapter, DWORD ip, USHORT port, USHORT ipcport)
{
	BOOL  result;
	DWORD nBytesReturn;
	BYTE  buffer[8];

	ChangeByteOrder(ip);
	memcpy(buffer, &ip, sizeof(ip));
	memcpy(buffer + sizeof(ip), &port, sizeof(port));
	memcpy(buffer + sizeof(ip) + sizeof(port), &ipcport, sizeof(ipcport));

	if(!DeviceIoControl(hAdapter, IOCTL_OPEN_UDP, buffer, sizeof(buffer), &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result; // None null if succeeded.
}

BOOL CloseUDP(HANDLE hAdapter)
{
	BOOL  result;
	DWORD nBytesReturn;

	if(!DeviceIoControl(hAdapter, IOCTL_CLOSE_UDP, 0, 0, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}


BOOL GetUDPInfo(HANDLE hAdapter, DWORD &ip, USHORT &port, BOOL &bOpen)
{
//	BOOL  result;
	DWORD nBytesReturn;
	BYTE buffer[256];

	ip = 0;
	port = 0;

	if(!DeviceIoControl(hAdapter, IOCTL_GET_UDP_INFO, 0, 0, buffer, sizeof(buffer), &nBytesReturn, 0))
		return FALSE;

	if(nBytesReturn == (sizeof(ip) + sizeof(port) + sizeof(bOpen)))
	{
		memcpy(&ip, buffer, sizeof(ip));
		memcpy(&port, buffer + sizeof(ip), sizeof(port));
		memcpy(&bOpen, buffer + sizeof(ip) + sizeof(port), sizeof(bOpen));

	//	IPV4 v4 = ip;
	//	printx("UDP: %d - %d.%d.%d.%d:%d\n", bOpen, v4.b1, v4.b2, v4.b3, v4.b4, port);

		return TRUE;
	}

	return FALSE;
}


UINT GetTableData(HANDLE hAdapter, stMapTable *pTable)
{
	UINT count = 0;
	DWORD nBytesReturn;

	if(!pTable)
	{
		DeviceIoControl(hAdapter, IOCTL_GET_TABLE_INFO, 0, 0, &count, sizeof(count), &nBytesReturn, 0);
	}
	else
	{
		DeviceIoControl(hAdapter, IOCTL_GET_TABLE_INFO, 0, 0, pTable, sizeof(stMapTable), &nBytesReturn, 0);
		count = pTable->m_Count;
	}

	return count;
}

BOOL SetTableData(HANDLE hAdapter, stMapTable *pTable)
{
	BOOL  result;
	DWORD nBytesReturn;

	if(!DeviceIoControl(hAdapter, IOCTL_SET_TABLE_DATA, pTable, sizeof(stMapTable), &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}

BOOL CleanTable(HANDLE hAdapter)
{
	BOOL  result = TRUE;
	DWORD nBytesReturn;

	if(!DeviceIoControl(hAdapter, IOCTL_CLEAN_TABLE, 0, 0, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}

BOOL SetTableItem(HANDLE hAdapter, UINT count, UINT index, stEntry *pEntry)
{
	BOOL  result = TRUE;
	DWORD nBytesReturn;

	if(!count || !pEntry || index > MAX_NETWORK_CLIENT || (count + index >= MAX_NETWORK_CLIENT))
		return FALSE;

	stSetItemData entry;
	memcpy(entry.entry, pEntry, sizeof(stEntry) * count);
	entry.count = count;
	entry.index = index;

	//for(UINT i = 0; i < count; i++)
	//{
	//	ChangeByteOrder(entry.entry[i].vip);
	//	ChangeByteOrder(entry.entry[i].pip);
	//}

	nBytesReturn = sizeof(entry.count) + sizeof(entry.index) + sizeof(stEntry) * count;
	if(!DeviceIoControl(hAdapter, IOCTL_SET_TABLE_ITEM, &entry, nBytesReturn, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}

struct stAddEntryData
{
	UINT count;
	stEntry in[MAX_NETWORK_CLIENT];
};

BOOL AddTableEntry(HANDLE hAdapter, stEntry *pEntry, UINT count)
{
	BOOL  result;
	DWORD nBytesReturn;

	if(!pEntry || !count || count > MAX_NETWORK_CLIENT)
		return FALSE;

	stAddEntryData entryInfo;
	entryInfo.count = count;
	memcpy(entryInfo.in, pEntry, sizeof(stEntry) * count);

	//for(UINT i = 0; i < count; i++)
	//{
	//	ChangeByteOrder(entryInfo.in[i].vip);
	//	ChangeByteOrder(entryInfo.in[i].pip);
	//}

	nBytesReturn = sizeof(entryInfo.count) + sizeof(stEntry) * count;
	if(!DeviceIoControl(hAdapter, IOCTL_ADD_ADDRESS, &entryInfo, nBytesReturn, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}

BOOL SetEntryData(HANDLE hAdapter, INT Index, INT nType, INT value)
{
	if(Index >= MAX_NETWORK_CLIENT)
		return FALSE;

	BOOL  result;
	DWORD nBytesReturn;
	INT Data[3] = { Index, nType, value };

	nBytesReturn = sizeof(Data);
	if(!DeviceIoControl(hAdapter, IOCTL_SET_ENTRY_DATA, Data, nBytesReturn, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return result;
}

BOOL EnableUserMode(HANDLE hAdapter, DWORD dwMode, BOOL bEnable)
{
	const INT MaxLen = 128;
	struct stInput
	{
		UINT  Mode, bEnable; // Mode 1: control send event. Mode 2: control receive event.
	} InputBuffer;

	InputBuffer.Mode = dwMode;
	InputBuffer.bEnable = bEnable;

	BOOL bResult;
	DWORD nBytesReturn = 0;
	if(!DeviceIoControl(hAdapter, IOCTL_ATTACH_USER_MODE, &InputBuffer, sizeof(InputBuffer), &bResult, sizeof(bResult), &nBytesReturn, 0))
		return FALSE;

	return bResult;
}

BOOL ReadTrafficInfo(HANDLE hAdapter, UINT *pDataIn, UINT *pDataOut, UINT ReadCount)
{
	DWORD nBytesReturn;
	UINT Buffer[MAX_NETWORK_CLIENT * 2];

	if(ReadCount > MAX_NETWORK_CLIENT)
		return FALSE;

	if(!DeviceIoControl(hAdapter, IOCTL_GET_TRAFFIC_INFO, &ReadCount, sizeof(ReadCount), Buffer, ReadCount * sizeof(DWORD) * 2, &nBytesReturn, 0))
		return FALSE;

	if(pDataIn)
		memcpy(pDataIn, Buffer, ReadCount * sizeof(DWORD));
	if(pDataOut)
		memcpy(pDataOut, &Buffer[ReadCount], ReadCount * sizeof(DWORD));

	return TRUE;
}

BOOL WriteTrafficInfo(HANDLE hAdapter, UINT *pDataIn, UINT *pDataOut, UINT Count)
{
	DWORD nBytesReturn;
	UINT Buffer[MAX_NETWORK_CLIENT * 2];

	if(Count > MAX_NETWORK_CLIENT)
		return FALSE;

	memcpy(Buffer, pDataIn, Count * sizeof(DWORD));
	memcpy(&Buffer[Count], pDataOut, Count * sizeof(DWORD));

	if(!DeviceIoControl(hAdapter, IOCTL_SET_TRAFFIC_INFO, Buffer, Count * sizeof(DWORD) * 2, 0, 0, &nBytesReturn, 0))
		return FALSE;

	return TRUE;
}

BOOL DirectSend(HANDLE hAdapter, IpAddress ip, BOOL bIsIPV6, USHORT port, void *pData, UINT size)
{
	stDirectSendInfo SendInfo;
	DWORD nBytesReturn;
	BOOL  result;

	if(!size || size > sizeof(SendInfo.data))
		return FALSE;

	SendInfo.ip = ip;
	SendInfo.port = port;
	SendInfo.datasize = size;
	SendInfo.bIsIPV6 = bIsIPV6;
	memcpy(SendInfo.data, pData, size);

	nBytesReturn = sizeof(SendInfo.ip) + sizeof(SendInfo.port) + sizeof(SendInfo.datasize) + sizeof(SendInfo.bIsIPV6) + size;
	if(!DeviceIoControl(hAdapter, IOCTL_SEND, &SendInfo, nBytesReturn, &result, sizeof(result), &nBytesReturn, 0))
		return FALSE;

	return TRUE;
}

BOOL RegKernelBuffer(HANDLE hAdapter, OVERLAPPED *pwo, void *pBuffer, UINT size, BOOL bSendBuffer, BOOL bOpt)
{
	DWORD nBytesReturned, IoCtrlCode;

	if(bSendBuffer)
	{
		IoCtrlCode = IOCTL_REG_SEND_BUFFER;
	}
	else
	{
		IoCtrlCode = IOCTL_REG_RECV_BUFFER;
		if(pBuffer && size >= 4)
		{
			if(bOpt)
				((stReadBuffer*)pBuffer)->DataCount = 1; // Use internal buffer.
			else
				((stReadBuffer*)pBuffer)->DataCount = 0;
		}
	}

	printx("---> RegKernelBuffer. %s Buffer: %08x Size: %d\n", bSendBuffer ? "Send" : "Receive", pBuffer, size);
	ASSERT(size == 0 || ((size - 8) % sizeof(stBufferSlot) == 0));

	if(pBuffer)
	{
		if(!DeviceIoControl(hAdapter, IoCtrlCode, pBuffer, size, pBuffer, size, &nBytesReturned, pwo))
		{
			printx("ec: %d\n", GetLastError());
			return FALSE;
		}
	}
	else // Deregister buffer.
	{
		if(!DeviceIoControl(hAdapter, IoCtrlCode, pBuffer, size, pBuffer, size, &nBytesReturned, 0))
			return FALSE;
	}
	return TRUE;
}

BOOL SetAdapterFunc(HANDLE hAdapter, DWORD dwFuncFlags)
{
	DWORD nBytesReturn = 0, dwResult;
	if(!DeviceIoControl(hAdapter, IOCTL_SET_ADAPTER_FUNC, &dwFuncFlags, sizeof(dwFuncFlags), &dwResult, sizeof(dwResult), &nBytesReturn, 0))
		return FALSE;
	return dwResult;
}

BOOL GetAdapterParam(HANDLE hAdapter, BYTE Mac[], DWORD *vip, BOOL *TaskOffload)
{
	stAdapterInfo staa;

	DWORD nBytesReturn = 0;
	if(!DeviceIoControl(hAdapter, IOCTL_GET_ADDRESS, 0, 0, &staa, sizeof(staa), &nBytesReturn, 0))
		return FALSE;

	ASSERT(nBytesReturn == sizeof(staa));

	if(vip)
		*vip = staa.ipv4;
	if(Mac)
		memcpy(Mac, staa.mac, sizeof(staa.mac));
	if(TaskOffload)
		*TaskOffload = staa.bTaskOffload;

	return TRUE;
}

BOOL SetAdapterParam(HANDLE hAdapter, BYTE Mac[], DWORD vip)
{
	stAdapterInfo staa;

	memcpy(staa.mac, Mac, sizeof(staa.mac));
	staa.ipv4 = vip;

	DWORD nBytesReturn = 0;
	if(!DeviceIoControl(hAdapter, IOCTL_SET_IP_MAC, &staa, sizeof(staa), 0, 0, &nBytesReturn, 0))
		return FALSE;

	return TRUE;
}

BOOL GetAdapterDriverVersion(HANDLE hAdapter, stDriverVerInfo *pOut)
{
	ZeroMemory(pOut, sizeof(*pOut));

	DWORD nBytesReturn = 0;
	if(!DeviceIoControl(hAdapter, IOCTL_DRIVER_VERSION, 0, 0, pOut, sizeof(*pOut), &nBytesReturn, 0))
		return FALSE;

	ASSERT(nBytesReturn == sizeof(stDriverVerInfo) && sizeof(*pOut) == sizeof(stDriverVerInfo));
	return TRUE;
}

BOOL PrintData(HANDLE hAdapter)
{
	DWORD nBytesReturned;

	if(!DeviceIoControl(hAdapter, IOCTL_PRINT_DATA, 0, 0, 0, 0, &nBytesReturned, 0))
		return FALSE;

	return TRUE;
}

void GetDriverState(HANDLE hAdapter, stDriverInternalState *pDriverState)
{
	DWORD nBytesReturned = 0;
	stDriverInternalState DriverState;

	if(!DeviceIoControl(hAdapter, IOCTL_DRIVER_STATE, 0, 0, &DriverState, sizeof(DriverState), &nBytesReturned, 0))
	{
	}
	else if(nBytesReturned == sizeof(UINT)) // The size of the buffer is not enough.
	{
		printx("Size required: %d.\n", *(UINT*)&DriverState);
	}
	else
	{
		memcpy(pDriverState, &DriverState, sizeof(DriverState));

		//printx("\n\n---> DebugCheckResource\n");
		//printx("Send thread ID: %d. Recv thread ID: %d.\n", DriverState.SendThreadID, DriverState.RecvThreadID);
		//printx("TDI Send: %d / %d. TDISyncSend: %d / %d. SecWait: %d / %d. TDI error: %d.\n",
		//	DriverState.TDICom, DriverState.TDISend, DriverState.TDISyncCom, DriverState.TDISyncSend, DriverState.TDIExitSecWait, DriverState.TDISecWait, DriverState.TDIError);
		//printx("Free send slot: %d. Driver counter: %d. Busy: %d. Total: %d.\n", DriverState.nFreeSendSlot, DriverState.nFreeSendSlotDC, DriverState.nBusySendSlot, DriverState.nTotalSendSlot);
		//printx("Free recv slot: %d. Driver counter: %d.\n", DriverState.nFreeRecvSlot, DriverState.nFreeRecvSlotDC);
	}
}

BOOL GetVirtualAdapterInfo(BOOL *pbDHCP, CString *pIP, CString *pGUID, DWORD *pAdapterIndex)
{
	BOOL bFound = FALSE;
	IP_ADAPTER_INFO *pAdptInfo = 0, *pNextAd = 0;
	ULONG ulLen	= 0, error;

	error = ::GetAdaptersInfo(pAdptInfo, &ulLen);
	if(error == ERROR_BUFFER_OVERFLOW)
	{
		pAdptInfo = (IP_ADAPTER_INFO*)malloc(ulLen);
		error = ::GetAdaptersInfo(pAdptInfo, &ulLen);

		if(error == ERROR_SUCCESS)
		{
			pNextAd = pAdptInfo;
			CStringA AdapterDesc(ADAPTER_DESC), desc;
			ulLen = AdapterDesc.GetLength();

			while(pNextAd) // To find adapter.
			{
				desc = pNextAd->Description;
				if(desc.Left(ulLen) == AdapterDesc)
				{
			//		printx("Adapter Name: %s Index: %d\n", pNextAd->AdapterName, pNextAd->Index);
					if(pbDHCP)
						*pbDHCP = pNextAd->DhcpEnabled;
					if(pIP)
						*pIP = pNextAd->IpAddressList.IpAddress.String; // Get only one ip address currently.
					if(pGUID)
						*pGUID = pNextAd->AdapterName;
					if(pAdapterIndex)
						*pAdapterIndex = pNextAd->Index;

					bFound = TRUE;
					break;
				}

				pNextAd = pNextAd->Next;
			}
		}
		free(pAdptInfo);
	}

	return bFound;
}

BOOL RenewAdapter(UINT nReleaseAddressMode)
{
	printx("---> RenewAdapter.\n");

	CString strGUID, strName;
	BOOL bResult = FALSE;
	if(!GetVirtualAdapterInfo(0, 0, &strGUID) || strGUID == _T(""))
		return FALSE;

	IP_INTERFACE_INFO *pInfo = 0;
	ULONG ulLen = 0;
	DWORD err = ::GetInterfaceInfo(pInfo, &ulLen);
	if(err != ERROR_INSUFFICIENT_BUFFER)
		return FALSE;
	else
	{
		pInfo = (IP_INTERFACE_INFO*)malloc(ulLen);
		if(::GetInterfaceInfo(pInfo, &ulLen) != NO_ERROR)
			goto End;
	}

	for(INT i = 0; i < pInfo->NumAdapters; ++i)
	{
		strName = pInfo->Adapter[i].Name;
		printx("%ws\n%ws\n", pInfo->Adapter[i].Name, strName.Right(strGUID.GetLength()));

		if(strName.Right(strGUID.GetLength()) == strGUID)
		{
			printx("GUID found!\n");
			if(nReleaseAddressMode)
				IpReleaseAddress(&pInfo->Adapter[i]);
			if(nReleaseAddressMode != 2)
				IpRenewAddress(&pInfo->Adapter[i]);
			bResult = TRUE;
		}
	}

End:
	free(pInfo);
	return bResult;
}

BOOL ResetAdapter()
{
	printx("---> ResetAdapter.\n");

	TCHAR path[MAX_PATH];
	GetTempPath(MAX_PATH, path);
	CString csDevInsPath(path), csParam;
	ASSERT(csDevInsPath[csDevInsPath.GetLength() - 1] == '\\');
	csDevInsPath += _T(DEVINS_FILE_NAME);
	WORD DevInsResID = Is64BitsOs() ? IDR_DEVINS64 : IDR_DEVINS32;
	BOOL bResult = FALSE;

	if(!SaveResourceToFile(csDevInsPath, DevInsResID))
	{
		printx(_T("Failed to save DevIns file! Path: %s\n"), csDevInsPath);
	}
	else
	{
		STARTUPINFO si = {0};
		PROCESS_INFORMATION pi = {0};
		si.cb = sizeof(si);

		csParam.Format(_T("-r %s"), _T(ADAPTER_DESC));
		if( !CreateProcess( csDevInsPath,			// Module name.
							csParam.GetBuffer(),	// Command line.
							NULL,					// Process handle not inheritable.
							NULL,					// Thread handle not inheritable.
							FALSE,					// Set handle inheritance to FALSE.
							0,						// No creation flags.
							NULL,					// Use parent's environment block.
							NULL,					// Use parent's starting directory.
							&si,					// Pointer to STARTUPINFO structure.
							&pi ))					// Pointer to PROCESS_INFORMATION structure.
		{
			printx("CreateProcess() failed! Error code: %d.\n", GetLastError());
		}
		else
		{
			WaitForSingleObject(pi.hProcess, INFINITE);
			bResult = 0;
			if(!GetExitCodeProcess(pi.hProcess, (DWORD*)&bResult))
				printf("GetExitCodeProcess() failed! Error code: %d.\n", GetLastError());

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		INT nTryCount = 10;
		while(nTryCount--)
			if(DeleteFile(csDevInsPath))
				break;
			else if(GetLastError() == ERROR_ACCESS_DENIED)
			{
				printx("Failed to delete DevIns file(ERROR_ACCESS_DENIED)!\n");
				Sleep(50);
			}
			else
			{
				printx("Failed to delete DevIns file! Error coe: %d.\n", GetLastError());
				break;
			}
	}

	return bResult;
}

BOOL SetNetworkConnectionName(CString TargetAdapterDesc, CString NewName)
{
	CoInitialize(0);

	BOOL bFound = FALSE;
	LPITEMIDLIST pidl = NULL;
	HRESULT hr = SHGetSpecialFolderLocation(0, CSIDL_CONNECTIONS, &pidl);
	if(FAILED(hr))
		goto Exit;

	IShellFolder *pDesktop = 0, *pParentFolder = 0;
	hr = SHGetDesktopFolder(&pDesktop);
	if(FAILED(hr))
		goto Exit;

	hr = pDesktop->BindToObject(pidl, 0, IID_IShellFolder, (void**)&pParentFolder);
	if(FAILED(hr))
		goto Exit;

	STRRET str;
	IEnumIDList *IEnum = 0;
	hr = pParentFolder->EnumObjects(0, SHCONTF_INCLUDEHIDDEN | SHCONTF_NONFOLDERS, &IEnum);
	if(FAILED(hr))
		goto Exit;

	ITEMIDLIST *pItemIDList;
	for(; ;)
	{
		hr = IEnum->Next(1, &pItemIDList, 0);
		if(hr == S_FALSE) // Reach the last item.
			break;

		if(pParentFolder->GetDisplayNameOf(pItemIDList, SHGDN_NORMAL, &str) == S_OK) // Network connection name.
			if(str.uType == STRRET_WSTR)
			{
				printx(_T("%s"), str.pOleStr);
				CoTaskMemFree(str.pOleStr);
			}

		IQueryInfo *pQInfo = 0;
		if(pParentFolder->GetUIObjectOf(0, 1, (LPCITEMIDLIST*)&pItemIDList, IID_IQueryInfo, 0, (void**)&pQInfo) == S_OK)
		{
			TCHAR *pTip;
			if(pQInfo->GetInfoTip(QITIPF_DEFAULT, &pTip) == S_OK)
			{
				CString csTip = pTip;
				if(csTip.GetLength() > TargetAdapterDesc.GetLength())
					csTip = csTip.Left(TargetAdapterDesc.GetLength());
				printx(_T(" - %s"), pTip);
				if(TargetAdapterDesc == csTip)
				{
					bFound = TRUE;
					if(NewName != _T(""))
						hr = pParentFolder->SetNameOf(0, pItemIDList, NewName, SHGDN_NORMAL, 0);
					printx(" (Target found)");
				}
				CoTaskMemFree(pTip);
			}
			pQInfo->Release();
		}

		printx("\n");
		CoTaskMemFree(pItemIDList);
	}

	if(IEnum)
		IEnum->Release();

Exit:
	if(pParentFolder)
		pParentFolder->Release();
	if(pDesktop)
		pDesktop->Release();
	if(pidl)
		CoTaskMemFree(pidl);
	CoUninitialize();

	return bFound;
}

BOOL DoesAdapterMatch(INetwork *pNetwork, CString csAdapterGUID)
{
	BOOL bResult = FALSE;
	CString csEnumGUID;
	CComPtr<IEnumNetworkConnections> pEnumNetworkConnections;
	HRESULT hr = pNetwork->GetNetworkConnections(&pEnumNetworkConnections);

	if (SUCCEEDED(hr))
	{
		GUID guid;
		ULONG cFetched = 0;
		INetworkConnection *pNC[10];

		for (; 1; )
		{
			hr = pEnumNetworkConnections->Next(_countof(pNC), pNC, &cFetched);

			if (SUCCEEDED(hr) && (cFetched > 0))
			{
				for (ULONG i = 0; i < cFetched; i++)
				{
					pNC[i]->GetAdapterId(&guid);
					csEnumGUID.Format(_T("{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}"), guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
									guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
					if(csEnumGUID == csAdapterGUID)
						bResult = TRUE;

					pNC[i]->Release();
				}
			}
			else
				break;
		}
	}

	return bResult;
}


typedef void (*FindINetworkCallback)(INetwork *pINetwork, void *pContext); // Don't release ref in the callback.


void FindAdapterINetworkInterface(FindINetworkCallback Callback, void *pContext)
{
	// Registry locatoin: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\NetworkList\Profiles
	CString strGUID;
	if(!GetVirtualAdapterInfo(0, 0, &strGUID) || strGUID == _T(""))
		return;

	HRESULT hrCoinit = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (SUCCEEDED(hrCoinit) || (RPC_E_CHANGED_MODE == hrCoinit))
	{
		INetworkListManager *pNLM;
		HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNLM);
		if (SUCCEEDED(hr))
		{
			CComPtr<IEnumNetworks> pEnumNetworks;
			hr = pNLM->GetNetworks(NLM_ENUM_NETWORK_ALL, &pEnumNetworks); // NLM_ENUM_NETWORK_CONNECTED
			if (SUCCEEDED(hr))
			{
				INetwork* pNetworks[50];
				ULONG i, cFetched = 0;
				for (; 1; )
				{
					hr = pEnumNetworks->Next(_countof(pNetworks), pNetworks, &cFetched);
					if (SUCCEEDED(hr) && (cFetched > 0))
						for (i = 0; i < cFetched; i++) // Don't break loop.
						{
							if(DoesAdapterMatch(pNetworks[i], strGUID))
							{
						//		printx(_T("Target found!\n"));
								Callback(pNetworks[i], pContext);
							}
							pNetworks[i]->Release();
						}
					else
						break;
				}
			}
			pNLM->Release();
		}
	//	else
	//		printx("Failed to CoCreate INetworkListManager\n");

		if (hrCoinit != RPC_E_CHANGED_MODE)
			CoUninitialize();
	}
}

void SetNT6NetworkNameCB(INetwork *pINetwork, void *pContext)
{
	CString *pNewName = (CString*)pContext;
	BSTR bstr = pNewName->AllocSysString();
	pINetwork->SetName(bstr);
	SysFreeString(bstr);
}

void SetNT6NetworkName(CString NewName)
{
	FindAdapterINetworkInterface(SetNT6NetworkNameCB, &NewName);
}

void SwitchNT6NetworkLocationCB(INetwork *pINetwork, void *pContext)
{
	NLM_NETWORK_CATEGORY nnc;

	if (pINetwork->GetCategory(&nnc) == S_OK)
		if (nnc == NLM_NETWORK_CATEGORY_PUBLIC)
		{
			pINetwork->SetCategory(NLM_NETWORK_CATEGORY_PRIVATE);
		}
		else if(nnc == NLM_NETWORK_CATEGORY_PRIVATE)
		{
			pINetwork->SetCategory(NLM_NETWORK_CATEGORY_PUBLIC);
		}
}

void SwitchNT6NetworkLocation()
{
	FindAdapterINetworkInterface(SwitchNT6NetworkLocationCB, 0);
}


