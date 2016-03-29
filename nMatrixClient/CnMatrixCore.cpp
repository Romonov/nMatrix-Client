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
#include "CnMatrixCore.h"
#include "CoreAPI.h"
#include "SocketSP.h"
#include "JobQueue.h"
#include "resource.h"
#include "SetupDialog.h"
#include "VPN Client.h"


#define WM_GUI_EVENT (WM_USER + 1)


CnMatrixCore* CnMatrixCore::pnMatrixCore = NULL;
HWND CnMatrixCore::hGUIHandle = NULL;


const TCHAR * const GAppString[] =
{
	_T("NULL"),

	_T("GUI"),
	_T("PosX"),
	_T("PosY"),
	_T("Width"),
	_T("Height"),
	_T("%d.%d.%d.%d"),
	_T("\\\\.\\pipe\\nMatrixCore"),
	_T("NMATRIX_SINGLE_INSTANCE_MUTEX"),

};

const TCHAR* APP_STRING(APP_STRING_INDEX index)
{
	ASSERT(index < ASI_TOTAL);
	return GAppString[index];
}


stGUIEventMsg* AllocGUIEventMsg(DWORD UsageFlag)
{
	return new stGUIEventMsg(UsageFlag);
}

void ReleaseGUIEventMsg(stGUIEventMsg *pMsg)
{
	ASSERT(pMsg);
	pMsg->DeleteHeap();
	delete pMsg;
}


BOOL AddJob(stJob *pJob, UINT DataSize)
{
	CnMatrixCore *pnMatrixCore = AppGetnMatrixCore();

	if (pnMatrixCore->IsPureGUIMode())
	{
		pnMatrixCore->PipeSend((BYTE*)pJob, sizeof(pJob->type) + DataSize);
	}
	else
	{
		// Service may run into this code path. Just add it directly.
		AppGetJobQueue()->AddJob(pJob, DataSize);
	}

	return TRUE;
}

BOOL uCharToCString(uChar* const pStrIn, INT iLen, CString &out)
{
#ifdef UCHAR_AS_UTF8

	if (iLen <= 0)
		iLen = ustrlen(pStrIn);
	if (iLen == 0)
		goto FAILED;

	LPTSTR ptr = out.GetBuffer(iLen + 1);

#ifdef UNICODE
	// CString is UNICODE string so we convert.
	INT newLen = MultiByteToWideChar(CP_UTF8, 0, pStrIn, iLen + 1, ptr, iLen + 1);
	if (!newLen)
	{
		out.ReleaseBuffer(0);
		goto FAILED;
	}
#else
	WCHAR* buf = (WCHAR*)malloc(utf8StrLen);

	if (buf == nullptr)
	{
		out.ReleaseBuffer(0);
		return false;
	}

	int newLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, utf8StrLen, buf, utf8StrLen);
	if(!newLen)
	{
		free(buf);
		out.ReleaseBuffer(0);
		return false;
	}

	ASSERT(newLen < utf8StrLen);
	newLen = WideCharToMultiByte(CP_ACP, 0, buf, newLen,  ptr, utf8StrLen);
	if(!newLen)
	{
		free(buf);
		out.ReleaseBuffer(0);
		return false;
	}

	free(buf);
#endif

	out.ReleaseBuffer(newLen);
	return TRUE;

#else

	ASSERT(sizeof(uChar) == 2);
	out = pStrIn;
	return TRUE;

#endif

FAILED:

	out.Empty();

	return FALSE;
}

INT CheckUTF8Length(const TCHAR *str, INT iStrLen)
{
#ifdef UCHAR_AS_UTF8
	ASSERT(iStrLen > 0);
	ASSERT(str[iStrLen - 1] != 0); // Make sure iStrLen doesn't include null-terminator.
	return WideCharToMultiByte(CP_UTF8, 0, str, iStrLen, NULL, 0, 0, 0);
#else
	return iStrLen;
#endif
}

void NotifyGUIMessage(DWORD type, stGUIEventMsg *pMsg) // Only call this two functions in job thread or cause data packing when sending data through the named pipe.
{
	CnMatrixCore *pnMatrixCore = AppGetnMatrixCore();

	if(pnMatrixCore->IsServiceMode())
	{
		if(pnMatrixCore->IsGUIReady())
		{
			BYTE buffer[1024 * 10], *pBuffer = buffer;
			UINT len = sizeof(type) + pMsg->WriteStream(0);
			if(len > sizeof(buffer))
				pBuffer = (BYTE*)malloc(len);
			CStreamBuffer sb;
			sb.AttachBuffer(pBuffer, len);
			sb << type;
			pMsg->WriteStream(&sb);

			ASSERT(len == sb.GetDataSize());
			pnMatrixCore->PipeSend(pBuffer, len);
			if(pBuffer != buffer)
				free(pBuffer);
		}
	}
	else
	{
		ASSERT(CnMatrixCore::hGUIHandle);
		PostMessage(CnMatrixCore::hGUIHandle, WM_GUI_EVENT, type, (LPARAM)pMsg);
	}
}

void NotifyGUIMessage(DWORD type, DWORD dwMsg)
{
	if(AppGetnMatrixCore()->IsServiceMode())
	{
		DWORD d2[2] = { type, dwMsg};

		if(AppGetnMatrixCore()->IsGUIReady())
			AppGetnMatrixCore()->PipeSend((BYTE*)d2, sizeof(d2));
	}
	else
	{
		ASSERT(CnMatrixCore::hGUIHandle);
		VERIFY(PostMessage(CnMatrixCore::hGUIHandle, WM_GUI_EVENT, type, (LPARAM)dwMsg));
	}
}


UINT stGUIVLanMember::ReadFromStream(CStreamBuffer &sb)
{
	// Must match stNetMember::ExportGUIDataStream() & stGUIVLanMember::WriteStream.

	BYTE len;
	UINT BytesReaded;
	TCHAR name[MAX_HOST_NAME_LENGTH + 1];

	sb.ReadString(len, _countof(name) - 1, name, sizeof(name));
	HostName = name;

	sb >> dwUserID >> vip >> LinkState;
	sb.Read(vmac, sizeof(vmac));
	sb >> DriverMapIndex >> eip >> eip.m_port >> Flag >> dwGroupBitMask >> GroupIndex;

	BytesReaded = sizeof(len) + sizeof(TCHAR) * len + sizeof(dwUserID) + sizeof(vip) + sizeof(LinkState) + sizeof(vmac) + sizeof(DriverMapIndex)
				+ eip.GetStreamingSize() + sizeof(eip.m_port) + sizeof(Flag) + sizeof(dwGroupBitMask) + sizeof(GroupIndex);

	return BytesReaded;
}

UINT stGUIVLanMember::WriteStream(CStreamBuffer *psb)
{
	UINT BytesWritten;
	BYTE len = HostName.GetLength();

	if(psb)
	{
		psb->WriteString(len, (const TCHAR*)HostName);
		*psb << dwUserID << vip << LinkState;
		psb->Write(vmac, sizeof(vmac));
		*psb << DriverMapIndex << eip << eip.m_port << Flag << dwGroupBitMask << GroupIndex;
	}

	BytesWritten = sizeof(len) + sizeof(TCHAR) * len + sizeof(dwUserID) + sizeof(vip) + sizeof(LinkState) + sizeof(vmac) + sizeof(DriverMapIndex)
				+ eip.GetStreamingSize() + sizeof(eip.m_port) + sizeof(Flag) + sizeof(dwGroupBitMask) + sizeof(GroupIndex);

	return BytesWritten;
}

UINT stGUIVLanInfo::ReadFromStream(CStreamBuffer &sb) // Match stVPNet::ExportGUIDataStream.
{
	USHORT len;
	UINT BytesReaded;
	TCHAR name[MAX_NET_NAME_LENGTH + 1];

	sb.ReadString(len, _countof(name) - 1, name, sizeof(name));
	NetName = name;

	sb >> NetIDCode >> NetworkType >> Flag >> dwGroupBitMask >> GroupIndex >> m_nTotalGroup;

	for(UINT i = 0; i < m_nTotalGroup; ++i)
	{
		sb.Read(&m_GroupArray[i].VNetGroup, sizeof(stVNetGroup));
		m_GroupArray[i].pVNetInfo = this;
	}

	BytesReaded = sizeof(len) + sizeof(TCHAR) * len + sizeof(NetIDCode) + sizeof(Flag) + sizeof(dwGroupBitMask)
				+ sizeof(GroupIndex) + sizeof(m_nTotalGroup) + m_nTotalGroup * sizeof(stVNetGroup);

	return BytesReaded;
}

void stGUIVLanInfo::UpdataToolTipString()
{
	ASSERT(nOnline <= MemberList.GetCount());
	CString csNetworkType = GUILoadString(IDS_TYPE) + csGColon, csNetworkID = GUILoadString(IDS_NETWORK_ID) + csGColon, csRelayHost, csTemp, csGroupName;

	switch(NetworkType)
	{
		case VNT_MESH:
			csNetworkType += _T("Mesh");
			break;
		case VNT_HUB_AND_SPOKE:
			csNetworkType += _T("Hub & Spoke - ");
			if(Flag & VF_HUB)
				csNetworkType += GUILoadString(IDS_HUB_CLIENT);
			else
				csNetworkType += GUILoadString(IDS_SPOKE_CLIENT);
			break;
		case VNT_GATEWAY:
			csNetworkType += _T("Gateway");
			break;

		//default:
		//	csNetworkType += _T("Undefined");
	}

	if(Flag & VF_RELAY_HOST)
	{
		csRelayHost.Format(_T(" (%s)"), GUILoadString(IDS_RELAY_HOST));
		csNetworkType += csRelayHost;
	}

	UINT nTotalMember = MemberList.GetCount();
	ToolTipInfo.Format(_T("%s%s   (%d / %d)\n\n"), csNetworkID, NetName, nOnline, nTotalMember);
	if(IsOwner())
		ToolTipInfo += (GUILoadString(IDS_NETWORK_ADM) + _T("\n"));

	ToolTipInfo += csNetworkType;

	if(m_nTotalGroup > 1)
	{
		uCharToCString(m_GroupArray[GroupIndex].VNetGroup.GroupName, -1, csGroupName);
		csTemp = _T("\n") + GUILoadString(IDS_SUBGROUP) + csGColon + csGroupName;
		ToolTipInfo += csTemp;
	}
}


UINT stGUIEventMsg::ReadFromStream(CStreamBuffer &sb)
{
	USHORT len;
	UINT BytesReaded;
	TCHAR name[MAX_NET_NAME_LENGTH + 1];

	DeleteHeap();

	sb >> UsageFlag;
	if(sb.ReadString(len, _countof(name) - 1, name, sizeof(name)))
		string = name;
	ASSERT(len <= MAX_NET_NAME_LENGTH);

	sb >> DWORD_1 >> DWORD_2 >> DWORD_3 >> DWORD_4 >> DWORD_5 >> HeapDataSize;
	if(HeapDataSize)
	{
		Heap(HeapDataSize);
		sb.Read(pHeapData, HeapDataSize);
	}

	BytesReaded = sizeof(UsageFlag) + sizeof(len) + len * sizeof(TCHAR) + sizeof(DWORD_1) * 5 + sizeof(HeapDataSize) + HeapDataSize;
	BytesReaded += member.ReadFromStream(sb);

	return BytesReaded;
}

UINT stGUIEventMsg::WriteStream(CStreamBuffer *psb)
{
	UINT BytesWritten;
	USHORT len = string.GetLength();
//	ASSERT(len <= MAX_NET_NAME_LENGTH); // Not correct since we use utf-8 as character type to communicate.

	if(psb)
	{
		*psb << UsageFlag;
		psb->WriteString(len, (const TCHAR*)string);

		*psb << DWORD_1 << DWORD_2 << DWORD_3 << DWORD_4 << DWORD_5 << HeapDataSize;

		if(HeapDataSize)
			psb->Write(pHeapData, HeapDataSize);
	}

	BytesWritten = sizeof(UsageFlag) + sizeof(len) + len * sizeof(TCHAR) + sizeof(DWORD_1) * 5 + sizeof(HeapDataSize) + HeapDataSize;
	BytesWritten += member.WriteStream(psb);

	return BytesWritten;
}


void AddJobDetectNatFirewall()
{
	stJob Job;
	Job.type = CT_DETECT_NAT_FIREWALL;
	AddJob(&Job, 0);
}

BOOL AddJobRegister(const TCHAR *pName, const TCHAR *pPassword)
{
	stJob job;
	job.type = CT_REG;
	DWORD dwFunc;
	stRegisterID RegID;

	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));

	if(!pName)
	{
		// Get a new host ID.
		dwFunc = 1;
		sBuffer << dwFunc;
		sBuffer.Write(&RegID, sizeof(RegID));
	}
	else
	{
		ASSERT(pName && pPassword);
		BYTE len = 0;
		dwFunc = 2;
		sBuffer << dwFunc;
		CoreWriteString(sBuffer, len, pName, -1);
		CoreWriteString(sBuffer, len, pPassword, -1);
	}

	AddJob(&job, sBuffer.GetDataSize());

	return TRUE;
}

BOOL AddJobLogin(UINT nLogoutReason, const TCHAR *pUserName, const TCHAR *pPassword, BOOL bCloseTunnel) // Todo: fix length check for utf-8.
{
	stJob Job;
	Job.type = CT_EVENT_LOGIN;

	USHORT idLen = 0, passwordLen = 0;
	if (!nLogoutReason)
	{
		if (pUserName != NULL)
		{
			idLen = _tcslen(pUserName);
			if (idLen > LOGIN_ID_LEN)
				return FALSE;
		}
		if (pPassword != NULL)
		{
			passwordLen = _tcslen(pPassword);
			if (passwordLen > LOGIN_PASSWORD_LEN)
				return FALSE;
		}
	}

	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(Job.Data, sizeof(Job.Data));
	sBuffer << nLogoutReason << bCloseTunnel;
	sBuffer.WriteString(idLen, pUserName); // Core worker thread will read these strings in TCHAR type.
	sBuffer.WriteString(passwordLen, pPassword);

	AddJob(&Job, sBuffer.GetDataSize());

	return TRUE;
}

void AddJobCreateJoinSubnet(const TCHAR *pNetName, const TCHAR *pPassword, BOOL bCreate, USHORT NetworkType)
{
	stJob job;
	BYTE byLen = 0;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));

	job.type = bCreate ? CT_CREATE_SUBNET : CT_JOIN_SUBNET;
	sBuffer << (DWORD) 0; // Reserved for server return code.

	CoreWriteString(sBuffer, byLen, pNetName, -1);
	CoreWriteString(sBuffer, byLen, pPassword, -1);

	if(bCreate)
		sBuffer << NetworkType;

	AddJob(&job, sBuffer.GetDataSize());
}

void PackCmdInfo(DWORD CmdType, DWORD NetID, DWORD UID)
{
	stJob job;
	job.type = CmdType;

	job.dword1 = 0; // Server return code.
	job.dword2 = NetID;
	job.dword3 = UID;

	AddJob(&job, sizeof(DWORD) * 3);
}

void AddJobExitSubnet(DWORD NetID, DWORD UID)
{
	PackCmdInfo(CT_EXIT_SUBNET, NetID, UID);
}

void AddJobDeleteSubNet(DWORD NetID, DWORD UID)
{
	PackCmdInfo(CT_DELETE_SUBNET, NetID, UID);
}

void AddJobRemoveUser(DWORD NetID, DWORD UserID)
{
	stJob job;
	job.type = CT_REMOVE_USER;

	job.dword1 = 0; // Server return code.
	job.dword2 = 0;
	job.dword3 = NetID;
	job.dword4 = UserID;

	AddJob(&job, sizeof(DWORD) * 4);
}

void AddJobOfflineSubNet(BOOL bOnline, DWORD UID, DWORD NetID)
{
	stJob job;
	job.type = CT_OFFLINE_SUBNET;

	job.dword1 = 0; // Server return code.
	job.dword2 = bOnline;
	job.dword3 = UID;
	job.dword4 = NetID;

	AddJob(&job, sizeof(DWORD) * 4);
}

void AddJobRetrieveNetList(DWORD NetIDCode, const TCHAR *pNetName)
{
	stJob job;
	job.type = CT_RETRIEVE_NET_LIST;
	job.dword1 = NetIDCode;

	AddJob(&job, sizeof(job.dword1));
}

BOOL AddJobTextChat(const TCHAR *pText, DWORD dwHostUID, DWORD dwPeerUID)
{
	if(pText == NULL || !dwPeerUID)
		return FALSE;

	INT len = _tcslen(pText);
	if(len > MAX_CHAT_STRING_LENGTH)
		len = MAX_CHAT_STRING_LENGTH;

	stJob job;
	CStreamBuffer sBuffer;
	job.type = CT_TEXT_CHAT | CT_RESPONSE;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));

	sBuffer << dwHostUID << dwPeerUID << len;
	CoreWriteString(sBuffer, pText, len);

	ASSERT(sBuffer.GetDataSize() <= sizeof(job.Data));
	AddJob(&job, sBuffer.GetDataSize());

	return TRUE;
}

void AddJobResetNetworkPassword(DWORD NetID, const TCHAR *NetName, const TCHAR *Password)
{
	BYTE byLen = 0;
	DWORD dwReserved = 0, dwType = JST_NET_PASSWORD | JST_SET_DATA;

	stJob job;
	job.type = CT_SETTING;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer << dwType << dwReserved << NetID;

	CoreWriteString(sBuffer, byLen, NetName, _tcslen(NetName));
	CoreWriteString(sBuffer, byLen, Password, _tcslen(Password));

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobResetNetworkFlag(DWORD NetID, const TCHAR *NetName, DWORD dwFlag, DWORD dwMask)
{
	DWORD dwReserved = 0, dwType = JST_NET_FLAG | JST_SET_DATA;

	stJob job;
	job.type = CT_SETTING;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer << dwType << dwReserved;

	sBuffer << NetID << dwFlag << dwMask;
	CoreWriteString(sBuffer, BYTE(0), NetName, -1);

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobSetHostRole(DWORD NetID, const TCHAR *NetName, DWORD UID, DWORD dwFlag, DWORD dwMask)
{
	DWORD dwReserved = 0, dwType = (dwMask == VF_HUB) ? JST_HOST_ROLE_HUB | JST_SET_DATA : JST_HOST_ROLE_RELAY | JST_SET_DATA;

	stJob job;
	job.type = CT_SETTING;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer << dwType << dwReserved;

	sBuffer << NetID << UID << dwFlag << dwMask;
	CoreWriteString(sBuffer, BYTE(0), NetName, -1);

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobQueryServerNews()
{
	stJob job;
	job.type = CT_CLIENT_QUERY;
	DWORD dwReserved = 0;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer << (DWORD)CQT_SERVER_NEWS << dwReserved;

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobQueryHostLastOnlineTime(DWORD NetID, DWORD UID)
{
	stJob job;
	job.type = CT_CLIENT_QUERY;
	DWORD dwReserved = 0;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer << (DWORD)CQT_HOST_LAST_ONLINE_TIME << dwReserved << NetID << UID;

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobQueryVNetMemberInfo(DWORD NetID)
{
	stJob job;
	job.type = CT_CLIENT_QUERY;
	job.dword1 = CQT_VNET_MEMBER_INFO;
	job.dword2 = 0;  // Reserved for server response code.
	job.dword3 = NetID;

	AddJob(&job, sizeof(job.dword1) * 3);
}

void AddJobReportBug(DWORD BugID, void *pRefData, UINT nDataSize)
{
	stJob job;
	job.type = CT_CLIENT_REPORT;
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));

	sBuffer << (DWORD)CRT_BUG << BugID;
	if(pRefData)
		sBuffer.Write(pRefData, nDataSize);

	AddJob(&job, sBuffer.GetDataSize());
}

void AddJobQueryServerTime(BOOL bTest)
{
	stJob job;
	job.type = CT_SERVER_TIME;
	job.dword1 = bTest;
	AddJob(&job, sizeof(job.dword1));
}

void AddJobRequestRelay(DWORD dwRequestType, DWORD NetID, DWORD DestUID, DWORD RelayHostUID)
{
	stJob job;
	job.type = CT_REQUEST_RELAY;
	job.dword1 = dwRequestType;
	job.dword2 = NetID;
	job.dword3 = 0; // This must be null.
	job.dword4 = DestUID;
	job.dword5 = RelayHostUID;

	AddJob(&job, sizeof(job.dword1) + sizeof(job.dword2) + sizeof(job.dword3) + sizeof(job.dword4) + sizeof(job.dword5));
}

void AddJobServerRelay(DWORD uid, void *pData, UINT nDataLen) // For core only.
{
	CJobQueue *pJobQueue = AppGetJobQueue();
	stJob *pJob = pJobQueue->AllocateJob();

	if(pJob == NULL)
		return;

	pJob->type = CT_SERVER_RELAY;
	pJob->dword1 = uid;
	memcpy(&pJob->dword2, pData, nDataLen);

	pJob->DataLength = sizeof(pJob->type) + sizeof(uid) + nDataLen;
	ASSERT(pJob->DataLength <= MAX_CMD_PACKET_SIZE);

	pJobQueue->AddJob(pJob, sizeof(uid) + nDataLen, FALSE, TRUE);
}

void AddJobSystemPowerEvent(BOOL bResume)
{
	stJob job;
	job.type = CT_SYSTEM_POWER_EVENT;
	job.dword1 = bResume;
	AddJob(&job, sizeof(job.dword1));
}

void AddJobReadTraffic()
{
	stJob job;
	job.type = CT_READ_TRAFFIC_INFO;
	AddJob(&job, 0);
}

UINT AddJobDataExchange(BOOL bRead, DWORD DataTypeEnum, void *pWriteValue, BOOL bPackDataOnly, void *pBuffer, UINT nBufferSize)
{
	stJob job;
	job.type = CT_DATA_EXCHANGE;

	job.dword1 = bRead;
	job.dword2 = DataTypeEnum;
	UINT unDataSize = sizeof(job.dword1) + sizeof(job.dword2);

	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));
	sBuffer.SetPos(unDataSize);

	if(!bRead)
	{
		switch(DataTypeEnum)
		{
			case DET_LOGIN_INFO:
				break;

			case DET_REG_ID:
				sBuffer.Write(pWriteValue, sizeof(stRegisterID));
				break;

			case DET_SERVER_MSG_ID:
				sBuffer << *(UINT*)pWriteValue;
				break;
		}

		unDataSize = sBuffer.GetDataSize();
	}

	if(bPackDataOnly)
	{
		ASSERT(sizeof(job.type) + unDataSize <= nBufferSize);
		memcpy(pBuffer, &job, sizeof(job.type) + unDataSize);
	}
	else
		AddJob(&job, unDataSize);

	return sizeof(job.type) + unDataSize;
}

void AddJobEnableUMAccess(BOOL bEnable, BOOL bUserModeBuffer)
{
	stJob job;
	job.type = CT_ENABLE_UM_READ;
	job.dword1 = bEnable;
	AddJob(&job, sizeof(job.dword1));
}

UINT AddJobUpdateConfig(stConfigData *pConfigData, BOOL bRead, BOOL bPackDataOnly, void *pBuffer, UINT nBufferSize)
{
	stJob job;
	UINT nSize = sizeof(job.type) + sizeof(job.dword1);

	job.type = CT_CONFIG;
	job.dword1 = bRead;
	if(bRead)
		AddJob(&job, sizeof(job.dword1));
	else
	{
		nSize += sizeof(*pConfigData);
		memcpy(&job.dword2, pConfigData, sizeof(*pConfigData));
		if(bPackDataOnly)
		{
			ASSERT(nBufferSize >= nSize);
			memcpy(pBuffer, &job, nSize);
		}
		else
			AddJob(&job, sizeof(job.dword1) + sizeof(*pConfigData));
	}

	return nSize;
}

void AddJobUpdateHostName(DWORD UID, const TCHAR *pNewName, UINT nLen)
{
	ASSERT(nLen <= MAX_HOST_NAME_LENGTH);

	stJob job;
	job.type = CT_CLIENT_PROFILE;
	job.dword1 = CPT_HOST_NAME;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = UID;

	CStreamBuffer sb;
	sb.AttachBuffer(job.Data, sizeof(job.Data));
	sb.Skip(sizeof(DWORD) * 3);
	CoreWriteString(sb, pNewName, nLen);

	AddJob(&job, sb.GetDataSize());
}

void AddJobPingHost(DWORD uid, UINT nID)
{
	stJob job;
	job.type = CT_PING_HOST;
	job.dword1 = uid;
	job.dword2 = nID;

	AddJob(&job, sizeof(job.dword1) + sizeof(job.dword2));
}

void AddJobGUIPipeEvent(BOOL bAttach)
{
	stJob job;
	job.type = CT_GUI_PIPE_EVENT;
	job.dword1 = bAttach;
	AddJob(&job, sizeof(job.dword1));
}

void AddJobCloseMsg(DWORD dwMsgID)
{
	stJob job;
	job.type = CT_SETTING;
	job.dword1 = JST_CLOSED_SERVER_MSG_ID;
	job.dword2 = dwMsgID;

	AddJob(&job, sizeof(job.dword1) + sizeof(job.dword2));
}

void AddJobServiceState(BOOL bQueryVersion)
{
	stJob job;
	job.type = CT_SERVICE_STATE;
	job.dword1 = bQueryVersion;
	AddJob(&job, sizeof(job.dword1));
}

void AddJobDebugFunction(DWORD type, DWORD value1, DWORD value2)
{
	stJob job;
	job.type = CT_DEBUG_FUNCTION;
	job.dword1 = type;
	job.dword2 = value1;
	job.dword3 = value2;

	AddJob(&job, sizeof(job.dword1) + sizeof(job.dword2) + sizeof(job.dword3));
}

void AddJobGroupChatSetup(DWORD HostUID, DWORD VNetID, UINT nUserCount, DWORD *UIDArray)
{
	stJob job;

	if(nUserCount > MAX_GROUP_CHAT_HOST)
		nUserCount = MAX_GROUP_CHAT_HOST;

	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_REQUEST;
	job.dword2 = 0;               // Reserved for server return code.
	job.dword3 = 0;               // Session ID returned by server.
	job.dword4 = VNetID;
	job.dword5 = HostUID;         // Requester ID.
	job.dword6 = nUserCount + 1;  // Chat member count.
	job.dword7 = HostUID;         // First member must be the owner.

	UINT DataLength = sizeof(DWORD) * nUserCount;
	memcpy(&job.dword8, UIDArray, DataLength);

	DataLength += sizeof(job.dword1) * 7;
	ASSERT(DataLength <= sizeof(job.Data));

	AddJob(&job, DataLength);
}

void AddJobGroupChatInvite(DWORD VNetID, UINT nSessionID, UINT nUserCount, DWORD *UIDArray)
{
	stJob job;
	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_INVITE;
	job.dword2 = 0;           // Reserved for server return code.
	job.dword3 = nSessionID;
	job.dword4 = VNetID;
	job.dword5 = 0;
	job.dword6 = nUserCount;

	UINT DataLength = sizeof(DWORD) * nUserCount;
	memcpy(&job.dword7, UIDArray, DataLength);

	DataLength += sizeof(job.dword1) * 6;
	ASSERT(DataLength <= sizeof(job.Data));

	AddJob(&job, DataLength);
}

void AddJobGroupChatLeave(DWORD HostUID, UINT nSessionID)
{
	stJob job;
	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_LEAVE;
	job.dword2 = 0;           // Reserved for server return code.
	job.dword3 = nSessionID;
	job.dword4 = HostUID;

	AddJob(&job, sizeof(job.dword1) * 4);
}

void AddJobGroupChatClose(DWORD HostUID, UINT nSessionID)
{
	stJob job;
	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_CLOSE;
	job.dword2 = 0;           // Reserved for server return code.
	job.dword3 = nSessionID;
	job.dword4 = HostUID;

	AddJob(&job, sizeof(job.dword1) * 4);
}

void AddJobGroupChatEvict(DWORD DestUID, UINT nSessionID)
{
	stJob job;
	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_EVICT;
	job.dword2 = 0;           // Reserved for server return code.
	job.dword3 = nSessionID;
	job.dword4 = DestUID;

	AddJob(&job, sizeof(job.dword1) * 4);
}

BOOL AddJobGroupChatSend(DWORD HostUID, UINT nSessionID, const TCHAR *pString, UINT nLength)
{
	if(nLength > MAX_GROUP_CHAT_LENGTH)
		nLength = MAX_GROUP_CHAT_LENGTH;

	stJob job;
	CStreamBuffer sb;
	sb.AttachBuffer(job.Data, sizeof(job.Data));

	job.type = CT_GROUP_CHAT;
	job.dword1 = GCRT_CHAT;
	job.dword2 = 0;           // Reserved for server return code.
	job.dword3 = nSessionID;
	job.dword4 = HostUID;
	job.dword5 = nLength;

	sb.Skip(sizeof(DWORD) * 5);
	INT iUtf8Len = CoreWriteString(sb, pString, nLength);
	if(iUtf8Len > MAX_GROUP_CHAT_LENGTH * 3)
		return FALSE;

	ASSERT(sb.GetDataSize() <= sizeof(job.Data));
	AddJob(&job, sb.GetDataSize());

	return TRUE;
}

void AddJobCreateGroup(DWORD VNetID, const TCHAR *pNameString, UINT nLength)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_CREATE;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = 0;      // Group index.
	job.dword5 = nLength;

	memcpy(&job.dword6, pNameString, sizeof(TCHAR) * nLength);
	AddJob(&job, sizeof(job.dword1) * 5 + sizeof(TCHAR) * nLength);
}

void AddJobDeleteGroup(DWORD VNetID, UINT nIndex)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_DELETE;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = nIndex; // Group index.
	job.dword5 = 0;

	ASSERT(nIndex < MAX_VNET_GROUP_COUNT);
	AddJob(&job, sizeof(job.dword1) * 5);
}

void AddJobMoveMember(DWORD VNetID, DWORD UserID, UINT nNewIndex)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_MOVE;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = UserID; // Group index.
	job.dword5 = nNewIndex;

	ASSERT(nNewIndex < MAX_VNET_GROUP_COUNT);
	AddJob(&job, sizeof(job.dword1) * 5);
}

void AddJobSetGroupFlag(DWORD VNetID, UINT nGroupIndex, DWORD dwMask, DWORD dwFlag)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_SET_FLAG;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = nGroupIndex; // Group index.
	job.dword5 = dwMask;
	job.dword6 = dwFlag;

	ASSERT(nGroupIndex < MAX_VNET_GROUP_COUNT);
	AddJob(&job, sizeof(job.dword1) * 6);
}

void AddJobRenameGroup(DWORD VNetID, UINT nGroupIndex, const TCHAR *pNewName, UINT nLength)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_RENAME;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = nGroupIndex; // Group index.
	job.dword5 = nLength;

	memcpy(&job.dword6, pNewName, sizeof(TCHAR) * nLength);
	AddJob(&job, sizeof(job.dword1) * 5 + sizeof(TCHAR) * nLength);
}

void AddJobSubgroupOffline(DWORD UID, DWORD VNetID, UINT nGroupIndex, BOOL bOnline)
{
	stJob job;
	job.type = CT_SUBNET_SUBGROUP;
	job.dword1 = SSCT_OFFLINE;
	job.dword2 = 0;      // Reserved for server return code.
	job.dword3 = VNetID;
	job.dword4 = UID;
	job.dword5 = nGroupIndex;
	job.dword6 = bOnline;

	AddJob(&job, sizeof(job.dword1) * 6);
}

void AddJobDebugTest()
{
	stJob job;
	job.type = CT_DEBUG_TEST;
	job.dword1 = SSCT_OFFLINE;

	AddJob(&job, sizeof(job.dword1) * 1);
}

void CMessagePipe::OnConnectingPipe()
{
	//printx("---> CMessagePipe::OnConnectingPipe.\n");
	m_bConnected = TRUE;
	if(AppGetnMatrixCore()->IsServiceMode())
		AddJobGUIPipeEvent(TRUE);
}

void CMessagePipe::OnDisConnectingPipe(CPipeListener* pReader)
{
	//printx("---> CMessagePipe::OnDisConnectingPipe.\n");
	m_bConnected = FALSE;
	if(AppGetnMatrixCore()->IsServiceMode())
		AddJobGUIPipeEvent(FALSE);
}

void CMessagePipe::OnIncomingData(const BYTE* pData, DWORD nSize)
{
	printx("OnIncomingData: %d Bytes.\n", nSize);

	if(AppGetnMatrixCore()->IsServiceMode())
	{
		UINT nTryCount = 10;
		while(!AppGetnMatrixCore()->IsRunning() && nTryCount--)
			Sleep(100);

		AddJob((stJob*)pData, nSize - sizeof(((stJob*)pData)->type));
	}
	else
	{
		CStreamBuffer sb;
		UINT nRead;
		DWORD type, param;

		sb.AttachBuffer((BYTE*)pData, nSize);
		if(nSize == sizeof(DWORD) * 2)
		{
			sb >> type >> param;
			NotifyGUIMessage(type, param);
		}
		else
		{
			sb >> type;
			stGUIEventMsg *pMsg = AllocGUIEventMsg();
			nRead = pMsg->ReadFromStream(sb);
			ASSERT(nRead == nSize - sizeof(type));
			NotifyGUIMessage(type, pMsg);
		}
	}
}


BOOL CnMatrixCore::Init(HWND hWnd, CMessagePipe *pPipe, BOOL bForceGUIMode)
{
	BOOL bResult = TRUE;

	hGUIHandle = hWnd;
	if(hWnd == NULL) // Service mode.
	{
		m_flags = CF_SERVICE_MODE;
		ASSERT(pPipe == NULL && m_pPipe == NULL);
		m_pPipe = new CMessagePipe;
		m_pPipe->GetPipeObject()->CreateNamedPipe(APP_STRING(ASI_PIPE_NAME));
//		if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) // THREAD_PRIORITY_ABOVE_NORMAL THREAD_PRIORITY_HIGHEST.
//			printx("Error! SetThreadPriority failed! ec: %d\n", GetLastError());
	}
	else if(pPipe != NULL)
	{
		ASSERT(m_pPipe == NULL);
		m_pPipe = pPipe;
		m_pPipe->GetPipeObject()->StartReader(TRUE);
		m_flags = CF_PURE_GUI;
		AddJobServiceState();
	}
	else
	{
		m_pPipe = new CMessagePipe;

		if(bForceGUIMode)
		{
			m_flags = CF_PURE_GUI;
			bResult = FALSE;
			INT iWait = 300; // Wait 5 minutes.
			while(iWait--)
				if(!m_pPipe->GetPipeObject()->Connect(APP_STRING(ASI_PIPE_NAME)))
					Sleep(1000);
				else
				{
					m_pPipe->GetPipeObject()->StartReader(TRUE);
					AddJobServiceState(TRUE);
					bResult = TRUE;
					break;
				}
		}
		else if(m_pPipe->GetPipeObject()->Connect(APP_STRING(ASI_PIPE_NAME)))
		{
			m_flags = CF_PURE_GUI;
			m_pPipe->GetPipeObject()->StartReader(TRUE);
			AddJobServiceState(TRUE);
		}
		else
		{
			SAFE_DELETE(m_pPipe);
			// Create only one worker thread.
			ASSERT(m_hWorkerThread == NULL);
			m_hWorkerThread = CreateThread(NULL, 0, WorkerThread, this, 0, NULL);

			if(m_hWorkerThread != NULL)
				if(!SetThreadPriority(m_hWorkerThread, THREAD_PRIORITY_HIGHEST)) // THREAD_PRIORITY_ABOVE_NORMAL THREAD_PRIORITY_HIGHEST.
					printx("Error! SetThreadPriority failed! ec: %d\n", GetLastError());

			// Change priority may cause delay when use kernel mode socket to punch through.
			// Os: Win XP running in VMWare. The function printx uses much cpu time and thread priority is higher than kernel system thread.
		}
	}

	return bResult;
}

BOOL CnMatrixCore::Close()
{
	if(IsServiceMode())
	{
		NotifyGUIMessage(GET_SERVICE_STATE, 0x12345678);
		m_pPipe->GetPipeObject()->Close(TRUE);
		SAFE_DELETE(m_pPipe);

		m_flags &= ~(CF_SERVICE_MODE | CF_RUNNING | CF_GUI_READY); // Clean flag after closing pipe.
		AppGetJobQueue()->Stop();
	}
	else if(IsPureGUIMode())
	{
		m_flags &= ~(CF_PURE_GUI | CF_RUNNING);
		if(m_pPipe)
			m_pPipe->GetPipeObject()->Close(TRUE);
		SAFE_DELETE(m_pPipe);
	}
	else if(IsRunning())
	{
		m_flags &= ~CF_RUNNING;
		AppGetJobQueue()->Stop();
		ASSERT(m_hWorkerThread);
		DWORD dwResult = WaitForSingleObject(m_hWorkerThread, INFINITE);
	//	if(dwResult != WAIT_OBJECT_0)
	//		printx("Error occurred when wait worker thread object!\n");
		SAFE_CLOSE_HANDLE(m_hWorkerThread);
	}

	ASSERT(m_flags == 0);

	return TRUE;
}


void OnTCPSocketReceive(BYTE *pData, UINT size, CSocketTCP *pSocket) // Executed in socket thread.
{
//	printx("On receiving data: %d bytes.\n", size);

	if(((stJob*)pData)->type == (CT_SERVER_RELAY | CT_RESPONSE)) // Sending data to local address and let recv thread deal with it for bandwidth control.
	{
		JobServerRelay((stJob*)pData, pSocket, size);
		return;
	}

	AddJob((stJob*)pData, size - sizeof ((stJob*)pData)->type);
}

void OnTcpDisconnect(DWORD dwError, void *pContext) // Executed in socket thread.
{
	printx("Enter OnTcpDisconnect(). Error code: %d.\n", dwError);

	BOOL bCloseTunnel = TRUE;
	stClientInfo *pInfo = AppGetClientInfo();

	if(dwError == WSAECONNRESET && pInfo->ConfigData.KeepTunnelTime)
	{
		bCloseTunnel = FALSE;
	}

	// Test if internal address is still valid. Maybe we can check pppoe connection in advance.
	if(!CNetworkManager::IsValidIpAddress(AppGetClientInfo()->ClientInternalIP.v4))
		bCloseTunnel = TRUE;

	AddJobLogin(LOR_DISCONNECTED, 0, 0, bCloseTunnel);
}

void CoreTimerTick(stClientInfo *pClientInfo, UINT &SyncTimerTick)
{
	DWORD dwAppState = AppGetState();
	INT64 ServerTime = (INT64)pClientInfo->GetServerTime();
	INT ReservedTime = 1000 - ServerTime % 1000;

	if(ReservedTime < MIN_TIMER_RESERVED_TIME || ReservedTime > MAX_TIMER_RESERVED_TIME + 20)
		SetupWaitableTimer();

	if(GetKeyState(VK_F2) < 0)
		printx("Test Server Time: %d. Reserved Time: %d.\n", (INT)ServerTime, ReservedTime);

	if(dwAppState == AS_READY && pClientInfo->SyncTime)
	{
		if(SyncTimerTick)
			SyncTimerTick--;
		if(!SyncTimerTick)
		{
			printx("Start sync op! Time: %f\n", (DOUBLE)ServerTime);
			AddJobQueryServerTime(FALSE);
		}
	}

	if(dwAppState == AS_QUERY_ADDRESS)
		JobQueryExternelAddress(0);
}


void TimerCB(void *pCtx)
{
//	printx("---> TimerCB: %u, %u\n", (UINT)pCtx, timeGetTime());
	SetEvent(pCtx);
}

void SetupWaitableTimer()
{
	// Windows periodic timer doesn't match absolutely time.
	// It will generate a large bias after a long time.

	stClientInfo *pClientInfo = AppGetClientInfo();
	INT64 ServerTime = (INT64)pClientInfo->GetServerTime();
	INT SleepTime = 1000 - ServerTime % 1000 - MAX_TIMER_RESERVED_TIME; // Reserved some time so we can prepare to work more precisely.
	if(SleepTime < 0)
		SleepTime += 1000;

	AppGetnMatrixCore()->CoreStartTimer(SleepTime);
	printx("---> SetupWaitableTimer. Server Time: %d.\n", (INT)ServerTime);
}

BOOL CnMatrixCore::CoreCreateTimer(HANDLE *pWaitEvent)
{
	ASSERT(m_hTimer == NULL && m_hTimerEvent == NULL);

#ifdef USE_PLATFORM_TIMER
	m_hTimer = m_hTimerEvent = CreateWaitableTimer(0, FALSE, 0);
#else
	m_hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hTimer = CEventManager::GetEventManager()->CreateTimer();
#endif

	*pWaitEvent = m_hTimerEvent;

	return (m_hTimer != NULL);
}

void CnMatrixCore::CoreCloseTimer()
{
#ifdef USE_PLATFORM_TIMER
	//VERIFY(CancelWaitableTimer(m_hTimer));
	VERIFY(CloseHandle(m_hTimer));
	m_hTimer = m_hTimerEvent = NULL;
#else

	if(m_hTimer != NULL)
	{
		CEventManager::GetEventManager()->DeleteTimer(m_hTimer);
		m_hTimer = NULL;
	}
	if(m_hTimerEvent != NULL)
	{
		CloseHandle(m_hTimerEvent);
		m_hTimerEvent = NULL;
	}

#endif
}

void CnMatrixCore::CoreStartTimer(INT iFirstTime)
{
#ifdef USE_PLATFORM_TIMER
	LARGE_INTEGER li;
	li.QuadPart = -iFirstTime * 10000;
	VERIFY(SetWaitableTimer(m_hTimer, &li, DEFAULT_TIMER_PERIOD, 0, 0, FALSE));
#else
	Sleep(iFirstTime);
	CEventManager::GetEventManager()->StartTimer(m_hTimer, DEFAULT_TIMER_PERIOD, TimerCB, m_hTimerEvent, FALSE);
#endif
}

void CnMatrixCore::CoreStopTimer()
{
#ifdef USE_PLATFORM_TIMER
	VERIFY(CancelWaitableTimer(m_hTimer));
#else
	CEventManager::GetEventManager()->StopTimer(m_hTimer);
#endif
}

void CnMatrixCore::Run()
{
	stJob *pJob;
	CJobQueue JobQueue;
	stClientInfo ClientInfo;
	CNetworkManager NetManager;
	CSocketTCP SocketSP;
//	CSocketManager SocketManager;
	CEventManager EventManager;
	VERIFY(EventManager.Init(FALSE));
	SocketSP.SetCallback(OnTcpDisconnect, (SocketCallback)OnTCPSocketReceive);

	DWORD dwResult, dwEventCount = CEI_IPC_SOCKET_EVENT_INDEX;
	HANDLE hEvents[CEI_TOTAL_EVENT] = {0};

	hEvents[CEI_JOB_QUEUE_EVENT_INDEX] = CreateEvent(0, FALSE, FALSE, 0);     // Job  queue event.
	CoreCreateTimer(&hEvents[CEI_CORE_TIMER_EVENT_INDEX]);                    // Timer for setup and keeping udp tunnel.
	hEvents[CEI_SERVER_SOCKET_EVENT_INDEX] = CreateEvent(0, FALSE, FALSE, 0); // Server socket event.

	JobQueue.SetEventHandle(hEvents[CEI_JOB_QUEUE_EVENT_INDEX]);
	EnableServerSocketEvent(0, 0, hEvents[CEI_SERVER_SOCKET_EVENT_INDEX]);
	NetManager.SetEventHandle(hEvents, &dwEventCount);
	m_flags |= CF_RUNNING;

	UINT SyncTimerTick = 0;
	AppSetState(AS_DEFAULT);
	CoreReadConfigData(&ClientInfo.ConfigData);
	JobUpdateConfig(&ClientInfo, &NetManager, 0);
	NotifyGUIMessage(GET_READY, (DWORD)0);


	HANDLE hTimer50, hTimer100, hTimer150, hTimer200, hTimer350, hTimer400;
	CPerformanceCounter pc;
	BOOL bOnce = TRUE;

//	HANDLE hTimerInit = CEventManager::GetEventManager()->StartTimerEx(10000000, TimerCB, (void*)350, FALSE);

	pc.Start();

/*	hTimer350 = CEventManager::GetEventManager()->CreateTimer();
	hTimer100 = CEventManager::GetEventManager()->CreateTimer();
	hTimer400 = CEventManager::GetEventManager()->CreateTimer();
	hTimer200 = CEventManager::GetEventManager()->CreateTimer();
	hTimer50  = CEventManager::GetEventManager()->CreateTimer();
	hTimer150 = CEventManager::GetEventManager()->CreateTimer();

	CEventManager::GetEventManager()->StartTimer(hTimer350, 350, TimerCB, (void*)350, bOnce);
	CEventManager::GetEventManager()->StartTimer(hTimer100, 100, TimerCB, (void*)100, bOnce);
	CEventManager::GetEventManager()->StartTimer(hTimer400, 400, TimerCB, (void*)400, bOnce);
	CEventManager::GetEventManager()->StartTimer(hTimer200, 200, TimerCB, (void*)200, bOnce);
	CEventManager::GetEventManager()->StartTimer(hTimer50, 50, TimerCB, (void*)50, bOnce);
	CEventManager::GetEventManager()->StartTimer(hTimer150, 150, TimerCB, (void*)150, bOnce); //*/

/*	hTimer350 = CEventManager::GetEventManager()->StartTimerEx(350, TimerCB, (void*)350, bOnce);
	hTimer100 = CEventManager::GetEventManager()->StartTimerEx(100, TimerCB, (void*)100, bOnce);
	hTimer400 = CEventManager::GetEventManager()->StartTimerEx(400, TimerCB, (void*)400, bOnce);
	hTimer200 = CEventManager::GetEventManager()->StartTimerEx(200, TimerCB, (void*)200, bOnce);
	hTimer50 = CEventManager::GetEventManager()->StartTimerEx(50, TimerCB, (void*)50, bOnce);
	hTimer150 = CEventManager::GetEventManager()->StartTimerEx(150, TimerCB, (void*)150, bOnce); //*/

	pc.End();
	printx("Setup timer time: %lf ms\n", pc.Get());

	LARGE_INTEGER f = CPerformanceCounter::GetFreq();
	printx("Setup timer time: %lf ms. (%llu)\n", pc.Get(f), f.QuadPart);

/*
	CEventManager::GetEventManager()->StopTimer(hTimer50);
	CEventManager::GetEventManager()->StopTimer(hTimer100);
	CEventManager::GetEventManager()->StopTimer(hTimer150);
	CEventManager::GetEventManager()->StopTimer(hTimer200);
	CEventManager::GetEventManager()->StopTimer(hTimer350);
	CEventManager::GetEventManager()->StopTimer(hTimer400);
//*/

	if (IsServiceMode())
	{
		EnableServicePipe();
		if (CoreLoadSvcLoginState())
			AddJobLogin(0, nullptr, nullptr, FALSE);
	}

	while(1)
	{
		pJob = JobQueue.GetJob();

		if(!pJob)
		{
			if(!IsRunning())
				break;

			dwResult = WaitForMultipleObjects(dwEventCount, hEvents, FALSE, INFINITE);

			switch(dwResult)
			{
				case WAIT_OBJECT_0 + CEI_CORE_TIMER_EVENT_INDEX: // Timer event.
					CoreTimerTick(&ClientInfo, SyncTimerTick); // Don't do work that need large cpu time in this function.
					if(NetManager.TickOnce())
						JobClientTimer(&ClientInfo, &NetManager, 0, &SocketSP, SyncTimerTick);
					break;

				case WAIT_OBJECT_0 + CEI_SERVER_SOCKET_EVENT_INDEX:
					WSANETWORKEVENTS NetworkEvents;
					WSAEnumNetworkEvents(SocketSP.GetSocket(), hEvents[CEI_SERVER_SOCKET_EVENT_INDEX], &NetworkEvents);

					if(NetworkEvents.lNetworkEvents & FD_CLOSE)
						SocketSP.OnClose(NetworkEvents.iErrorCode[FD_CLOSE_BIT], 0);
					else
						SocketSP.OnReceive();
					break;

				case WAIT_OBJECT_0 + CEI_IPC_SOCKET_EVENT_INDEX: // IPC socket event.
					JobKernelDriverEvent(&NetManager, &SocketSP);
				//	WSAResetEvent(hEvents[IPC_SOCKET_EVENT_INDEX]);
					break;
			}

			continue;
		}

		if(AppGetState() != AS_READY)
		{
			switch(GET_TYPE(pJob->type))
			{
				case CT_REG:
					JobRegister(pJob, &SocketSP);
					break;

				case CT_LOGIN:
					JobLogin(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_DETECT_NAT_FIREWALL:
					JobDetectNatFirewallType(pJob);
					break;

				case CT_QUERY_EXTERNEL_ADDRESS:
					JobQueryExternelAddress(pJob);
					break;

				case CT_RETRIEVE_NET_LIST:
					JobRetrieveNetList(pJob, &SocketSP);
					break;

				case CT_SETTING:
					JobSetting(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_CLIENT_QUERY:
					JobClientQuery(&ClientInfo, pJob, &SocketSP);
					break;

				case CT_SERVER_TIME:
					JobQueryServerTime(&ClientInfo, pJob, &SocketSP, SyncTimerTick);
					break;

				case CT_SERVICE_STATE:
					JobServiceState(&ClientInfo, pJob);
					break;

				case CT_SYSTEM_POWER_EVENT:
					JobSystemPowerEvent(&ClientInfo, pJob);
					break;

				case CT_SOCKET_CONNECT_RESULT:
					JobSocketConnectResult(&ClientInfo, pJob, &SocketSP);
					break;

				case CT_DATA_EXCHANGE:
					JobDataExchange(pJob);
					break;

				case CT_READ_TRAFFIC_INFO: // Code to avoid error message.
					break;

				case CT_EVENT_LOGIN:
					JobLoginEvent(pJob, &SocketSP);
					break;

				case CT_ENABLE_UM_READ:
					JobEnableUserModeAccess(&NetManager, pJob->dword1);
					break;

				case CT_CONFIG:
					JobUpdateConfig(&ClientInfo, &NetManager, pJob);
					break;

				case CT_GUI_PIPE_EVENT:
					JobGUIPipeEvent(pJob);
					break;

				default:
					printx("Error! Not ready for job type: 0x%08x.\n", GET_TYPE(pJob->type));
			}
		}
		else
		{
			switch(GET_TYPE(pJob->type))
			{
				case CT_REG:
					JobRegister(pJob, &SocketSP);
					break;

				case CT_RETRIEVE_NET_LIST:
					JobRetrieveNetList(pJob, &SocketSP);
					break;

				case CT_CREATE_SUBNET:
					JobCreateSubnet(pJob, &SocketSP);
					break;

				case CT_JOIN_SUBNET:
					JobJoinSubnet(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_EXIT_SUBNET:
					JobExitSubnet(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_REMOVE_USER:
					JobRemoveUser(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_OFFLINE_SUBNET:
					JobOfflineSubnet(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_DELETE_SUBNET:
					JobDeleteSubnet(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_TEXT_CHAT:
					JobTextChat(pJob, &SocketSP);
					break;

				case CT_SERVER_NOTIFY:
					JobServerNotify(pJob, &SocketSP);
					break;

				case CT_HOST_SEND:
					JobHostSend(pJob);
					break;

				case CT_SETTING:
					JobSetting(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_CLIENT_QUERY:
					JobClientQuery(&ClientInfo, pJob, &SocketSP);
					break;

				case CT_CLIENT_REPORT:
					JobClientReport(pJob, &SocketSP);
					break;

				case CT_SERVER_TIME:
					JobQueryServerTime(&ClientInfo, pJob, &SocketSP, SyncTimerTick);
					break;

				case CT_SERVER_QUERY:
					JobServerQuery(pJob, &SocketSP);
					break;

				case CT_SERVER_REQUEST:
					JobServerRequest(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_REQUEST_RELAY:
					JobRequestRelay(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_SERVER_RELAY:
					JobServerRelay(pJob, &SocketSP);
					break;

				case CT_DEBUG_FUNCTION:
					JobDebugFunction(pJob, &SocketSP);
					break;

				case CT_GROUP_CHAT:
					JobGroupChat(pJob, &SocketSP);
					break;

				case CT_SUBNET_SUBGROUP:
					JobSubnetSubgroup(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_CLIENT_PROFILE:
					JobClientProfile(pJob, &SocketSP);
					break;

				case CT_SERVICE_STATE:
					JobServiceState(&ClientInfo, pJob);
					break;

				case CT_SYSTEM_POWER_EVENT:
					JobSystemPowerEvent(&ClientInfo, pJob);
					break;

				case CT_DATA_EXCHANGE:
					JobDataExchange(pJob);
					break;

				case CT_READ_TRAFFIC_INFO:
					JobReadTrafficInfo(pJob);
					break;

				case CT_EVENT_LOGIN: // Logout.
					JobLoginEvent(pJob, &SocketSP);
					break;

				case CT_ENABLE_UM_READ:
					JobEnableUserModeAccess(&NetManager, pJob->dword1);
					break;

				case CT_CONFIG:
					JobUpdateConfig(&ClientInfo, &NetManager, pJob);
					break;

				case CT_PING_HOST:
					JobPingHost(&ClientInfo, &NetManager, pJob, &SocketSP);
					break;

				case CT_GUI_PIPE_EVENT:
					JobGUIPipeEvent(pJob);
					break;

				case CT_DEBUG_TEST:
					JobDebugTest(pJob, &SocketSP);
					break;

				default:
					printx("Error! Not ready for job type: 0x%08x.\n", GET_TYPE(pJob->type));
			}
		}

		JobQueue.DeleteJob(pJob);
	}

	EventManager.RemoveSocket(SocketSP.GetSocket(), TRUE);
	SocketSP.ShutDown();
	SocketSP.CloseSocket();

	CoreCloseTimer();
	EnableServerSocketEvent(0, 0, hEvents[CEI_SERVER_SOCKET_EVENT_INDEX]);
	SAFE_CLOSE_HANDLE(hEvents[CEI_JOB_QUEUE_EVENT_INDEX]);
	SAFE_CLOSE_HANDLE(hEvents[CEI_SERVER_SOCKET_EVENT_INDEX]);
	EventManager.Close();
//	SocketManager.WaitForClose();

	HANDLE hAdapter = OpenAdapter();
	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		NetManager.CloseTunnelSocket(hAdapter);
		CloseAdapter(hAdapter);
	}
}

DWORD WINAPI CnMatrixCore::WorkerThread(LPVOID lpParameter)
{
	CnMatrixCore *pnMatrixCore = (CnMatrixCore*)lpParameter;
	pnMatrixCore->Run();
	printx("<--- CnMatrixCore::WorkerThread.\n");
	return 0;
}


BOOL IsBroadcastMac(BYTE *mac)
{
	BYTE data[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	if(!memcmp(data, mac, sizeof(data)))
		return TRUE;

	return FALSE;
}

//BOOL IsDHCPPacket(UCHAR *pData, UINT size)
//{
//	if(pData[12] == 0x08 && pData[13] == 0x00) // ip protocol.
//		if(pData[23] == 0x11) // Udp protocol.
//			if(pData[0x22] == 0x00 && pData[0x23] == 0x44 && pData[0x24] == 0x00 && pData[0x25] == 0x43) // Check udp port.
//				return TRUE;
//
//	return FALSE;
//}
//
//UINT IsArpPacket(UCHAR *pData, UINT size)
//{
////	ASSERT(sizeof(sArpPacket) == 28);
////	DbgPrint("sizeof(sIPHeader) == %d bytes\n", sizeof(sIPHeader));
//
//	if(size < sizeof(sEthernetFrame) + sizeof(sArpPacket))
//		return FALSE;
//
//	if(pData[12] == 0x08 && pData[13] == 0x06) // ARP protocol.
//		return TRUE;
//
//	return FALSE;
//}


enum PACKET_PROTOCOL
{
	PP_NONE = 0,

	PP_ARP_REQUEST,
	PP_ARP_REPLY,
	PP_ARP,

	PP_DHCP,
	PP_DHCP_RELEASE,
	PP_FORCE_DWORD = 0xFFFFFFFF
};


DWORD CheckPacketProtocol(BYTE *pData, UINT size) // 1: ARP. 2: DHCP.
{
	ASSERT(pData && size);

	if(pData[12] == 0x08 && pData[13] == 0x06) // ARP protocol.
	{
		if(pData[21] == 0x01)
			return PP_ARP_REQUEST;
		else
		{
			ASSERT(pData[21] == 0x02);
			return PP_ARP_REPLY;
		}
	}

	if(pData[12] == 0x08 && pData[13] == 0x00) // IP protocol.
		if(pData[23] == 0x11) // Udp protocol.
			if(pData[0x22] == 0x00 && pData[0x23] == 0x44 && pData[0x24] == 0x00 && pData[0x25] == 0x43) // Check udp port.
			{
				if(pData[0x11c] == 0x07) // For windows only.
					return PP_DHCP_RELEASE;
				return PP_DHCP;
			}

	return PP_NONE;
}

UCHAR GArpContent[] =
{
	0x00, 0x0c, 0x29, 0x47, 0xd3, 0xb8, 0x00, 0x1d, 0x7d, 0x0c, 0x6f, 0x21, 0x08, 0x06, 0x00, 0x01,
	0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x00, 0x1d, 0x7d, 0x0c, 0x6f, 0x21, 0xc0, 0xa8, 0x01, 0x02,
	0x00, 0x0c, 0x29, 0x47, 0xd3, 0xb8, 0xc0, 0xa8, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

UCHAR GDhcpAck[] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x45, 0x00,
	0x01, 0x30, 0x04, 0xd4, 0x00, 0x00, 0x80, 0x11, 0x00, 0x00, 0x05, 0x00, 0x00, 0x01, 0xff, 0xff, // Default ip header checksum is null.
	0xff, 0xff, 0x00, 0x43, 0x00, 0x44, 0x01, 0x1c, 0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x4d, 0x02, // UDP packet length and checksum must be set properly.
	0x8a, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x89, 0xc9, 0xfd, 0x05, 0x00,
	0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0xc3, 0x89, 0xc9, 0xfd, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82, 0x53, 0x63, 0x35, 0x01, 0x05, 0x36, 0x04, 0x05,
	0x00, 0x00, 0x01, 0x33, 0x04, 0x01, 0xe1, 0x33, 0x80, 0x01, 0x04, 0xff, 0x00, 0x00, 0x00, 0x03, // T3 option: default gateway.
	0x04, 0x06, 0x00, 0x00, 0x01, /*0x3c, 0x08, 0x4d, 0x53, 0x46, 0x54, 0x20, 0x35, 0x2e, 0x30,*/ 0x2B,
	0x06, 0x03, 0x04, 0x00, 0x00, 0x13, 0x88, 0xff // T43 Vendor-Specific Information. To control default gateway metric.
};


USHORT udp_checksum(const void *buff, size_t len, USHORT *ip_src, USHORT *ip_dst)
{
	const USHORT *buf = (const USHORT*)buff;
	unsigned int sum = 0;
	size_t length = len;

	while(len > 1)
	{
		sum += *buf++;
		if(sum & 0x80000000)
			sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}

	// Add the padding if the packet lenght is odd.
	if(len & 1)
		sum += *((BYTE *)buf);

	// Add the pseudo-header.
	sum += *(ip_src++);
	sum += *ip_src;
	sum += *(ip_dst++);
	sum += *ip_dst;
	sum += htons(IPPROTO_UDP); // 17
	sum += htons((USHORT)length);

	// Add the carries.
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return (USHORT)(~sum); // Return the one's complement of sum.
}

USHORT software_checksum(BYTE *rxtx_buffer, USHORT len, DWORD sum) // Used for ip header.
{
	// Build the sum of 16bit words.
	while(len > 1)
	{
		sum += 0xFFFF & (*rxtx_buffer << 8 | *(rxtx_buffer + 1));
		rxtx_buffer += 2;
		len -= 2;
	}

	// If there is a byte left then add it (padded with zero).
	if(len)
		sum += (0xFF & *rxtx_buffer) << 8;

	// Now calculate the sum over the bytes in the sum until the result is only 16bit long.
	while(sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ((USHORT) sum ^ 0xFFFF); // Build 1's complement.
}

BYTE GDHCPServerMac[6] = { 0x00, 0x23, 0x05, 0x00, 0x00, 0x01 };
DWORD GDHCPServerIP = IP(6, 0, 0, 1), GClientIP = IP(6, 1, 1, 1), GSubnetMask = IP(255, 0, 0, 0);
USHORT GIpID = 0x1200;

BOOL IsSystemVIP(DWORD vip)
{
	if((vip & GSubnetMask) == (GDHCPServerIP & GSubnetMask))
		return TRUE;
	return FALSE;
}

INT MakeDhcpReplyPacket(UCHAR *data, UINT size, UCHAR *out)
{
//	sEthernetFrame *pDesEther, *pSrcEther;
//	sArpPacket *pDesArp, *pSrcArp;
	BYTE *DesMac = 0;
	stIpHeader *pIpHeader;
	USHORT udpChecksum;

	GClientIP = AppGetClientInfo()->vip;

	RtlCopyMemory(out, GDhcpAck, sizeof(GDhcpAck));
	RtlCopyMemory(&out[6], GDHCPServerMac, sizeof(GDHCPServerMac));

	if(data[0] != 0xff)
		RtlCopyMemory(out, &data[6], sizeof(GDHCPServerMac));

	pIpHeader = (stIpHeader*)&out[14];

	// Set ip header.
	pIpHeader->ID = htons(GIpID);
	GIpID++;

	pIpHeader->SourIP = GDHCPServerIP;

	if(data[0] != 0xff)
		pIpHeader->DestIP = ((stIpHeader *)&data[14])->SourIP;
	out[16] = (BYTE)(((sizeof(GDhcpAck) - 14) >> 8) & 0xFF); // Set IP packet length.
	out[17] = (BYTE)((sizeof(GDhcpAck) - 14) & 0xFF);

	pIpHeader->Checksum = htons(software_checksum((BYTE*)pIpHeader, sizeof(stIpHeader), 0));

	// Set bootstrap parameters. Start at 0x2a byte.
	RtlCopyMemory(&out[0x2a + 4], &data[0x2a + 4], 4); // Copy transaction ID.

//	RtlCopyMemory(&out[0x2a + 0x0C], &data[0x1A], 4);    // Copy client ip.
	RtlZeroMemory(&out[0x2a + 0x0C], 4);                 // This must be zero.
	RtlCopyMemory(&out[0x2a + 0x10], &GClientIP, 4);     // Copy new client ip.
	RtlCopyMemory(&out[0x2a + 0x14], &GDHCPServerIP, 4); // Copy server ip.
	RtlCopyMemory(&out[0x2a + 0x1c], &data[6], 6);       // Copy client mac.

	RtlCopyMemory(&out[0x2a + 0xf5], &GDHCPServerIP, 4); // Copy server id.
	RtlCopyMemory(&out[0x2a + 0x0101], &GSubnetMask, 4); // Copy subnet mask.

	if(data[0x011c] == 0x01) // This is dhcp discover packet.
		out[0x011c] = 0x02;

	out[38] = (BYTE)(((sizeof(GDhcpAck) - 14 - 20) >> 8) & 0xFF); // Set UDP packet length.
	out[39] = (BYTE)((sizeof(GDhcpAck) - 14 - 20) & 0xFF);

	udpChecksum = udp_checksum(&out[0x22], sizeof(GDhcpAck) - 14 - 20, (USHORT*)&out[0x1a], (USHORT*)&out[0x1e]); // Set udp checksum.
	RtlCopyMemory(&out[0x28], &udpChecksum, sizeof(udpChecksum));

	return -1;
}

void CopyMac(UCHAR *dest, UCHAR *source)
{
	RtlCopyMemory(dest, source, 6);
}

INT MakeArpReplyPacket(UCHAR *data, UINT size, BYTE *out, DWORD *TargetVIP)
{
	sEthernetFrame *pDesEther, *pSrcEther;
	sArpPacket *pDesArp, *pSrcArp;
	BYTE *DesMac = 0, FIMac[6] = { FIMAC_VB1, FIMAC_VB2, FIMAC_VB3, FIMAC_VB4, 0, 0 };
	BOOL bHasFound = FALSE, bUseFastIndex = AppGetClientInfo()->ServerCtrlFlag & SCF_FMAC_INDEX;
	UINT i = 256;

	pDesEther = (sEthernetFrame*)out;
	pDesArp = (sArpPacket*)(out + sizeof(sEthernetFrame));

	pSrcEther = (sEthernetFrame*)data;
	pSrcArp = (sArpPacket*)(data + sizeof(sEthernetFrame));

	*TargetVIP = pSrcArp->TPA;

	// Check virtual dhcp server first.
	if(pSrcArp->TPA == GDHCPServerIP)
	{
		DesMac = GDHCPServerMac;
		bHasFound = TRUE;
	}
	else
	{
		CMapTable *pTable = &(AppGetClientInfo()->MapTable);
		for(i = 0; i < pTable->m_Count; i++)
			if(pSrcArp->TPA == pTable->Entry[i].vip)
			{
				if(!pTable->Entry[i].port) // 2011/12/05 Arp bug.
					break;

				if(bUseFastIndex)
				{
					FIMac[4] = i & 0xFF;
					FIMac[5] = (i >> 8) & 0xFF;
					DesMac = FIMac;
				}
				else
					DesMac = pTable->Entry[i].mac;

				bHasFound = TRUE;
				break;
			}
	}

	if(bHasFound)
	{
		CopyMac(pDesEther->SourMac, DesMac);
		CopyMac(pDesEther->DestMac, pSrcEther->SourMac);

		pDesArp->Op = 0x0200;

		CopyMac(pDesArp->SHA, DesMac);
		CopyMac(pDesArp->THA, pSrcArp->SHA);

		pDesArp->TPA = pSrcArp->SPA;
		pDesArp->SPA = pSrcArp->TPA;
		return i;
	}

	return -1;
}

struct stDestHostInfo
{
	BOOL   bServerRelay;
	USHORT RHDestIndex;
	USHORT SrcHostDestIndex;
	DWORD  UID;
	BYTE   PacketHeaderFlag;
};

USHORT GetDestInfoByIndex(CMapTable *pAddrTable, USHORT index, IpAddress &ip, USHORT &port, stDestHostInfo &DestInfo) // For broadcast packet only.
{
	UINT count = pAddrTable->m_Count;
	stEntry *pBaseEntry = pAddrTable->Entry, *pEntry = pBaseEntry + index;
	DestInfo.PacketHeaderFlag = PHF_BROADCAST;

	if(pEntry->flag & AIF_PT_FIN || pEntry->flag & AIF_PENDING)
	{
		DestInfo.bServerRelay = pEntry->flag & AIF_SERVER_RELAY;
		DestInfo.UID = pEntry->uid;

		if(pEntry->flag & AIF_RELAY_HOST && pEntry->rhindex < count)
		{
			DestInfo.PacketHeaderFlag |= PHF_USE_RELAY;
			DestInfo.RHDestIndex = pEntry->destindex;
			DestInfo.SrcHostDestIndex = pEntry->dindex;
			pEntry = pBaseEntry + pEntry->rhindex; // Mod pEntry at last.
		}
		ip = pEntry->pip;
		port = pEntry->port;
		return pEntry->dindex;
	}

	return INVALID_DM_INDEX;
}

USHORT FindDestInfoByMac(CMapTable *pAddrTable, BYTE DestMac[], IpAddress &ip, USHORT &port, USHORT &DMIndex, stDestHostInfo &DestInfo)
{
	USHORT i;
	BOOL bFound = FALSE;
	UINT count = pAddrTable->m_Count;
	stEntry *pBaseEntry = pAddrTable->Entry, *pEntry = pBaseEntry;

//	DbgPrint("Address table item count:%d.\n", pAddrTable->Count);
	DestInfo.PacketHeaderFlag = 0;
	DestInfo.RHDestIndex = INVALID_DM_INDEX;

	if(*(UINT*)DestMac == FIMAC_VALUE)
	{
		i = *(USHORT*)(DestMac + sizeof(INT));
		pEntry += i;
		if(i < count) // Must check i here.
			bFound = TRUE;
	}
	else
		for(i = 0; i < count; i++, pEntry++)
			if(!memcmp(pEntry->mac, DestMac, sizeof(pEntry->mac)))
			{
				bFound = TRUE;
				break;
			}

	if(bFound && (pEntry->flag & AIF_PT_FIN || pEntry->flag & AIF_PENDING))
	{
		DestInfo.bServerRelay = pEntry->flag & AIF_SERVER_RELAY;
		DestInfo.UID = pEntry->uid;

		DMIndex = i; // Actaul target driver map index in local client.
		if(pEntry->flag & AIF_RELAY_HOST && pEntry->rhindex < count)
		{
			DestInfo.PacketHeaderFlag |= PHF_USE_RELAY;
			DestInfo.RHDestIndex = pEntry->destindex;
			DestInfo.SrcHostDestIndex = pEntry->dindex;
			pEntry = pBaseEntry + pEntry->rhindex; // Mod pEntry at last.
		}
		ip.v4 = pEntry->pip.v4;
		port = pEntry->port;
		return pEntry->dindex;
	}

	return INVALID_DM_INDEX;
}

void SetupPacketHeader(USHORT dindex, BYTE *pData, stDestHostInfo &DestInfo)
{
	const INT offset = PACKET_SKIP_SIZE - sizeof(stPacketHeader);

	stPacketHeader *pHeader = (stPacketHeader*)&pData[offset];

	pHeader->dindex = dindex;
	pHeader->flag1 = DestInfo.PacketHeaderFlag;
	pHeader->dw = 0;

	if(DestInfo.PacketHeaderFlag & PHF_USE_RELAY)
	{
		pHeader->RHDestIndex = DestInfo.RHDestIndex;
		pHeader->SrcHostDestIndex = DestInfo.SrcHostDestIndex;
	}
}


struct stTimerContext
{
	stTimerContext() { ZeroMemory(this, sizeof(*this)); }

	DWORD lasttime;
	INT iCurrent, iLimit;
	HANDLE hEvent;
	BOOL bKernelWriteThreadExit;
};

stTimerContext GTimerContext;

void TimerCallback(DWORD ctime)
{
	stTimerContext *pCtx = &GTimerContext;
	if(pCtx->iLimit)
	{
		pCtx->iCurrent -= pCtx->iLimit * (ctime - pCtx->lasttime) / (1000 / BANDWIDTH_DL_TIME_RATE);
		if(pCtx->iCurrent < -pCtx->iLimit)
			pCtx->iCurrent = -pCtx->iLimit;
		pCtx->lasttime = ctime;

		SetEvent(pCtx->hEvent);
	}
}

DWORD WINAPI CnMatrixCore::KernelReadThreadEx(LPVOID lpParameter) // Use registered buffer
{
	CNetworkManager *pNM = AppGetNetManager();
	TUNNEL_SOCKET_TYPE *pSocketUDP = pNM->GetTunnelSocket();
	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hAdapter = OpenAdapter(TRUE);

	stReadBuffer ReadBuffer;
	stKBuffer<2> KBuffer; // 1 for dhcp, 2 for arp.
	DWORD dwWritten, bArpDhcp, dwReturn;
	BYTE *pPacketData;
	CMapTable *pTable = &(AppGetClientInfo()->MapTable);
	sockaddr_in addr;

	memcpy(&KBuffer.BufferSlot[1].Buffer, GArpContent, sizeof(GArpContent));
	KBuffer.DataCount = 1;
	addr.sin_family = AF_INET;
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));

	OVERLAPPED wo = {0}, woWrite = {0};
//	wo.hEvent = CreateEvent(0, FALSE, 0, 0);
	RegKernelBuffer(hAdapter, &wo, &ReadBuffer, sizeof(ReadBuffer), TRUE);

	UINT BufferIndex = 0, DataLen;
	const INT offset = PACKET_SKIP_SIZE - sizeof(stPacketHeader);

	HANDLE hEvent[2] = {0};
	hEvent[0] = pClientInfo->hUMSendEvent;
	hEvent[1] = CreateWaitableTimer(0, FALSE, 0);
	if(!hEvent[1])
		printx("CreateWaitableTimer failed! ec: %d\n", GetLastError());

	CPacketQueue PacketQueue;
	INT iLimit = 0, iCurrent = 0;
	DWORD ctime, lasttime;
	BOOL bTimerEvent = FALSE;
	pClientInfo->bSendParamChanged = TRUE;

	stDestHostInfo DestInfo;
	IpAddress pip;
	USHORT port, TDMIndex, DMIndex;

	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = 0;
	if(!SetWaitableTimer(hEvent[1], &liDueTime, (1000 / BANDWIDTH_UL_TIME_RATE), 0, 0, FALSE))
		printx("SetWaitableTimer failed! ec: %d\n", GetLastError());
	ctime = lasttime = GetTickCount();

	while(pClientInfo->bEnableUM)
	{
		for(; ReadBuffer.DataCount || bTimerEvent; )
		{
		//	printx("%d %d %d bTimerEvent: %d Queue: %d\n", ReadBuffer.DataCount, dwCurrent, BufferIndex, bTimerEvent, PacketQueue.GetCount());
		//	printx("Send data count: %d\n", ReadBuffer.DataCount);
			if(BufferIndex == MAX_READ_BUFFER_SLOT)
				BufferIndex = 0;

			pPacketData = ReadBuffer.BufferSlot[BufferIndex].Buffer;
			DataLen = ReadBuffer.BufferSlot[BufferIndex].Len;

			if(DataLen) // DataLen may be null because timer event.
			{
				bArpDhcp = CheckPacketProtocol(pPacketData, DataLen);
				if(bArpDhcp)
				{
					BOOL bContinue = TRUE;

					if(bArpDhcp < PP_ARP)
					{
						DWORD TargetVIP;
						if(bArpDhcp == PP_ARP_REQUEST)
						{
							if(MakeArpReplyPacket(pPacketData, DataLen, KBuffer.BufferSlot[1].Buffer, &TargetVIP) != -1)
							{
								ASSERT(KBuffer.BufferSize == 2 && KBuffer.DataCount == 1);
								KBuffer.BufferSlot[1].Len = sizeof(GArpContent);
								KBuffer.StartIndex = 1;
								WriteFile(hAdapter, &KBuffer, sizeof(KBuffer), &dwWritten, &woWrite);
							}
							else if(IsSystemVIP(TargetVIP)) // Don't send this packet to peers. (Prevent peers updating to real mac)
								bContinue = TRUE;
							else // No target found.
								bContinue = FALSE; // Send arp request packet.
						}
						else // Arp reply packet.
							bContinue = FALSE;
					}
					else if(bArpDhcp != PP_DHCP_RELEASE)
					{
					//	printx("Dhcp packet received!\n");
						ASSERT(bArpDhcp == PP_DHCP && KBuffer.BufferSize == 2 && KBuffer.DataCount == 1);
						MakeDhcpReplyPacket(pPacketData, DataLen, KBuffer.BufferSlot[0].Buffer);
						KBuffer.BufferSlot[0].Len = sizeof(GDhcpAck);
						KBuffer.StartIndex = 0;
						WriteFile(hAdapter, &KBuffer, sizeof(KBuffer), &dwWritten, &woWrite);
					//	bContinue = FALSE;
					}

					if(bContinue)
					{
						ReadBuffer.BufferSlot[BufferIndex++].Len = 0;
						InterlockedDecrement(&ReadBuffer.DataCount);
						continue;
					}
				}
			}

			if(DataLen && iLimit && iCurrent > iLimit)
			{
				PacketQueue.AddPacket(pPacketData, DataLen); // Copy data to user mode space so kernel has more buffer.
				ReadBuffer.BufferSlot[BufferIndex++].Len = 0;
				InterlockedDecrement(&ReadBuffer.DataCount);
				break; // Don't use continue here.
			}

			if(PacketQueue.GetCount())
				pPacketData = PacketQueue.GetHeadData(&DataLen);
			else
			{
				bTimerEvent = 0;
				if(!DataLen)
					break;
			}

			if(IsBroadcastMac(pPacketData))
			{
				for(USHORT j = 0; j < pTable->m_Count; ++j)
					if((TDMIndex = GetDestInfoByIndex(pTable, j, pip, port, DestInfo)) != INVALID_DM_INDEX)
					{
						SetupPacketHeader(TDMIndex, pPacketData, DestInfo);

						if(DestInfo.bServerRelay)
							AddJobServerRelay(DestInfo.UID, pPacketData + offset, DataLen - offset);
						else
						{
							addr.sin_addr.S_un.S_addr = pip.v4;
							addr.sin_port = port;
							pSocketUDP->SendTo(pPacketData + offset, DataLen - offset, (SOCKADDR*)&addr, sizeof(addr), 0);
						}

						iCurrent += (DataLen - offset);
						pTable->DataOut[j] += DataLen;
					}
			}
			else
			{
				if((TDMIndex = FindDestInfoByMac(pTable, pPacketData, pip, port, DMIndex, DestInfo)) != INVALID_DM_INDEX)
				{
					SetupPacketHeader(TDMIndex, pPacketData, DestInfo);
					
					if(DestInfo.bServerRelay)
						AddJobServerRelay(DestInfo.UID, pPacketData + offset, DataLen - offset);
					else
					{
						addr.sin_addr.S_un.S_addr = pip.v4;
						addr.sin_port = port;
						pSocketUDP->SendTo(pPacketData + offset, DataLen - offset, (SOCKADDR*)&addr, sizeof(addr), 0);
					}

					iCurrent += (DataLen - offset);
					pTable->DataOut[DMIndex] += DataLen;
				}
			}

			if(PacketQueue.GetCount())
				PacketQueue.RemoveHeadData();
			else
			{
				ReadBuffer.BufferSlot[BufferIndex++].Len = 0;
				InterlockedDecrement(&ReadBuffer.DataCount);
			}
		}

		dwReturn = WaitForMultipleObjects(2, hEvent, FALSE, INFINITE);

		switch(dwReturn)
		{
			case WAIT_OBJECT_0:
				break;

			case WAIT_OBJECT_0 + 1:
				ctime = GetTickCount();
				if(iLimit)
				{
					iCurrent -= (iLimit * (ctime - lasttime) / (1000 / BANDWIDTH_UL_TIME_RATE));
					lasttime = ctime;

					if(iCurrent < -iLimit)
						iCurrent = 0;
				//	if(iCurrent > iLimit)
				//		iCurrent = iLimit;

					bTimerEvent = TRUE;
				}
				TimerCallback(ctime);
				break;

			default:
				printx("Failed to wait objects! ec: %d", GetLastError());
				break;
		}

		if(pClientInfo->bSendParamChanged)
		{
			iLimit = pClientInfo->iSendLimit; // Changed in JobUpdateConfig.
			iCurrent = 0;
			pClientInfo->bSendParamChanged = FALSE;
		}
	}

	CancelWaitableTimer(hEvent[1]);
	CloseHandle(hEvent[1]);
	while(!GTimerContext.bKernelWriteThreadExit)
	{
		SetEvent(GTimerContext.hEvent);
		Sleep(1);
	}

	RegKernelBuffer(hAdapter, 0, 0, 0, TRUE);
	CloseAdapter(hAdapter);
//	CloseHandle(wo.hEvent);

	printx("<--- KernelReadThreadEx\n");
	return 0;
}

DWORD WINAPI CnMatrixCore::KernelReadThread(LPVOID lpParameter) // Use registered buffer.
{
	CNetworkManager *pNM = AppGetNetManager();
	TUNNEL_SOCKET_TYPE *pSocketUDP = pNM->GetTunnelSocket();
	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hAdapter = OpenAdapter(TRUE);

	stReadBuffer ReadBuffer;
	stKBuffer<2> KBuffer; // 1 for dhcp, 2 for arp.
	DWORD dwWritten, bArpDhcp;
	BYTE *pPacketData;
	CMapTable *pTable = &(AppGetClientInfo()->MapTable);
	sockaddr_in addr;

	memcpy(&KBuffer.BufferSlot[1].Buffer, GArpContent, sizeof(GArpContent));
	KBuffer.DataCount = 1;
	addr.sin_family = AF_INET;
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));

	OVERLAPPED wo = {0}, woWrite = {0};
//	wo.hEvent = CreateEvent(0, FALSE, 0, 0);
	RegKernelBuffer(hAdapter, &wo, &ReadBuffer, sizeof(ReadBuffer), TRUE);

	UINT BufferIndex = 0, DataLen;
	const INT offset = PACKET_SKIP_SIZE - sizeof(stPacketHeader);

	stDestHostInfo DestInfo;
	IpAddress pip;
	USHORT port, TDMIndex, DMIndex; // DMIndex is for traffic statistic.

	while(pClientInfo->bEnableUM)
	{
		for(; ReadBuffer.DataCount; ++BufferIndex)
		{
		//	printx("Send data count: %d\n", ReadBuffer.DataCount);
			if(BufferIndex == MAX_READ_BUFFER_SLOT)
				BufferIndex = 0;

			pPacketData = ReadBuffer.BufferSlot[BufferIndex].Buffer;
			DataLen = ReadBuffer.BufferSlot[BufferIndex].Len;
			ASSERT(DataLen);

			bArpDhcp = CheckPacketProtocol(pPacketData, DataLen);
			if(bArpDhcp)
			{
				BOOL bContinue = TRUE;

				if(bArpDhcp < PP_ARP)
				{
					DWORD TargetVIP;
					if(bArpDhcp == PP_ARP_REQUEST)
					{
						if(MakeArpReplyPacket(pPacketData, DataLen, KBuffer.BufferSlot[1].Buffer, &TargetVIP) != -1)
						{
							ASSERT(KBuffer.BufferSize == 2 && KBuffer.DataCount == 1);
							KBuffer.BufferSlot[1].Len = sizeof(GArpContent);
							KBuffer.StartIndex = 1;
							WriteFile(hAdapter, &KBuffer, sizeof(KBuffer), &dwWritten, &woWrite);
						}
						else if(IsSystemVIP(TargetVIP)) // Don't send this packet to peers. (Prevent peers updating to real mac)
							bContinue = TRUE;
						else // No target found.
							bContinue = FALSE; // Send arp request packet.
					}
					else // Arp reply packet.
						bContinue = FALSE;
				}
				else if(bArpDhcp != PP_DHCP_RELEASE)
				{
				//	printx("Dhcp packet received!\n");
					ASSERT(bArpDhcp == 2 && KBuffer.BufferSize == 2 && KBuffer.DataCount == 1);
					MakeDhcpReplyPacket(pPacketData, DataLen, KBuffer.BufferSlot[0].Buffer);
					KBuffer.BufferSlot[0].Len = sizeof(GDhcpAck);
					KBuffer.StartIndex = 0;
					WriteFile(hAdapter, &KBuffer, sizeof(KBuffer), &dwWritten, &woWrite);
				}

				if(bContinue)
				{
					ReadBuffer.BufferSlot[BufferIndex].Len = 0;
					InterlockedDecrement(&ReadBuffer.DataCount);
					continue;
				}
			}

			if(IsBroadcastMac(pPacketData))
			{
				for(USHORT j = 0; j < pTable->m_Count; ++j)
					if((TDMIndex = GetDestInfoByIndex(pTable, j, pip, port, DestInfo)) != INVALID_DM_INDEX)
					{
						SetupPacketHeader(TDMIndex, pPacketData, DestInfo);

						if(DestInfo.bServerRelay)
							AddJobServerRelay(DestInfo.UID, pPacketData + offset, DataLen - offset);
						else
						{
							addr.sin_addr.S_un.S_addr = pip.v4;
							addr.sin_port = port;
							pSocketUDP->SendTo(pPacketData + offset, DataLen - offset, (SOCKADDR*)&addr, sizeof(addr), 0);
						}

						pTable->DataOut[j] += DataLen;
					}
			}
			else
			{
				if((TDMIndex = FindDestInfoByMac(pTable, pPacketData, pip, port, DMIndex, DestInfo)) != INVALID_DM_INDEX)
				{
					SetupPacketHeader(TDMIndex, pPacketData, DestInfo);

					if(DestInfo.bServerRelay)
						AddJobServerRelay(DestInfo.UID, pPacketData + offset, DataLen - offset);
					else
					{
						addr.sin_addr.S_un.S_addr = pip.v4;
						addr.sin_port = port;
						pSocketUDP->SendTo(pPacketData + offset, DataLen - offset, (SOCKADDR*)&addr, sizeof(addr), 0);
					}

					pTable->DataOut[DMIndex] += DataLen;
				}
			}

			ReadBuffer.BufferSlot[BufferIndex].Len = 0;
			InterlockedDecrement(&ReadBuffer.DataCount);
		}

	//	printx("---> wait for sending.\n");
		WaitForSingleObject(pClientInfo->hUMSendEvent, INFINITE);
	}

	RegKernelBuffer(hAdapter, 0, 0, 0, TRUE);
	CloseAdapter(hAdapter);
//	CloseHandle(wo.hEvent);

	printx("<--- KernelReadThread\n");
	return 0;
}


BOOL DealWithRelayPacket(CMapTable *pTable, TUNNEL_SOCKET_TYPE *pSocketUDP, USHORT usIndex, stPacketHeader *pHeader, UINT DataSize)
{
	stEntry *pDest = &pTable->Entry[pHeader->RHDestIndex];

//	IPV4 v4 = pDest->vip;
//	printx("RH: Packet to %d.%d.%d.%d\n", v4.b1, v4.b2, v4.b3, v4.b4);

	// Modify header.
	pHeader->flag1 &= ~PHF_USE_RELAY; // Must remove PHF_USE_RELAY flag.
	pHeader->flag1 |= PHF_RH_SEND;
	pHeader->dindex = pDest->dindex;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.S_un.S_addr = pDest->pip.v4;
	addr.sin_port = pDest->port;

	INT nSend = pSocketUDP->SendTo(pHeader, DataSize, (SOCKADDR*)&addr, sizeof(addr), 0);
//	if(nSend == SOCKET_ERROR)
//		printx("Failed to send relay packet!\n");

	return TRUE;
}

DWORD WINAPI CnMatrixCore::KernelWriteThreadEx(LPVOID lpParameter)
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hWriteEvent = pClientInfo->hUMRecvEvent;
	TUNNEL_SOCKET_TYPE *pSocketUDP = AppGetNetManager()->GetTunnelSocket();
	HANDLE hAdapter = OpenAdapter(TRUE);
	SOCKET s = pSocketUDP->GetSocket();
	sockaddr_in addr;
	INT Index, AddrLen, DataSize, BufferSlotCount;
	const INT offset = PACKET_SKIP_SIZE - sizeof(stPacketHeader);
	stPacketHeader *pHeader;
	DWORD dwLocalAddressV4 = pClientInfo->ClientInternalIP.v4;
	USHORT usIndex, LocalUDPPort = NBPort(pClientInfo->UDPPort);
	CMapTable *pTable = &(pClientInfo->MapTable);
	BYTE mac[6], *pBuffer;

	ASSERT(hWriteEvent && sizeof(stHelloPacket) == 12);
	memcpy(mac, pClientInfo->vmac, sizeof(mac));
	pClientInfo->bRecvParamChanged = TRUE;
/*
	stKBuffer<16> ReadBuffer; // Test code.
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));
	BufferSlotCount = 16;
/*/
	stReadBuffer ReadBuffer;
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));
	BufferSlotCount = MAX_READ_BUFFER_SLOT;
//*/

	OVERLAPPED wo = {0};
//	wo.hEvent = CreateEvent(0, FALSE, 0, 0);
	RegKernelBuffer(hAdapter, &wo, &ReadBuffer, sizeof(ReadBuffer), FALSE, FALSE);

	stTimerContext &ctx = GTimerContext;
	ASSERT(!ctx.hEvent);
	ZeroMemory(&ctx, sizeof(ctx));
	ctx.hEvent = CreateEvent(0, FALSE, FALSE, 0);

	for(Index = 0; pClientInfo->bEnableUM;)
	{
		//if(ReadBuffer.DataCount == MAX_READ_BUFFER_SLOT)
		//{
		//	printx("Receive buffer fulled.\n");
		//	WaitForSingleObject(hWriteEvent, INFINITE);
		//}
		if(ReadBuffer.BufferSlot[Index].Len) // If use internal buffer, comment these code.
		{
			printx("Buffer collision detected!\n");
			while(ReadBuffer.BufferSlot[Index].Len);
		}

		AddrLen = sizeof(addr);
		pBuffer = ReadBuffer.BufferSlot[Index].Buffer;
		DataSize = recvfrom(s, (char*)(pBuffer + offset), sizeof(ReadBuffer.BufferSlot[0].Buffer) - offset, 0, (sockaddr*)&addr, &AddrLen);

		if(DataSize > (INT)sizeof(stPacketHeader))
		{
			ASSERT(sizeof(stPacketHeader) < sizeof(stHelloPacket));
		//	printx("Recv index: %d, Data size: %d\n", Index, DataSize);
			pHeader = (stPacketHeader*)(pBuffer + offset);
			usIndex = pHeader->dindex;

			if(usIndex < pTable->m_Count && ((addr.sin_addr.S_un.S_addr == pTable->Entry[usIndex].pip.v4 && addr.sin_port == pTable->Entry[usIndex].port) ||
				(addr.sin_addr.S_un.S_addr == dwLocalAddressV4 && addr.sin_port == LocalUDPPort)))
			{
				if(pHeader->flag1 & PHF_USE_RELAY) // The client is relay host.
				{
					if(pTable->Entry[usIndex].flag & AIF_RH_RIGHT && pHeader->RHDestIndex < pTable->m_Count)
						DealWithRelayPacket(pTable, pSocketUDP, usIndex, pHeader, DataSize);
				}
				else
				{
					if(pHeader->flag1 & PHF_RH_SEND)
					{
			//			printx("RH packet received!\n");
						if(pHeader->SrcHostDestIndex < pTable->m_Count)
							usIndex = pHeader->SrcHostDestIndex;
						else
							continue;
					}
					if(pHeader->flag1 & PHF_CTRL_PKT) // Must handle this after checking PHF_RH_SEND flag.
					{
						JobHandleCtrlPacket(pTable, pSocketUDP, pHeader, DataSize, usIndex);
						continue;
					}

					DataSize += offset;
					pTable->DataIn[usIndex] += DataSize;

					memcpy(pBuffer, mac, sizeof(mac));
					memcpy(pBuffer + sizeof(mac), pTable->Entry[usIndex].mac, sizeof(mac));

					ReadBuffer.BufferSlot[Index].Len = DataSize;
					if(InterlockedIncrement((LONG*)&ReadBuffer.DataCount) == 1)
					{
						SetEvent(hWriteEvent);

						if(pClientInfo->bRecvParamChanged)
						{
							ctx.iCurrent = 0;
							ctx.iLimit = pClientInfo->iRecvLimit;
							if(ctx.iLimit)
								ctx.lasttime = GetTickCount(); // Must set last time before creating timer.
							pClientInfo->bRecvParamChanged = FALSE;
						}
					}
					if(++Index == BufferSlotCount)
						Index = 0;

					ctx.iCurrent += (DataSize - offset);
					if(ctx.iLimit && ctx.iCurrent >= ctx.iLimit)
						WaitForSingleObject(ctx.hEvent, INFINITE);
				}
			}
			else if(DataSize == sizeof(stHelloPacket))
			{
				stHelloPacket *pHelloPacket = (stHelloPacket*)(pBuffer + offset);
				usIndex = pHelloPacket->Index;

				if(usIndex < pTable->m_Count && pTable->Entry[usIndex].flag & AIF_START_PT) // Check access index first.
					if(pHelloPacket->key == pTable->Entry[usIndex].uid)
						if(AddrLen == sizeof(sockaddr_in))
						{
							pTable->Entry[usIndex].pip.v4 = addr.sin_addr.S_un.S_addr;
							pTable->Entry[usIndex].port = addr.sin_port;
							pTable->Entry[usIndex].flag = AIF_ACK_RECEIVED | AIF_IPV4;
						}
			}
		}
		else if(!DataSize || DataSize == SOCKET_ERROR)
		{
			printx("Socket recvfrom failed! ec: %d\n", WSAGetLastError());
			break;
		}
	}

	ctx.bKernelWriteThreadExit = TRUE; // Let timer thread know this thread is ready to leave.
	SAFE_CLOSE_HANDLE(ctx.hEvent);
//	CloseHandle(wo.hEvent);

	RegKernelBuffer(hAdapter, 0, 0, 0, FALSE);
	CloseAdapter(hAdapter);

	printx("<--- KernelWriteThreadEx\n");
	return 0;
}

DWORD WINAPI CnMatrixCore::KernelWriteThread(LPVOID lpParameter)
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hWriteEvent = pClientInfo->hUMRecvEvent;
	TUNNEL_SOCKET_TYPE *pSocketUDP = AppGetNetManager()->GetTunnelSocket();
	HANDLE hAdapter = OpenAdapter(TRUE);
	SOCKET s = pSocketUDP->GetSocket();
	sockaddr_in addr;
	INT Index, AddrLen, DataSize, BufferSlotCount;
	const INT offset = PACKET_SKIP_SIZE - sizeof(stPacketHeader);
	stPacketHeader *pHeader;
	DWORD dwLocalAddressV4 = pClientInfo->ClientInternalIP.v4;
	USHORT usIndex, LocalUDPPort = NBPort(pClientInfo->UDPPort);
	CMapTable *pTable = &(AppGetClientInfo()->MapTable);
	BYTE mac[6], *pBuffer;

	ASSERT(hWriteEvent && sizeof(stHelloPacket) == 12);
	memcpy(mac, pClientInfo->vmac, sizeof(mac));

/*
	stKBuffer<16> ReadBuffer; // Test code.
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));
	BufferSlotCount = 16;
/*/
	stReadBuffer ReadBuffer;
	ZeroMemory(&ReadBuffer, sizeof(ReadBuffer));
	BufferSlotCount = MAX_READ_BUFFER_SLOT;
//*/

	OVERLAPPED wo = {0};
//	wo.hEvent = CreateEvent(0, FALSE, 0, 0);
	RegKernelBuffer(hAdapter, &wo, &ReadBuffer, sizeof(ReadBuffer), FALSE, FALSE);

	for(Index = 0; pClientInfo->bEnableUM;)
	{
		//if(ReadBuffer.DataCount == MAX_READ_BUFFER_SLOT)
		//{
		//	printx("Receive buffer fulled.\n");
		//	WaitForSingleObject(hWriteEvent, INFINITE);
		//}
		if(ReadBuffer.BufferSlot[Index].Len) // If use internal buffer, comment these code.
		{
			printx("Buffer collision detected!\n");
			while(ReadBuffer.BufferSlot[Index].Len);
		}

		AddrLen = sizeof(addr);
		pBuffer = ReadBuffer.BufferSlot[Index].Buffer;
		DataSize = recvfrom(s, (char*)(pBuffer + offset), sizeof(ReadBuffer.BufferSlot[0].Buffer) - offset, 0, (sockaddr*)&addr, &AddrLen);

		if(DataSize > (INT)sizeof(stPacketHeader))
		{
			ASSERT(sizeof(stPacketHeader) < sizeof(stHelloPacket));
		//	printx("Recv index: %d, Data size: %d\n", Index, DataSize);
			pHeader = (stPacketHeader*)(pBuffer + offset);
			usIndex = pHeader->dindex;

			if(usIndex < pTable->m_Count && ((addr.sin_addr.S_un.S_addr == pTable->Entry[usIndex].pip.v4 && addr.sin_port == pTable->Entry[usIndex].port) ||
				(addr.sin_addr.S_un.S_addr == dwLocalAddressV4 && addr.sin_port == LocalUDPPort)))
		//	if(usIndex < pTable->m_Count && addr.sin_addr.S_un.S_addr == pTable->Entry[usIndex].pip.v4 && addr.sin_port == pTable->Entry[usIndex].port)
			{
				DataSize += offset;
				pTable->DataIn[usIndex] += DataSize;

				memcpy(pBuffer, mac, sizeof(mac));
				memcpy(pBuffer + sizeof(mac), pTable->Entry[usIndex].mac, sizeof(mac));

				ReadBuffer.BufferSlot[Index].Len = DataSize;
				if(InterlockedIncrement((LONG*)&ReadBuffer.DataCount) == 1)
					SetEvent(hWriteEvent);
				if(++Index == BufferSlotCount)
					Index = 0;
			}
			else if(DataSize == sizeof(stHelloPacket))
			{
				stHelloPacket *pHelloPacket = (stHelloPacket*)(pBuffer + offset);
				usIndex = pHelloPacket->Index;

				if(usIndex < pTable->m_Count && pTable->Entry[usIndex].flag & AIF_START_PT) // Check access code first.
					if(pHelloPacket->key == pTable->Entry[usIndex].uid)
						if(AddrLen == sizeof(sockaddr_in))
						{
							pTable->Entry[usIndex].pip.v4 = addr.sin_addr.S_un.S_addr;
							pTable->Entry[usIndex].port = addr.sin_port;
							pTable->Entry[usIndex].flag = AIF_ACK_RECEIVED | AIF_IPV4;
						}
			}
		}
		else if(!DataSize || DataSize == SOCKET_ERROR)
		{
			printx("Socket recvfrom failed! ec: %d\n", WSAGetLastError());
			break;
		}
	}

	RegKernelBuffer(hAdapter, 0, 0, 0, FALSE);
	CloseAdapter(hAdapter);
//	CloseHandle(wo.hEvent);

	printx("<--- KernelWriteThread\n");
	return 0;
}


