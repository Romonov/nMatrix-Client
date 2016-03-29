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
#include <Mstcpip.h>
#include "CnMatrixCore.h"
#include "CoreAPI.h"
#include "SocketSP.h"
#include "JobQueue.h"
#include "DriverAPI.h"
#include "Profiler.h"


LONG crc32(LONG crc, BYTE *buf, UINT len);


CJobQueue* CJobQueue::pJobQueue = 0;


void ConvertData(stGUIVLanMember *pVLanMember, stNetMember *pMember)
{
	pVLanMember->HostName = pMember->HostName;
	pVLanMember->LinkState = pMember->LinkState;
	pVLanMember->vip = pMember->vip;
	pVLanMember->dwUserID = pMember->UserID;
	pVLanMember->Flag = pMember->Flag;

	pVLanMember->DriverMapIndex = pMember->DriverMapIndex;
	pVLanMember->eip = pMember->eip;
	pVLanMember->eip.m_port = pMember->eip.m_port;
}

void PrintServerCtrlFlag(DWORD dwFlag)
{
	printx("Server ctrl flag (0x%08x):", dwFlag);

	if(dwFlag & SCF_ACCOUNT_LOGIN)
		printx(" SCF_ACCOUNT_LOGIN");
	if(dwFlag & SCF_RENAME_NETWORK)
		printx(" SCF_RENAME_NETWORK");
	if(dwFlag & SCF_CLIENT_RELAY)
		printx(" SCF_CLIENT_RELAY");
	if(dwFlag & SCF_SERVER_NEWS)
		printx(" SCF_SERVER_NEWS");
	if(dwFlag & SCF_TPT_REPORT)
		printx(" SCF_TPT_REPORT");
	if(dwFlag & SCF_TPT_REPORT_OAS)
		printx(" SCF_TPT_REPORT_OAS");
	if(dwFlag & SCF_FMAC_INDEX)
		printx(" SCF_FMAC_INDEX");
	if(dwFlag & SCF_SERVER_REALY)
		printx(" SCF_SERVER_REALY");

	printx("\n");
}

void CJobQueue::AddJob(stJob *pJobIn, UINT DataSize, BOOL bPriority, BOOL bDynaBuff)
{
	POSITION pos;
	UINT JobCounts;
	stJob *pJob;

	if(m_bStop)
	{
		if(bDynaBuff)
			DeleteJob(pJobIn);
		return;
	}

	if(!bDynaBuff)
	{
		pJob = AllocateJob();
		pJob->type = pJobIn->type;
		if(DataSize)
			memcpy(pJob->Data, pJobIn->Data, DataSize);
		pJob->DataLength = sizeof(pJob->type) + DataSize;
	}
	else
	{
		pJob = pJobIn;
		ASSERT(pJob->DataLength == sizeof(pJob->type) + DataSize);
	}

	m_cs.EnterCriticalSection();

	JobCounts = m_List.GetCount() + m_PriorityList.GetCount();

	if(bPriority)
		pos = m_PriorityList.AddHead(pJob);
	else
		pos = m_List.AddHead(pJob);

	m_cs.LeaveCriticalSection();

	if(!JobCounts)
		SetEvent(m_hEvent);
}

stJob* CJobQueue::GetJob()
{
	stJob *pJob = 0;

	m_cs.EnterCriticalSection();

	if(!m_PriorityList.IsEmpty())
		pJob = m_PriorityList.RemoveTail();
	else if(!m_List.IsEmpty())
		pJob = m_List.RemoveTail();

	m_cs.LeaveCriticalSection();

	return pJob;
}

void EnableServerSocketEvent(SOCKET s, BOOL bEnable, HANDLE ServerSocketEvent)
{
	static HANDLE hServerSocketEvent = NULL;

	if(ServerSocketEvent != NULL)
	{
		ASSERT((hServerSocketEvent != NULL && hServerSocketEvent == ServerSocketEvent) || hServerSocketEvent == NULL);
		hServerSocketEvent = (hServerSocketEvent != NULL) ? NULL : ServerSocketEvent;
	}
	else if(bEnable)
	{
		ASSERT(s != INVALID_SOCKET && hServerSocketEvent != NULL);
		WSAEventSelect(s, hServerSocketEvent, FD_READ | FD_CLOSE); // The WSAEventSelect function automatically sets socket to nonblocking mode.
	}
	else
		WSAEventSelect(s, NULL, 0);
}

void JobRegister(stJob *pJob, CSocketTCP *pSocket)
{
	stJob job;
	CStreamBuffer sBuffer;
	job.type = CT_REG;
	DWORD dwFunc, dwResultCode, code[4] = {0}, ServerCtrlFlag, OffloadCaps; // 128 bit request code.

	printx("---> JobRegister().\n");
	if(IS_RESPONSE(pJob->type))
	{
		sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
		sBuffer >> dwFunc >> dwResultCode;

		if(dwFunc == 1)
		{
			if(dwResultCode == 1)
			{
				stClientInfo *pInfo = AppGetClientInfo();

				sBuffer.Read(&pInfo->RegID, sizeof(pInfo->RegID));
				sBuffer.Read(pInfo->vmac, sizeof(pInfo->vmac));
				sBuffer >> pInfo->vip >> ServerCtrlFlag >> OffloadCaps;

				CoreSaveRegisterID(&pInfo->RegID);
				HANDLE hAdapter = OpenAdapter();
				CloseUDP(hAdapter);
				CloseAdapter(hAdapter);
				SetAdapterRegInfo(pInfo->vmac, pInfo->vip, OffloadCaps);
				Sleep(50); // Make sure close is really over.

				//if(memcmp(vmac, pInfo->vmac, sizeof(vmac))) // 2012.6.26 Update adapter parameters in JobLogin.
				//{
				//	RenewAdapter(2);
				//	ResetAdapter();  // Reset virtual adapter.
				//}
				//else if(vip != pInfo->vip)
				//	RenewAdapter(1); // Windows Vista won't renew adapter address in some case.

				stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg:: UF_DWORD5);
				pMsg->DWORD_5 = pInfo->vip;
				pMsg->DWORD_1 = pInfo->RegID.d1;
				pMsg->DWORD_2 = pInfo->RegID.d2;
				pMsg->DWORD_3 = pInfo->RegID.d3;
				pMsg->DWORD_4 = pInfo->RegID.d4;
				NotifyGUIMessage(GET_REGISTER_EVENT, pMsg); // Notify GUI to update.

				pJob->type = CT_LOGIN;
				AppGetJobQueue()->AddJob(pJob, 0);
			}
			else if(dwResultCode == 65535)
			{
				pJob->type = CT_REG;
				sBuffer.Read(code, sizeof(code));
				sBuffer.SetPos(sizeof(dwFunc));
				sBuffer.Write(code, sizeof(code));
				pSocket->SendData(pJob, sizeof(pJob->type) + sizeof(dwFunc) + sizeof(code));
			}
			else // Unknow error.
			{
			}
		}
		else if(dwFunc == 2)
		{
			if(!dwResultCode)
				printx("Register with name ID failed!\n");
			else
				printx("Register with name ID succeeded!\n");
			NotifyGUIMessage(GET_REGISTER_RESULT, dwResultCode);
		}

		return;
	}

	pSocket->SendData(pJob, pJob->DataLength);
}

void JobLogin(stClientInfo *pClientInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	if(IS_RESPONSE(pJob->type))
	{
		if(AppGetState() != AS_LOGIN)
			return;

		BYTE byLen;
		BOOL bInNat;
		USHORT usLen, KeepTunnelTime, KeepTunnelServer, port1, port2;
		DWORD ID1, ID2, dwResultCode, dwTaskOffloadCaps, AdapterCaps = 0;
		CStreamBuffer sBuffer;

		sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
		sBuffer >> dwResultCode;

		if(dwResultCode != LRC_LOGIN_SUCCESS)
		{
			pClientInfo->bLastLoginError = TRUE;
			sBuffer.DetachBuffer();
			NotifyGUIMessage(GET_LOGIN_EVENT, dwResultCode);
			return;
		}

		sBuffer	>> pClientInfo->SyncTime >> KeepTunnelServer >> KeepTunnelTime >> port1 >> port2;
		sBuffer >> bInNat >> ID1 >> ID2;

		sBuffer.ReadString(byLen, _countof(pClientInfo->LoginName) - 1, pClientInfo->LoginName, sizeof(pClientInfo->LoginName));
		sBuffer.ReadString(byLen, _countof(pClientInfo->LoginPassword) - 1, pClientInfo->LoginPassword, sizeof(pClientInfo->LoginPassword));

		sBuffer >> pClientInfo->ServerCtrlFlag >> dwTaskOffloadCaps >> pClientInfo->vip;
		sBuffer.Read(pClientInfo->vmac, sizeof(pClientInfo->vmac));

		// Read aes key.
		sBuffer.ReadString(usLen, _countof(pClientInfo->ClientKey) - 1, pClientInfo->ClientKey, sizeof(pClientInfo->ClientKey));
		pSocket->InitAesData(pClientInfo->ClientKey);

		PrintServerCtrlFlag(pClientInfo->ServerCtrlFlag);
		printx("Login ID: %S Password: %S Aes Key: %s\n", pClientInfo->LoginName, pClientInfo->LoginPassword, pClientInfo->ClientKey);

		sBuffer.DetachBuffer();

		pClientInfo->bInNat = bInNat;
		pClientInfo->bEipFailed = FALSE;
		pClientInfo->ServerUDPPort1 = port1;
		pClientInfo->ServerUDPPort2 = port2;
		pClientInfo->ID1 = ID1;
		pClientInfo->ID2 = ID2;
		pNM->KTSetTimerTick(KeepTunnelServer, KeepTunnelTime);

		BYTE *mac = pClientInfo->vmac;
		IPV4 vip = pClientInfo->vip;
		printx("KT time: %d(s), %d(p).\n", KeepTunnelServer, KeepTunnelTime);
		printx("SyncTime: %d. Server udp port: %d, %d.\n", pClientInfo->SyncTime, port1, port2); // Show info.
		printx("VMac: %02x-%02x-%02x-%02x-%02x-%02x, Vip: %d.%d.%d.%d\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], vip.b1, vip.b2, vip.b3, vip.b4);

		pNM->EnableReportTPTResult(pClientInfo->ServerCtrlFlag & (SCF_TPT_REPORT | SCF_TPT_REPORT_OAS));
		printx("Report TPT result: %d.\n", pClientInfo->ServerCtrlFlag & SCF_TPT_REPORT);
		if(pClientInfo->ServerCtrlFlag & SCF_FMAC_INDEX)
			AdapterCaps |= AFF_USE_FMAC_INDEX;

		pNM->AdjustAdapterParameter(pClientInfo->vip, pClientInfo->vmac, dwTaskOffloadCaps, AdapterCaps);

		if(pClientInfo->ServerCtrlFlag & SCF_SERVER_NEWS)
		{
			pClientInfo->dwClosedServerMsgID = CoreLoadClosedMsgID();
			AddJobQueryServerNews();
		}
		else
		{
			AppSetState(AS_QUERY_SERVER_TIME);
			pClientInfo->ServerTimeOffset = 0.0;
			AddJobQueryServerTime(FALSE);
		}

		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_LOGIN_SUCCESS);
		return;
	}

	ASSERT(AppGetState() == AS_DEFAULT);
	AppSetState(AS_LOGIN);

	stVersionInfo VerInfo;
	DWORD dwLoginMode = pClientInfo->LoginName[0] ? 2 : 1; // 1: use reg ID. 2: use login account.
	VerInfo.AppVer = AppGetVersion();
	VerInfo.AppBuildDate = AppGetBuildDate();
	VerInfo.AppVerFlag = AppGetVersionFlag();

	stDriverVerInfo DriVerInfo = { 0 };
	HANDLE hAdapter = OpenAdapter(); // Open UDP tunnel.

	if(pClientInfo->ConfigData.UDPTunnelAddress)
		pClientInfo->ClientInternalIP.v4 = pClientInfo->ConfigData.UDPTunnelAddress;
	pClientInfo->UDPPort = pClientInfo->ConfigData.UDPTunnelPort;

	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		pNM->KTUnMarkPending(hAdapter, 0, 0); // Remove pending tunnel that will close tunnel socket.
		pClientInfo->MapTable.CleanTable(hAdapter, pNM->KTGetPendingCount());

		GetAdapterDriverVersion(hAdapter, &DriVerInfo);
		VerInfo.DriVer = DriVerInfo.DriVersion;
		VerInfo.DriBuildDate = DriVerInfo.DriBuildDate;
		VerInfo.DriVerFlag = DriVerInfo.Flag;

		pNM->CloseTunnelSocket(hAdapter, FALSE); // Make sure tunnel socket is closed before changing mode silently.
		if(pClientInfo->ConfigData.DataInLimit || pClientInfo->ConfigData.DataOutLimit) // Use user mode to enable bandwidth control.
			pNM->SetTunnelSocketMode(CNetworkManager::TSM_USER_MODE);
		else
			pNM->SetTunnelSocketMode(CNetworkManager::TSM_KERNEL_MODE);

		DWORD ErrorCode = 0;
		if(pNM->CheckTunnelSocketAddress(pClientInfo->ClientInternalIP.v4, pClientInfo->UDPPort))
		{
			if(!pNM->OpenTunnelSocket(hAdapter, pClientInfo->ClientInternalIP.v4, pClientInfo->UDPPort))
				ErrorCode = LRC_SOCKET_ERROR;
		}
		else
			ErrorCode = LRC_UDP_PORT_ERROR;

		CloseAdapter(hAdapter);

		if(ErrorCode)
		{
			pSocket->CloseSocket();
			AppSetState(AS_DEFAULT);
			NotifyGUIMessage(GET_LOGIN_EVENT, ErrorCode);
			return;
		}
	}

	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer.Write(&VerInfo, sizeof(VerInfo));
	sBuffer << dwLoginMode;
	if(dwLoginMode == 1)
	{
		sBuffer.Write(&pClientInfo->RegID, sizeof(pClientInfo->RegID));
		printx("Login with RegID.\n");
	}
	else
	{
		CoreWriteString(sBuffer, (BYTE)0, pClientInfo->LoginName, -1);
		CoreWriteString(sBuffer, (BYTE)0, pClientInfo->LoginPassword, -1);
		printx("Login ID: %s Password: %s.\n", pClientInfo->LoginName, pClientInfo->LoginPassword);
	}

	if(pClientInfo->ConfigData.LocalName[0])
	{
		CoreWriteString(sBuffer, (BYTE)0, pClientInfo->ConfigData.LocalName, -1);
	}
	else
	{
		TCHAR HostName[256];
		DWORD size = _countof(HostName);
		GetComputerNameEx(ComputerNameDnsHostname, HostName, &size);

		CoreWriteString(sBuffer, (BYTE)0, HostName, size);
	}

	sBuffer << pClientInfo->ClientInternalIP << pClientInfo->ClientInternalIP.m_port;
	sBuffer << pClientInfo->ClientInternalIP << pClientInfo->UDPPort;

	pSocket->SendData((BYTE*)pJob, sizeof(pJob->type) + sBuffer.GetDataSize());
	sBuffer.DetachBuffer();
}

void JobDetectNatFirewallType(stJob *pJob)
{
	printx("Enter JobDetectNatFirewallType().\n");

	if(AppGetState() != AS_DETECT_NAT_FIREWALL)
	{
		printx("Error state for NAT & firewall detection: %d.\n", AppGetState());
		return;
	}

	//if(IS_RESPONSE(pJob->type))
	//{
	//}

	// Do nothing currently. Go to next phase.
	stClientInfo *pClientInfo = AppGetClientInfo();
	pClientInfo->bInFirewall = FALSE;
	ASSERT(pClientInfo->QueryCount == 0);

	SetupWaitableTimer();

	// Open the port from windows firewall. Must open for better punching rate.
	AppGetNetManager()->OpenFirewallPort(pClientInfo->UDPPort);

	if(pClientInfo->bInNat /*|| 1*/)
	{
		pClientInfo->ClientExternalIP.ZeroInit();
		AppSetState(AS_QUERY_ADDRESS);
	}
	else
	{
		AppSetState(AS_RETRIEVE_NET_LIST);
		pClientInfo->ClientExternalIP = pClientInfo->ClientInternalIP;
		pClientInfo->ClientExternalIP.m_port = pClientInfo->UDPPort;
		AddJobRetrieveNetList(0, 0);
	}
}

void JobQueryExternelAddress(stJob *pJob)
{
	CIpAddress ip;
	USHORT port;
	CStreamBuffer sBuffer;
	stClientInfo *pClientInfo = AppGetClientInfo();
	CNetworkManager *pNM = AppGetNetManager();

	if(pJob && IS_RESPONSE(pJob->type))
	{
		if(pClientInfo->ClientExternalIP.IsZeroAddress()) // Get net list at first time. Server may send the result twice or more.
		{
			ASSERT(!pNM->GetNetCount());
			AppSetState(AS_RETRIEVE_NET_LIST);
			AddJobRetrieveNetList(0, 0);
		}

		pClientInfo->bEipFailed = FALSE;
		pClientInfo->QueryCount = 0;

		sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
		sBuffer >> ip >> port; // External ip and port reported by server.
		sBuffer.DetachBuffer();

		printx("---> JobQueryExternelAddress(). %d.%d.%d.%d:%d\n", ip.IPV4[0], ip.IPV4[1], ip.IPV4[2], ip.IPV4[3], port);
		pClientInfo->ClientExternalIP = ip;
		pClientInfo->ClientExternalIP.m_port = port;

		if(pClientInfo->bUseUPNP)
			pNM->AddPortMapping(TRUE, pClientInfo->ClientInternalIP, pClientInfo->UDPPort, port);

		// Keep tunnel for server. For in firewall case.
		stKTInfo info;
		ZeroMemory(&info, sizeof(info));
		info.eip = pClientInfo->ServerIP;
		info.eip.m_port = NBPort(pClientInfo->ServerUDPPort1);
		info.uid = 0;
		info.vip = SERVER_VIRTUAL_IP;
		pNM->KTAddHost(&info, TRUE);

		return;
	}

	if(pClientInfo->QueryCount == QUERY_ADDRESS_COUNT)
	{
		pClientInfo->bEipFailed = TRUE;
		pClientInfo->QueryCount = 0;
/*
		AddJobLogin(LOR_QUERY_ADDRESS_FAILED, 0, 0, TRUE);
/*/
		if(pClientInfo->ClientExternalIP.IsZeroAddress()) // Get net list at first time.
		{
			ASSERT(!pNM->GetNetCount());
			AppSetState(AS_RETRIEVE_NET_LIST);
			AddJobRetrieveNetList(0, 0);
		}

		pClientInfo->ClientExternalIP = pClientInfo->ClientInternalIP;
		pClientInfo->ClientExternalIP.m_port = pClientInfo->UDPPort;
//*/
		printx("QueryExternelAddress() failed.\n");

		return;
	}
	pClientInfo->QueryCount++;
	printx("---> JobQueryExternelAddress(). %dth.\n", pClientInfo->QueryCount);

	stJob job;
	job.type = CT_QUERY_EXTERNEL_ADDRESS;
	sBuffer.AttachBuffer(job.Data, sizeof(job.Data));

	sBuffer << (DWORD)pClientInfo->ID1 << (DWORD)pClientInfo->ID2;
	DWORD crc = crc32(0, (BYTE*)&job, sizeof(job.type) + sBuffer.GetDataSize());
	sBuffer << crc;

	// Query external ip and port of our tunnel socket.
	IpAddress DriverIP = AppGetClientInfo()->ServerIP;
	port = NBPort(AppGetClientInfo()->ServerUDPPort1);
	HANDLE hAdapter = OpenAdapter();
	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		pNM->DirectSend(hAdapter, DriverIP, ip.IsIPV6(), port, &job.type, sizeof(pJob->type) + sBuffer.GetDataSize());
		CloseAdapter(hAdapter);
	}

	sBuffer.DetachBuffer();
}

stVPNet* ParseNetListData(CStreamBuffer &sb, CSocketTCP *pSocket)
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	CNetworkManager *pNM = AppGetNetManager();
	CMapTable *pTable = &pClientInfo->MapTable;
	UINT nTotalGroup;
	INT ExistCount;
	USHORT i, j, NetCount, UserCount, NetFlag, DriverMapIndex, LinkState;
	stVPNet *pVNet = NULL;
	stNetMember NetMember;
	stEntry entry;
	DWORD NetIDCode, SelfUID = pClientInfo->ID1;

	ASSERT(!pClientInfo->ClientExternalIP.IsZeroAddress());

	HANDLE hAdapter = OpenAdapter();

	sb >> NetCount;
	ASSERT(!(NetCount > 1 && pNM->GetNetCount()));
	pNM->PrepareHandshakeData();

	for(i = 0; i < NetCount; i++)
	{
		pVNet = pNM->AllocVNet();

		CoreReadString(sb, pVNet->Name);
		sb >> NetIDCode >> pVNet->NetworkType >> NetFlag >> pVNet->m_dwGroupBitMask >> pVNet->m_GroupIndex >> nTotalGroup;

		pVNet->SetFlag(NetFlag);
		pNM->AddVPNet(pVNet, NetIDCode);
		ASSERT(nTotalGroup <= MAX_VNET_GROUP_COUNT);
		pVNet->SetGroupCount(nTotalGroup);
		sb.Read(pVNet->GetGroupArrayAddress(), nTotalGroup * sizeof(stVNetGroup));

		for(j = 0, sb >> UserCount; j < UserCount; ++j)
		{
			NetMember.ReadInfo(sb);
			if(NetMember.UserID == SelfUID)
			{
				pVNet->SetGroupData(NetMember.GroupIndex, NetMember.dwGroupBitMask);
				continue;
			}

			NetMember.DriverMapIndex = INVALID_DM_INDEX;
			if(NetMember.eip.IsZeroAddress()) // Not online.
			{
				NetMember.LinkState = LS_OFFLINE;
				pVNet->AddMember(&NetMember);
				continue;
			}
			NetMember.LinkState = LS_NO_CONNECTION;

			// Update mapping table of driver.
			stNetMember *pMember = pVNet->AddMember(&NetMember);
			ExistCount = pNM->GetConnectedStateInfo(pVNet, pMember, &DriverMapIndex, &LinkState);

			if(!ExistCount || DriverMapIndex == INVALID_DM_INDEX)
			{
				if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
				{
					pMember->GetDriverEntryData(&entry, pClientInfo->ClientInternalIP, pClientInfo->ClientExternalIP);

					pMember->DriverMapIndex = pTable->FindPendingIndex(NetMember.vip);
					if(pMember->DriverMapIndex == INVALID_DM_INDEX)
						pMember->DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
					else
					{
						entry.flag |= AIF_PENDING;
						pTable->SetTableItem(hAdapter, pMember->DriverMapIndex, &entry, FALSE);
					}
					pMember->LinkState = pNM->AddPunchHost(pMember, TRUE);
				}
			}
			else
			{
				ASSERT(ExistCount && DriverMapIndex != INVALID_DM_INDEX);
				if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
				{
					pMember->DriverMapIndex = DriverMapIndex;
					pMember->LinkState = LinkState;
				}
			}
		}
	}

#ifdef _DEBUG
	pNM->DebugCheckState();
#endif

	stJob job;
	CStreamBuffer sBuffer;
	while(pNM->GetHandshakeHostCount()) // Use server to do the handshake.
	{
		SetHostSendJob(&job, HST_PT_HANDSHAKE, SelfUID, &sBuffer);
		pNM->GetHandshakeData(sBuffer, HSF_PT_INFO);
		pSocket->SendData(&job, sizeof(job.type) + sBuffer.GetDataSize());
		sBuffer.DetachBuffer();
	}

	if(hAdapter != INVALID_HANDLE_VALUE)
		CloseAdapter(hAdapter);

	if(NetCount == 1)
		return pVNet;

	return 0;
}

void JobRetrieveNetList(stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobRetrieveNetList().\n");

	if(AppGetState() == AS_DEFAULT) // The operation has been canceled by the user.
		return;
	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	/*	Response packet format
		{
			Header
			{
				DWORD JobType
				DWORD ResultCode; // 1: success 2: access denied 3: VNet not found 4: Unknow error.
				USHORT TotalSize;
				USHORT PacketID;  // base from 0.
			}

			USHORT NetCount;
			{
				BYTE NetNameLength;
				TCHAT NetName[NetNameLength];
				DWORD NetIDCode;
				USHORT UserCount;

				{
					BYTE HostNameLength;
					TCHAR HostName[HostNameLength];
					DWORD NATType, IP;
					USHORT Port;
				}[UserCount]

			}[NetCount]
		}*/

	BOOL bReceiveDone = FALSE, bUseTempBuffer = TRUE;
	DWORD ResultCode; // 1: success 2: access denied 3: VNet not found 4: Unknow error.
	UINT TotalSize;
	USHORT PacketID; // Data only.
	INT HeaderSize = sizeof(pJob->type) + sizeof(ResultCode) + sizeof(TotalSize) + sizeof(PacketID);
	stClientInfo *pInfo = AppGetClientInfo();
	CStreamBuffer sBuffer;

	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> ResultCode >> TotalSize >> PacketID;

	if(ResultCode != 1)
	{
		printx("Error: Can't get net list data.\n");
		return;
	}

//	printx("ID: %d. Total size: %d.\n", PacketID, TotalSize);
	if(PacketID)
	{
		memcpy(&pInfo->pTemp[pInfo->nTempBufferSize], (void*)((DWORD)pJob + HeaderSize), pJob->DataLength - HeaderSize);
		pInfo->nTempBufferSize += (pJob->DataLength - HeaderSize);
		if(pInfo->nTempBufferSize == TotalSize)
			bReceiveDone = TRUE;
	}
	else
	{
		if(TotalSize + HeaderSize > MAX_CMD_PACKET_SIZE)
		{
			ASSERT(!pInfo->pTemp);
			pInfo->pTemp = (BYTE*)malloc(TotalSize);
			pInfo->nTempBufferSize = pJob->DataLength - HeaderSize;
			memcpy(pInfo->pTemp, (void*)((DWORD)pJob + HeaderSize), pJob->DataLength - HeaderSize);
		}
		else
		{
			bUseTempBuffer = FALSE;
			bReceiveDone = TRUE;
		}
	}

	sBuffer.DetachBuffer();

	if(bReceiveDone)
	{
		if(bUseTempBuffer)
			sBuffer.AttachBuffer(pInfo->pTemp, pInfo->nTempBufferSize);
		else
			sBuffer.AttachBuffer((BYTE*)((DWORD)pJob + HeaderSize), TotalSize);

		stVPNet *pJoinNet = ParseNetListData(sBuffer, pSocket);
		ASSERT(sBuffer.GetDataSize() == TotalSize);
		sBuffer.DetachBuffer();

		if(bUseTempBuffer)
		{
			ASSERT(pInfo->pTemp && (pInfo->nTempBufferSize == TotalSize));
			free(pInfo->pTemp);
			pInfo->pTemp = 0;
			pInfo->nTempBufferSize = 0;
		}

		// Update GUI when need. GUI now need update at least one time to initial connection state.
	//	if(pJoinNet || AppGetNetManager()->GetNetCount())
			NotifyGUIMessage(GET_UPDATE_NET_LIST, BuildDataStream(pJoinNet));

		if(AppGetState() == AS_RETRIEVE_NET_LIST)
			AppSetState(AS_READY);
	}
}

void JobCreateSubnet(stJob *pJob, CSocketTCP *pSocket)
{
//	printx("Enter JobCreateSubnet().\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	DWORD dwResult = pJob->dword1; // CSRC_SUCCESS: Success. CSRC_OBJECT_ALREADY_EXIST: NetName has existed. CSRC_INVALID_PARAM: Invalid string.

	//if(dwResult == CSRC_SUCCESS)
	//{
	//	CStreamBuffer sBuffer;
	//	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	//	sBuffer.SetPos(sizeof(pJob->dword1));
	//	DWORD_PTR NetIDCode;
	//	USHORT NetworkType, Flag;
	//	TCHAR NetName[MAX_NET_NAME_LENGTH];
	//	sBuffer >> NetIDCode >> NetworkType >> Flag;
	//	sBuffer.ReadString(1, NetName, sizeof(NetName));
	//	printx(_T("Create VNet(%s) successfully. Flag: %04x\n"), NetName, Flag);
	//	sBuffer.DetachBuffer();
	//}

	NotifyGUIMessage(GET_CREATE_NET_RESULT, dwResult);
}

void JobJoinSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	//printx("Enter JobJoinSubnet().\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	DWORD dwResult = pJob->dword1;

	if(dwResult == CSRC_NOTIFY)
	{
		DWORD NetID;
		stNetMember member;

		CStreamBuffer sBuffer;
		sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
		sBuffer.SetPos(sizeof(pJob->dword1));
		sBuffer >> NetID;
		member.ReadInfo(sBuffer);
		sBuffer.DetachBuffer();

		stVPNet *pVNet = pNM->FindNet(0, NetID);
		if(!pVNet)
			return;

		CMemberUpdateList MemberUpdateList;
		CMapTable *pTable = &pInfo->MapTable;
		stNetMember *pMember = pVNet->AddMember(&member);
		ASSERT(pMember->DriverMapIndex == INVALID_DM_INDEX && pMember->LinkState == LS_OFFLINE && pMember->GUILinkState == LS_OFFLINE);

		if(pMember->IsOnline())
		{
			pMember->LinkState = LS_NO_CONNECTION; // Update the default connection state.

			HANDLE hAdapter = OpenAdapter();
			pNM->UpdateConnectionState(hAdapter, pInfo, pVNet, pMember, MemberUpdateList, FALSE);
			CloseAdapter(hAdapter);

			if(pNM->GetHandshakeHostCount())
			{
				SetHostSendJob(pJob, HST_PT_HANDSHAKE, pInfo->ID1, &sBuffer);
				pNM->GetHandshakeData(sBuffer, HSF_PT_INFO, 1);
				pSocket->SendData(pJob, sizeof(pJob->type) + sBuffer.GetDataSize()); // Use server to do the handshake.
				sBuffer.DetachBuffer();
			}
		}

		stGUIEventMsg *pMsg = AllocGUIEventMsg();
		member.DriverMapIndex = pMember->GUIDriverMapIndex;
		member.LinkState = pMember->GUILinkState;
		ConvertData(&pMsg->member, &member);
		pMsg->DWORD_1 = NetID;
		NotifyGUIMessage(GET_ADD_USER, pMsg);

		return;
	}

	NotifyGUIMessage(GET_JOIN_NET_RESULT, dwResult);
}

void RemoveUserFromVLan(HANDLE hAdapter, CNetworkManager *pNM, CMapTable *pMapTable, stVPNet *pVNet, stNetMember *pMember, CMemberUpdateList &MemberUpdateList)
{
	DWORD DestUID = pMember->UserID;

	pNM->RemoveVNetMember(hAdapter, AppGetClientInfo(), pVNet, pMember, MemberUpdateList);

	stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD3);
	pMsg->DWORD_1 = pVNet->NetIDCode;
	pMsg->DWORD_2 = DestUID;
	//pMsg->DWORD_3 = (ExistCount && LinkState == LS_NO_CONNECTION) ? LS_NO_CONNECTION : 0; // The zero here means GUI doesn't need to update link state.
	NotifyGUIMessage(GET_HOST_EXIT_NET, pMsg);
}

void JobExitSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobExitSubnet().\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	HANDLE hAdapter = INVALID_HANDLE_VALUE;
	DWORD dwReturnCode = pJob->dword1, NetID = pJob->dword2, UID = pJob->dword3;
	CMapTable *pMapTable = &pInfo->MapTable;
	stVPNet *pVNet;
	CStreamBuffer sBuffer;
	CMemberUpdateList MemberUpdateList;
	stGUIEventMsg *pGUIMsg;

	switch(dwReturnCode)
	{
		case CSRC_SUCCESS:
		case CSRC_ADM_OPERATION:

			if((pVNet = pNM->FindNet(0, NetID)) == 0)
				break;

			hAdapter = OpenAdapter();
			pNM->CancelVNetRelay(hAdapter, pMapTable, pVNet, MemberUpdateList);
			pNM->ExitVPNet(hAdapter, pMapTable, pVNet, MemberUpdateList);

			pGUIMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD2);
			pGUIMsg->dwResult = dwReturnCode;
			pGUIMsg->DWORD_2  = NetID;

			pGUIMsg->Heap(MemberUpdateList.GetStreamSize());
			sBuffer.AttachBuffer(pGUIMsg->GetHeapData(), pGUIMsg->GetHeapDataSize());
			MemberUpdateList.WriteStream(sBuffer);

			NotifyGUIMessage(GET_EXIT_NET_RESULT, pGUIMsg);

			break;

		case CSRC_NOTIFY:

			if((pVNet = pNM->FindNet(0, NetID)) == 0)
				break;
			stNetMember *pMember = pVNet->FindHost(UID);
			if(!pMember)
				break;

			hAdapter = OpenAdapter();
			if(pMember->Flag & VF_RELAY_HOST)
				pNM->CancelRelayHost(pVNet, pMember, MemberUpdateList);
			pNM->CancelRelay(hAdapter, pMapTable, pMember, pVNet->NetIDCode, MemberUpdateList);

			RemoveUserFromVLan(hAdapter, pNM, pMapTable, pVNet, pMember, MemberUpdateList);

			if(MemberUpdateList.GetDataCount())
			{
				stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
				NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
			}

			break;
	}

	if(hAdapter != INVALID_HANDLE_VALUE)
		CloseAdapter(hAdapter);
}

void JobRemoveUser(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobRemoveUser().\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pJob->dword2 = pInfo->ID1;
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	CStreamBuffer sBuffer;
	DWORD dwResult, LocalUID, NetID, UserID;
	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> dwResult >> LocalUID >> NetID >> UserID;
	sBuffer.DetachBuffer();

	if(dwResult == CSRC_SUCCESS || dwResult == CSRC_NOTIFY)
	{
		stVPNet *pVNet = pNM->FindNet(0, NetID);
		if(!pVNet)
			return;

		CMemberUpdateList MemberUpdateList;
		HANDLE hAdapter = OpenAdapter();
		CMapTable *pMapTable = &pInfo->MapTable;
		stGUIEventMsg *pGUIMsg;

		if(UserID == pInfo->ID1) // The local host is removed.
		{
			pNM->CancelVNetRelay(hAdapter, pMapTable, pVNet, MemberUpdateList);
			pNM->ExitVPNet(hAdapter, pMapTable, pVNet, MemberUpdateList);

			pGUIMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD2);
			pGUIMsg->dwResult = dwResult;
			pGUIMsg->DWORD_2  = NetID;

			pGUIMsg->Heap(MemberUpdateList.GetStreamSize());
			sBuffer.AttachBuffer(pGUIMsg->GetHeapData(), pGUIMsg->GetHeapDataSize());
			MemberUpdateList.WriteStream(sBuffer);

			NotifyGUIMessage(GET_EXIT_NET_RESULT, pGUIMsg);
		}
		else
		{
			stNetMember *pMember = pVNet->FindHost(UserID);
			if(pMember)
			{
				if(pMember->Flag & VF_RELAY_HOST)
					pNM->CancelRelayHost(pVNet, pMember, MemberUpdateList);
				pNM->CancelRelay(hAdapter, pMapTable, pMember, pVNet->NetIDCode, MemberUpdateList);

				RemoveUserFromVLan(hAdapter, pNM, pMapTable, pVNet, pMember, MemberUpdateList);

				if(MemberUpdateList.GetDataCount())
				{
					stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
					NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
				}
			}
		}

		CloseAdapter(hAdapter);
	}
}

void JobOfflineSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobOfflineSubnet().\n");
	BOOL bOnline;
	DWORD dwResult, UID, NetID;

	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> dwResult >> bOnline >> UID >> NetID;
	sBuffer.DetachBuffer();

	if(!IS_RESPONSE(pJob->type))
	{
		stVPNet *pNet = pNM->FindNet(0, NetID);
		if(!pNet || (bOnline && pNet->IsNetOnline()) || (!bOnline && !pNet->IsNetOnline())) // Check vnet state.
		{
			ASSERT(0);
			return;
		}
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	stVPNet *pVNet;
	CMemberUpdateList MemberUpdateList;

	if(dwResult == CSRC_SUCCESS)
	{
		if((pVNet = pNM->FindNet(0, NetID)) == 0)
			return;

		HANDLE hAdapter = OpenAdapter();

		if(!bOnline) // Offline.
			pNM->CancelVNetRelay(hAdapter, &pInfo->MapTable, pVNet, MemberUpdateList);

		pNM->OfflineVPNet(hAdapter, pInfo, pVNet, MemberUpdateList, bOnline);

		if(bOnline) // Use server to do the handshake.
			while(pNM->GetHandshakeHostCount())
			{
				SetHostSendJob(pJob, HST_PT_HANDSHAKE, pInfo->ID1, &sBuffer);
				pNM->GetHandshakeData(sBuffer, HSF_PT_INFO);
				pSocket->SendData(pJob, sizeof(pJob->type) + sBuffer.GetDataSize());
				sBuffer.DetachBuffer();
			}

		CloseAdapter(hAdapter);
	}

	stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD3);
	pMsg->dwResult = dwResult;
	pMsg->DWORD_2 = NetID;
	pMsg->DWORD_3 = bOnline;

	pMsg->Heap(MemberUpdateList.GetStreamSize());
	sBuffer.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	MemberUpdateList.WriteStream(sBuffer);

	NotifyGUIMessage(GET_OFFLINE_SUBNET, pMsg);
}

void JobDeleteSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	//printx("---> JobDeleteSubnet().\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	CMemberUpdateList MemberUpdateList;
	DWORD dwResult = pJob->dword1, NetID = pJob->dword2;

	if(dwResult == CSRC_SUCCESS || dwResult == CSRC_NOTIFY)
	{
		stVPNet *pVNet = pNM->FindNet(0, NetID);
		if(!pVNet)
			return;

		HANDLE hAdapter = OpenAdapter();
		CMapTable *pTable = &pInfo->MapTable;

		pNM->CancelVNetRelay(hAdapter, pTable, pVNet, MemberUpdateList); // Cancel relay to restore link state of the members before actually exit the network.
		pNM->ExitVPNet(hAdapter, pTable, pVNet, MemberUpdateList); // This will delete pVNet.

		CloseAdapter(hAdapter);
	}

#ifdef _DEBUG
	pNM->DebugCheckState();
#endif

	stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD2); // Update GUI.
	pMsg->dwResult = dwResult;
	pMsg->DWORD_2  = NetID;

	pMsg->Heap(MemberUpdateList.GetStreamSize());
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	MemberUpdateList.WriteStream(sBuffer);

	NotifyGUIMessage(GET_DELETE_NET_RESULT, pMsg);
}

void OnServerNotifyOfflineNet(stJob *pJob, CStreamBuffer &sBuffer, CSocketTCP *pSocket)
{
	printx("---> OnServerNotifyOfflineNet().\n");

	CNetworkManager *pNM = AppGetNetManager();
	BOOL bOnline;
	DWORD UID, NetID;

	sBuffer >> bOnline >> UID >> NetID;

	stVPNet *pVNet = pNM->FindNet(0, NetID);
	if(!pVNet)
		return;
	stNetMember *pMember = pVNet->FindHost(UID);
	if(!pMember)
		return;

	ASSERT(pMember->IsOnline());
	ASSERT((bOnline && !pMember->IsNetOnline()) || (!bOnline && pMember->IsNetOnline()));

	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hAdapter = OpenAdapter();
	CMapTable *pTable = &pClientInfo->MapTable;
	CMemberUpdateList MemberUpdateList;

	if(bOnline)
	{
		pMember->Flag |= VF_IS_ONLINE;

		pNM->UpdateConnectionState(hAdapter, pClientInfo, pVNet, pMember, MemberUpdateList, FALSE);

		if(pNM->GetHandshakeHostCount()) // Use server to do the handshake.
		{
			stJob job;
			CStreamBuffer sb;
			SetHostSendJob(&job, HST_PT_HANDSHAKE, pClientInfo->ID1, &sb);
			pNM->GetHandshakeData(sb, HSF_PT_INFO, 1);
			pSocket->SendData(&job, sizeof(job.type) + sb.GetDataSize());
			sb.DetachBuffer();
		}
	}
	else
	{
		pMember->Flag &= ~VF_IS_ONLINE;

		pNM->CancelRelay(hAdapter, pTable, pMember, NetID, MemberUpdateList); // The connection to this host uses relay. Must restore before call pNM->UpdateConnectionState.
		pNM->UpdateConnectionState(hAdapter, pClientInfo, pVNet, pMember, MemberUpdateList, FALSE);

		if(pMember->Flag & VF_RELAY_HOST)
			pNM->CancelRelayHost(pVNet, pMember, MemberUpdateList);
	}
	CloseAdapter(hAdapter);

#ifdef _DEBUG
	pNM->DebugCheckState();
#endif

	stGUIEventMsg *pData = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD3); 
	pData->DWORD_1 = bOnline;
	pData->DWORD_2 = pVNet->NetIDCode;
	pData->DWORD_3 = pMember->UserID;
	NotifyGUIMessage(GET_OFFLINE_SUBNET_PEER, pData);

	if(MemberUpdateList.GetDataCount()) // Update GUI if need.
	{
		pData = SetupGUIUpdateData(MemberUpdateList);
		NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pData);
	}
}

void OnServerNotifyUserOnline(stJob *pJob, CStreamBuffer &sBuffer, CSocketTCP *pSocket)
{
//	printx("---> OnServerNotifyUserOnline().\n");

	DWORD UID, dwReason;
	stNetMember member;
	CNetworkManager *pNM = AppGetNetManager();
	stClientInfo *pClientInfo = AppGetClientInfo();
	CMemberUpdateList MemberUpdateList;
	BYTE bOnline;
	BOOL bKeepTunnel = FALSE;
	TCHAR HostName[MAX_NET_NAME_LENGTH + 1];
	INT iBufLen = _countof(HostName);

	sBuffer >> bOnline >> UID;
	CoreReadString(sBuffer, HostName, &iBufLen);

	if(bOnline)
		member.ReadInfo(sBuffer);
	else
	{
		sBuffer >> dwReason;
		member.UserID = UID;
		member.HostName = HostName;
	}

	// Update host state of vlan.
	BOOL bNeedPTT;
	CList<stNetMemberInfo> MemberList;
	if(!pNM->FindNetMember(member.UserID, &MemberList, bNeedPTT))
		return; // For error tolerance.

	// Keep driver map index and virtual ip.
	member.vip = MemberList.GetHead().pMember->vip;
	USHORT DriverMapIndex, LinkState;
	UINT ExistCount = pNM->GetUserExistCount(member.UserID, &DriverMapIndex, &LinkState);

	// Update driver data.
	HANDLE hAdapter = OpenAdapter();
	stEntry entry;
	CMapTable *pTable = &pClientInfo->MapTable;

	if(bOnline)
	{
		//ASSERT(DriverMapIndex == INVALID_DM_INDEX); // User may have already online (Disconnected without notify).
		if(DriverMapIndex != INVALID_DM_INDEX)
		{
			pNM->CancelPunchHost(UID);
			pNM->KTRemoveHost(UID);
		}

		if(bNeedPTT)
		{
			member.GetDriverEntryData(&entry, pClientInfo->ClientInternalIP, pClientInfo->ClientExternalIP);
			if(DriverMapIndex != INVALID_DM_INDEX)
			{
				pTable->SetTableItem(hAdapter, DriverMapIndex, &entry, TRUE);

				for(POSITION pos = MemberList.GetHeadPosition(); pos;)
				{
					stNetMemberInfo &Info = MemberList.GetNext(pos);
					if(Info.pMember->Flag & VF_RELAY_HOST)
						pNM->CancelRelayHost(Info.pVNet, Info.pMember, MemberUpdateList);
				}
			}
			else
			{
				DriverMapIndex = pTable->FindPendingIndex(member.vip);
				if(DriverMapIndex == INVALID_DM_INDEX)
					DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
				else
				{
					entry.flag |= AIF_PENDING;
					pTable->SetTableItem(hAdapter, DriverMapIndex, &entry, FALSE);
				}

			//	DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
			}
			member.DriverMapIndex = DriverMapIndex; // Must set DriverMapIndex first then call AddPunchHost.
			pNM->PrepareHandshakeData();
			member.LinkState = LinkState = pNM->AddPunchHost(&member);

			stJob job;
			CStreamBuffer sb;
			SetHostSendJob(&job, HST_PT_HANDSHAKE, pClientInfo->ID1, &sb);
			pNM->GetHandshakeData(sb, HSF_PT_INFO, 1);
			pSocket->SendData(&job, sizeof(job.type) + sb.GetDataSize()); // Use server to do the handshake.
			sb.DetachBuffer();

			for(POSITION pos = MemberList.GetHeadPosition(); pos;)
			{
				stNetMemberInfo &info = MemberList.GetNext(pos);

				if(info.pVNet->IsNetOnline() && info.pMember->IsNetOnline() && NeedConnect(info.pVNet, info.pMember))
				{
					member.DriverMapIndex = DriverMapIndex;
					member.LinkState = LinkState;
				}
				else
				{
					member.DriverMapIndex = INVALID_DM_INDEX;
					member.LinkState = LS_NO_CONNECTION;
				}
				info.pMember->UpdataOnlineInfo(member);
			}
			member.LinkState = LinkState; // Set GUI state.
		}
		else
		{
			ASSERT(DriverMapIndex == INVALID_DM_INDEX);

			member.DriverMapIndex = INVALID_DM_INDEX;
			member.LinkState = LS_NO_CONNECTION;
			for(POSITION pos = MemberList.GetHeadPosition(); pos;)
				MemberList.GetNext(pos).pMember->UpdataOnlineInfo(member);
		}
	}
	else
	{
		printx("Reason: %d\n", dwReason);
		BOOL bCloseTunnel = TRUE;
		pNM->CancelPunchHost(UID);

		if(dwReason == WSAECONNRESET && pClientInfo->ConfigData.KeepTunnelTime)
			bCloseTunnel = !pNM->KTMarkPending(hAdapter, UID, pClientInfo->ConfigData.KeepTunnelTime);

		if(bCloseTunnel)
		{
			pNM->KTRemoveHost(UID);

			if(DriverMapIndex != INVALID_DM_INDEX)
			{
				pTable->SetTableItem(hAdapter, DriverMapIndex, 0);
				pTable->RecycleMapIndex(DriverMapIndex, __LINE__);
				ASSERT(member.LinkState == LS_OFFLINE);
			}
		}

		member.DriverMapIndex = INVALID_DM_INDEX;
		for(POSITION pos = MemberList.GetHeadPosition(); pos;)
		{
			stNetMemberInfo &Info = MemberList.GetNext(pos);
			if(Info.pMember->Flag & VF_RELAY_HOST)
				pNM->CancelRelayHost(Info.pVNet, Info.pMember, MemberUpdateList);
			Info.pMember->UpdataOnlineInfo(member);
		}
	}

	CloseAdapter(hAdapter);

#ifdef _DEBUG
	pNM->DebugCheckState();
#endif

	stGUIEventMsg *pMsg = AllocGUIEventMsg(); // Update GUI finally.
	pMsg->dwResult = bOnline;
	ConvertData(&pMsg->member, &member);
	NotifyGUIMessage(GET_HOST_ONLINE, pMsg);

	if(MemberUpdateList.GetDataCount())
	{
		pMsg = SetupGUIUpdateData(MemberUpdateList);
		NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
	}
}

void OnServerNotifyUpdateAddress(stJob *pJob, CStreamBuffer &sBuffer)
{
	// 20110128 This function is obsoleted.
/*
	DWORD uid;
	CIpAddress eip, iip;
	USHORT eport, iport;
	BYTE len;
	TCHAR HostName[128];
	CNetworkManager *pNM = AppGetNetManager();
	CList<stNetMember*> MemberList;
	stNetMember *pMember;

	sBuffer >> uid >> len;
	sBuffer.Read(HostName, sizeof(TCHAR) * len);
	HostName[len] = 0;

	pNM->FindNetMember(uid, &MemberList);
	POSITION pos = MemberList.GetHeadPosition();
	if(!pos)
		return;

	sBuffer >> eip >> eport;
	sBuffer >> iip >> iport;
	pMember = MemberList.GetNext(pos);
	pMember->eip = eip;
	pMember->eport = eport;
	ASSERT(pMember->iip == iip && pMember->iport == iport);

	HANDLE hDevice = OpenAdapter();
	if(hDevice)
	{
		stEntry entry;
		pMember->GetDriverEntryData(&entry, AppGetClientInfo()->ClientInternalIP, AppGetClientInfo()->ClientExternalIP);
		AppGetClientInfo()->MapTable.SetTableItem(hDevice, 1, pMember->DriverMapIndex, &entry);
		CloseAdapter(hDevice);
	}

	for(; pos; )
	{
		pMember = MemberList.GetNext(pos);
		pMember->eip = eip;
		pMember->eport = eport;
	}

	// Do NAT traversal finally.
	//if(pMember->bInNat)
	ASSERT(pMember->bInNat);
	pNM->AddPunchHost(pMember);
*/
}

BOOL JobServerNotify(stJob *pJob, CSocketTCP *pSocket)
{
	DWORD FuncCode;
	CStreamBuffer sBuffer;

	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> FuncCode;

	switch(FuncCode)
	{
		case SNF_OFFLINE_NET:
			OnServerNotifyOfflineNet(pJob, sBuffer, pSocket);
			break;

		case SNF_USER_ONLINE:
			OnServerNotifyUserOnline(pJob, sBuffer, pSocket);
			break;

		case SNF_UPDATE_ADDRESS:
			OnServerNotifyUpdateAddress(pJob, sBuffer);
			break;

		case SNF_MSG:
			break;
	}

	sBuffer.DetachBuffer();
	return TRUE;
}

void JobHostSend(stJob *pJob)
{
	if(!IS_RESPONSE(pJob->type) || pJob->dword3 != AppGetClientInfo()->ID1) // Make the response only.
		return;

	CStreamBuffer sb;
	DWORD SenderUID = pJob->dword2;
	DWORD SelfUID = pJob->dword3;
	DWORD DataSize = pJob->dword4;

	printx("---> JobHostSend. Sender UID: %08x.\n", SenderUID);

	switch(pJob->dword5) // Function code.
	{
		case HSF_PT_ACK:
			if(!AppGetNetManager()->PTSetAck(SenderUID))
				printx("Error! Host not found when call PTSetAck().\n"); // This could happen when user offline suddenly.
			break;

		case HSF_PT_INFO:
		{
			USHORT DMIndex;
			DWORD key;
			UINT64 nStartTime;
			sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));
			sb.SetPos(sizeof(DWORD) * 5);
			sb >> DMIndex >> key >> nStartTime;
			INT Result = AppGetNetManager()->PTSetInitData(SenderUID, DMIndex, key, nStartTime);
			if(!Result)
			{
				printx("Error! Host not found when call PTSetInitData().\n");
				AddJobReportBug(CBC_PTSetInitData);
			}
			else if(Result == -1)
			{
	//			ASSERT(0);
				AddJobReportBug(CBC_PTTimeOrderError);
			}

			printx("HSF_PT_INFO Data size: %d. Driver Map Index: %d. Key: %08x.\n", DataSize, DMIndex, key);
			break;
		}
	}
}

void JobSetting(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	if(IS_RESPONSE(pJob->type))
	{
		stGUIEventMsg *pData = 0;
		BOOL bSet = pJob->dword1 & JST_SET_DATA;
		DWORD dwType = pJob->dword1 & ~JST_SET_DATA;
		stVPNet *pVNet = 0;
		stNetMember *pDestMember = 0, *pRelayHost = 0;
		CMapTable *pTable = &pInfo->MapTable;

		switch(dwType)
		{
			case JST_NET_PASSWORD:
			{
				printx("Reset network password.\n");
				break;
			}
			case JST_NET_FLAG:
			{
				if(pJob->dword2)
					if(pVNet = pNM->FindNet(0, pJob->dword3))
					{
						pVNet->UpdateFlag((USHORT)pJob->dword4);

						pData = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD1 | stGUIEventMsg::UF_DWORD2 | stGUIEventMsg::UF_DWORD3 | stGUIEventMsg::UF_DWORD4);
						pData->DWORD_1 = pJob->dword1; // Type.
						pData->DWORD_2 = pJob->dword2; // Result.
						pData->DWORD_3 = pJob->dword3; // Net ID.
						pData->DWORD_4 = pJob->dword4; // Flag.
					}
				break;
			}

			case JST_HOST_ROLE_HUB:
			case JST_HOST_ROLE_RELAY:
			{
				DWORD NetID = pJob->dword3, DestUID = pJob->dword4, dwMask = pJob->dword6, dwFlag = pJob->dword5;
				BOOL bRelayHostCancelled;
				CMemberUpdateList MemberUpdateList;

				if(pNM->UpdateMemberRole(NetID, DestUID, dwFlag, dwMask, pVNet, pDestMember, bRelayHostCancelled)) // Reference to AddJobSetHostRole for detail info.
				{
					HANDLE hAdapter = OpenAdapter();

					if(dwMask & VF_HUB) // Check mask type.
					{
						if(!(dwFlag & VF_HUB))
						{
							if(pDestMember)
							{
								if(pDestMember->Flag & VF_RELAY_HOST) // Dest is the relay host.
									pNM->CancelRelayHost(pVNet, pDestMember, MemberUpdateList, TRUE);
								if(pNM->GetRelayHost(pTable, pVNet, pDestMember, &pRelayHost) && (!(pRelayHost->Flag & VF_HUB) || !(pVNet->Flag & VF_HUB)))
									pNM->CancelRelay(hAdapter, pTable, pDestMember, pVNet->NetIDCode, MemberUpdateList);
							}
							else // Dest is local host.
							{
								CList<stNetMember> *pMemberList = pVNet->GetMemberList();
								for(POSITION pos = pMemberList->GetHeadPosition(); pos;)
								{
									pDestMember = &pMemberList->GetNext(pos);
									if(!pDestMember->IsOnline() || !pNM->GetRelayHost(pTable, pVNet, pDestMember, &pRelayHost))
										continue;
									if(!(pDestMember->Flag & VF_HUB) || !(pRelayHost->Flag & VF_HUB))
										pNM->CancelRelay(hAdapter, pTable, pDestMember, pVNet->NetIDCode, MemberUpdateList);
								}
							}
						}

						BOOL bActive = (DestUID == pInfo->ID1);
						pNM->UpdateConnectionState(hAdapter, pInfo, pVNet, MemberUpdateList, bActive);

						if((dwFlag & VF_HUB) && pNM->GetHandshakeHostCount()) // Use server to do the handshake.
						{
							stJob job;
							CStreamBuffer sBuffer;
							while(pNM->GetHandshakeHostCount())
							{
								SetHostSendJob(&job, HST_PT_HANDSHAKE, pInfo->ID1, &sBuffer);
								pNM->GetHandshakeData(sBuffer, HSF_PT_INFO);
								pSocket->SendData(&job, sizeof(job.type) + sBuffer.GetDataSize());
								sBuffer.DetachBuffer();
							}
						}
					}

					if((dwMask == VF_ALLOW_RELAY || dwMask == VF_RELAY_HOST) && bRelayHostCancelled && pDestMember) // If pDestMember is null, that means this host is relay host.
						pNM->CancelRelayHost(pVNet, pDestMember, MemberUpdateList);

					CloseAdapter(hAdapter);

					if(MemberUpdateList.GetDataCount())
					{
						stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
						NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
					}

					pData = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD1 | stGUIEventMsg::UF_DWORD2 | stGUIEventMsg::UF_DWORD3 | stGUIEventMsg::UF_DWORD4 | stGUIEventMsg::UF_DWORD5);
					pData->DWORD_1 = pJob->dword1; // Type.
					pData->DWORD_2 = pJob->dword3; // Net ID.
					pData->DWORD_3 = (pJob->dword4 == AppGetClientInfo()->ID1) ? 0 : pJob->dword4; // UID.
					pData->DWORD_4 = pJob->dword5; // Flag.
					pData->DWORD_5 = pJob->dword6; // Mask.
				}
				break;
			}
		}

		if(pData)
			NotifyGUIMessage(GET_SETTING_RESPONSE, pData);

		return;
	}
	else
	{
		if(pJob->dword1 == JST_CLOSED_SERVER_MSG_ID)
		{
			pInfo->dwClosedServerMsgID = pJob->dword2;
			CoreSaveClosedMsgID(pJob->dword2);
			return;
		}
	}

	pSocket->SendData(pJob, pJob->DataLength);
}

void JobClientQuery(stClientInfo *pInfo, stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobClientQuery.\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	stGUIEventMsg *pMsg = NULL;
	DWORD type, rcode, nid, uid;
	CStreamBuffer sb;
	CString csNews;
	sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sb >> type >> rcode;

	switch(type)
	{
		case CQT_SERVER_NEWS:

			pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD1 | stGUIEventMsg::UF_DWORD2 | stGUIEventMsg::UF_DWORD3);
			if(pMsg != NULL)
			{
				if(rcode) // Msg data size.
				{
					sb >> pMsg->DWORD_3; // Message ID.
					CoreReadString(sb, csNews);
					pMsg->Heap((csNews.GetLength() + 1) * sizeof(TCHAR), csNews.GetBuffer()); // Null-terminated string.
					if(AppGetnMatrixCore()->IsServiceMode())
					{
						pInfo->csServerNews = (TCHAR*)pMsg->GetHeapData();
						pInfo->dwServerMsgID = pMsg->DWORD_3;
					}
				}
				pMsg->DWORD_1 = type;
				pMsg->DWORD_2 = rcode; // Size.
				pMsg->DWORD_4 = pInfo->dwClosedServerMsgID;
			}

			if(AppGetState() == AS_LOGIN)
			{
				AppSetState(AS_QUERY_SERVER_TIME);
				pInfo->ServerTimeOffset = 0.0;
				AddJobQueryServerTime(FALSE);
			}

			break;

		case CQT_HOST_LAST_ONLINE_TIME:
			{
				time_t time = 0;
				sb >> nid >> uid;
				sb.Read(&time, sizeof(time));

				pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD1 | stGUIEventMsg::UF_DWORD2 | stGUIEventMsg::UF_DWORD3 | stGUIEventMsg::UF_DWORD4);
				if(pMsg != NULL)
				{
					pMsg->DWORD_1 = type;
					pMsg->DWORD_2 = rcode;
					pMsg->DWORD_3 = (time >> 32) & 0xFFFFFFFF;
					pMsg->DWORD_4 = time & 0xFFFFFFFF;
					pMsg->member.dwUserID = uid;
				}
			}
			break;

		case CQT_VNET_MEMBER_INFO:

		//	printx("CQT_VNET_MEMBER_INFO - Return code: %d. %dth packet. Count: %d\n", pJob->dword2, pJob->dword3, pJob->dword4);
			if(!pJob->dword2)
				break;
			pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD2);
			if(pMsg != NULL)
			{
				pMsg->DWORD_1 = type;
				pMsg->DWORD_2 = rcode;
				pMsg->Heap(pJob->DataLength, &pJob->dword1);
			}

			break;
	}

	sb.DetachBuffer();

	if(pMsg != NULL)
		NotifyGUIMessage(GET_CLIENT_QUERY, pMsg);
}

void JobClientReport(stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobClientReport.\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}
}

void JobQueryServerTime(stClientInfo *pClientInfo, stJob *pJob, CSocketTCP *pSocket, UINT &SyncTimerTick)
{
//	printx("---> JobQueryServerTime\n");

	static FLOAT  fPing[QUERY_SERVER_TIME_COUNT];
	static DOUBLE dOffset[QUERY_SERVER_TIME_COUNT];

	stQueryTimeStruct TimeInfo;
	CStreamBuffer sBuffer;

	if(IS_RESPONSE(pJob->type))
	{
		LARGE_INTEGER liCurrentTime;
		QueryPerformanceCounter(&liCurrentTime);

		memcpy(&TimeInfo, pJob->Data, sizeof(TimeInfo));

		DOUBLE dPing = (DOUBLE)(liCurrentTime.QuadPart - TimeInfo.ClientTime.QuadPart) * pClientInfo->InvFreq;
		DOUBLE dServerTime = (DOUBLE)TimeInfo.ServerTime.QuadPart * 1000 / TimeInfo.ServerFreq.QuadPart;
		DOUBLE dClientTime = (DOUBLE)liCurrentTime.QuadPart * pClientInfo->InvFreq;
		DOUBLE ServerTimeOffset = (dClientTime - dServerTime - dPing / 2);

		UINT QueryCount = pClientInfo->QueryServerTimeCounter;

		if(QueryCount < QUERY_SERVER_TIME_COUNT)
		{
			fPing[QueryCount] = (FLOAT)dPing;
			dOffset[QueryCount] = ServerTimeOffset;
			pClientInfo->QueryServerTimeCounter++;

			//printx("Ping server: %.3fms. Server Time: %.3f. Client Time: %.3f\n", dPing, dServerTime, dClientTime);
			// This will cause large delay when runs in vmware with pppoe.

			if(QueryCount == QUERY_SERVER_TIME_COUNT - 1)
			{
				FLOAT f = 99999.0f;
				DOUBLE dBestOffset, minus;
				printx("Server Time Result:\n");
				for(UINT i = 0; i < QUERY_SERVER_TIME_COUNT; ++i)
				{
					printx("Ping: %f. Offset: %.6f.\n", fPing[i], dOffset[i]);

					if(fPing[i] < f) // Get the lowest ping.
					{
						f = fPing[i];
						dBestOffset = dOffset[i];
					}
				}

				ASSERT(!SyncTimerTick);
				minus = pClientInfo->ServerTimeOffset - dBestOffset;
				if(-0.5 < minus && minus < 0.5)
					pClientInfo->SyncTimeScale++;
				else
					pClientInfo->SyncTimeScale = 1;
				SetRange(pClientInfo->SyncTimeScale, (DWORD)1, (DWORD)4);
				SyncTimerTick = pClientInfo->SyncTime * pClientInfo->SyncTimeScale;

				printx("Pick up ping: %f. Offset: %f. Default: %f.\nSyncTime: %d. SyncTimeScale: %d.\n", f, dBestOffset, pClientInfo->ServerTimeOffset, SyncTimerTick, pClientInfo->SyncTimeScale);
				pClientInfo->ServerTimeOffset = dBestOffset;

				if(AppGetState() == AS_QUERY_SERVER_TIME)
				{
					AppSetState(AS_DETECT_NAT_FIREWALL);
					AddJobDetectNatFirewall();
				}
				return;
			}
		}
		else if(QueryCount == QUERY_SERVER_TIME_COUNT)
		{
			printx("Predict Server Time: %f. Ping: %fms.\n", dClientTime - pClientInfo->ServerTimeOffset, dPing);
			printx("Server Time:         %f. Client Time: %f\n", dServerTime, dClientTime);
			return;
		}

		pJob->type = CT_SERVER_TIME;
		pJob->dword1 = 1; // Prevent re-init in the following code.
	}

	if(!pJob->dword1) // bTest.
	{
		SyncTimerTick = 0;
		pClientInfo->QueryServerTimeCounter = 0;
	}

//	Sleep(1); // Prevent delay that caused by Nagle's algorithm.

	// Send querying packet.
	QueryPerformanceCounter(&TimeInfo.ClientTime);
	memcpy(pJob->Data, &TimeInfo, sizeof(TimeInfo));
	pSocket->SendData(pJob, sizeof(pJob->type) + sizeof(TimeInfo)/* + 1360*/, 0);
}

void JobServerQuery(stJob *pJob, CSocketTCP *pSocket)
{
}

void JobServerRequest(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobServerRequest.\n");
	CStreamBuffer sb;
	CIpAddress ip;
	BYTE buffer[10] = {0};
	UINT DataLen;
	stDriverInternalState DriverState;
	HANDLE hAdapter = OpenAdapter();

	switch(pJob->dword1)
	{
		case SRT_TEST_SOCKET:
			sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));
			sb.SetPos(sizeof(pJob->dword1));
			sb.Read(&ip, sizeof(ip));
			sb >> DataLen;
			pNM->DirectSend(hAdapter, ip, ip.IsIPV6(), NBPort(ip.m_port), sb.GetCurrentBuffer(), DataLen);
			break;

		case SRT_DRIVER_STATE:
			sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));
			sb.Skip(sizeof(pJob->dword1));
			GetDriverState(hAdapter, &DriverState);
			GetUDPInfo(hAdapter, (DWORD&)*buffer, (USHORT&)buffer[4], (BOOL&)buffer[6]);
			sb.Write(&DriverState, sizeof(DriverState));
			sb.Write(buffer, sizeof(buffer));
			pSocket->SendData(pJob, sizeof(pJob->type) + sb.GetDataSize());
			break;

		case SRT_SYSTEM_INFO:
			sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));
			sb.SetPos(sizeof(pJob->dword1));
			CoreGetSystemInfo(pInfo->ClientInternalIP.v4, sb);
			pSocket->SendData(pJob, sizeof(pJob->type) + sb.GetDataSize());
			break;

		case SRT_VERIFY_EXE:
			pJob->dword2 = CoreVerifyEmbeddedSignature(pJob->dword3);
			pSocket->SendData(pJob, sizeof(pJob->type) + sizeof(DWORD) * 3);
			break;
	}

	CloseAdapter(hAdapter);
}

void JobRequestRelay(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobRequestRelay (%d Bytes)\n", pJob->DataLength);

	USHORT SrcDMIndex, DestDMIndex;
	DWORD_PTR ClientID = pInfo->ID1, dwType = pJob->dword1;
	CMapTable *pTable = &pInfo->MapTable;

	if(dwType == RRT_CLIENT_REQUEST)
	{
		if(!pJob->dword3)
		{
			pJob->dword3 = pInfo->ID1;
			pSocket->SendData(pJob, pJob->DataLength);
			return;
		}
		if(pJob->dword4 == ClientID) // Do nothing currently.
		{
		}
		else if(pJob->dword5 == ClientID) // Report valid driver map index if the host role is right.
		{
			if(!pNM->RequestHostRelay(pJob->dword2, pJob->dword3, pJob->dword4, SrcDMIndex, DestDMIndex))
			{
			}
			pJob->dword1 = RRT_RELAY_HOST_ACK;
			pJob->dword6 = (SrcDMIndex << 16) | DestDMIndex;
			pSocket->SendData(pJob, sizeof(pJob->type) + sizeof(DWORD) * 6); // Send result.
		}
	}
	else if(dwType == RRT_RELAY_HOST_ACK)
	{
		SrcDMIndex = pJob->dword6 >> 16;
		DestDMIndex = pJob->dword6 & 0xFFFF;

		if(SrcDMIndex == INVALID_DM_INDEX || DestDMIndex == INVALID_DM_INDEX)
		{
			if(pJob->dword3 == ClientID)
				printx("Relayed tunnel is unavailable!\n");
		}
		else if(pNM->SetRelayHostIndex(pJob->dword2, pJob->dword3, pJob->dword4, pJob->dword5, SrcDMIndex, DestDMIndex))
		{
		//	printx("Prepare relay for host. Src Index: %d Dest Index: %d\n", SrcDMIndex, DestDMIndex);
			stGUIEventMsg *pMsg = AllocGUIEventMsg();

			pMsg->DWORD_1 = pJob->dword5; // Relay host UID.
			pMsg->DWORD_2 = (pJob->dword3 == ClientID) ? pJob->dword4 : pJob->dword3;
			pMsg->DWORD_3 = pJob->dword2; // Relay network ID.
			NotifyGUIMessage(GET_RELAY_EVENT, pMsg);
		}
	}
	else if(dwType == RRT_CANCEL_RELAY)
	{
		stVPNet *pVNet;
		stNetMember *pMember;
		CMemberUpdateList MemberUpdateList;
		HANDLE hAdapter = OpenAdapter();

		if(!pJob->dword3)
		{
			if(pVNet = pNM->FindNet(0, pJob->dword2))
				if(pMember = pVNet->FindHost(pJob->dword4))
					if(pNM->CancelRelay(hAdapter, pTable, pMember, pVNet->NetIDCode, MemberUpdateList))
					{
						pJob->dword3 = pInfo->ID1;
						pSocket->SendData(pJob, pJob->DataLength);
					}
		}
		else if(pJob->dword4 == ClientID) // This host is target host.
		{
			if(pVNet = pNM->FindNet(0, pJob->dword2))
				if(pMember = pVNet->FindHost(pJob->dword3))
					pNM->CancelRelay(hAdapter, pTable, pMember, pVNet->NetIDCode, MemberUpdateList);
		}
		else if(pJob->dword5 == ClientID) // This host is relay host.
		{
		}
		CloseAdapter(hAdapter);

		if(MemberUpdateList.GetDataCount())
		{
			stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
			NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
		}
	}
	else if(dwType == RRT_REQUEST_SERVER)
	{
		ASSERT(!pJob->dword2 && !pJob->dword3);
		printx("RRT_REQUEST_SERVER %d\n", pJob->dword5);

		do
		{
			USHORT DriverMapIndex, LinkState, P2PLinkState;
			if(!pNM->GetUserExistCount(pJob->dword4, &DriverMapIndex, &LinkState, &P2PLinkState))
				break;
		//	printx("Relay dest link state: %d\n", LinkState);
			if(pJob->dword5 == MASTER_SERVER_RELAY_ID) // Request server relay.
			{
				if(LinkState == LS_OFFLINE || LinkState == LS_TRYING_TPT || LinkState == LS_SERVER_RELAYED)
					break;
			}
			else // Cancel server relay.
			{
				if(LinkState != LS_SERVER_RELAYED || P2PLinkState == LS_NO_TUNNEL)
					break;
			}

			pJob->dword3 = AppGetClientInfo()->ID1;
			pSocket->SendData(pJob, pJob->DataLength);
		}
		while(0);
	}
	else if(dwType == RRT_REQUEST_SERVER_ACK)
	{
		printx("RRT_REQUEST_SERVER_ACK\n"); // dwRequestType, dwReason, RequestUID, DestUID, UseOrCancelRelay.

		USHORT P2PLinkState = 0;
		DWORD dwPeerUID = (ClientID == pJob->dword3) ? pJob->dword4 : pJob->dword3;
		ASSERT(ClientID == pJob->dword4 || ClientID == pJob->dword3);

		HANDLE hAdapter = OpenAdapter();
		if(hAdapter != INVALID_HANDLE_VALUE)
		{
			stNatPunchInfo NatPunchInfo;
			if(pJob->dword5 && pJob->dword2) // 0 for user request. 1 for TPT failure.
				if(pNM->CancelPunchHost(dwPeerUID, &NatPunchInfo))
				{
					pTable->UpdateTableFlag(hAdapter, NatPunchInfo.DriverMapIndex, AIF_PT_FIN, AIF_START_PT);
					pTable->UpdateUID(hAdapter, NatPunchInfo.DriverMapIndex, NatPunchInfo.uid);
				}

			if(pTable->UpdateServerRelayFlag(hAdapter, pJob->dword5 ? TRUE : FALSE, dwPeerUID))
			{
				printx("UpdateServerRelayFlag OK! %d\n", pJob->dword5);

				if(pJob->dword5)
				{
					pNM->SwitchRelayMode(hAdapter, dwPeerUID, TRUE);
					pNM->UpdataMemberLinkState(dwPeerUID, LS_SERVER_RELAYED);
				}
				else
					pNM->UpdataMemberLinkState(dwPeerUID, 0, TRUE, 0, &P2PLinkState);

				stGUIEventMsg *pMsg = AllocGUIEventMsg();
				pMsg->DWORD_1 = pJob->dword5;
				pMsg->DWORD_2 = dwPeerUID;
				pMsg->DWORD_3 = MASTER_SERVER_RELAY_ID; // Relay network ID.
				pMsg->DWORD_4 = P2PLinkState;
				NotifyGUIMessage(GET_RELAY_EVENT, pMsg);
			}
			CloseAdapter(hAdapter);
		}
	}
}

void JobKernelDriverEvent(CNetworkManager *pNM, CSocketTCP *pSocket)
{
	stJob job;
	sockaddr_in addr;
	INT DataSize, AddrLen = sizeof(addr);
	SOCKET s = pNM->GetTunnelSocket()->GetSocket();

	DataSize = recvfrom(s, (char*)&job.dword2, sizeof(job.Data) - sizeof(job.dword1), 0, (sockaddr*)&addr, &AddrLen);

	stClientInfo *pInfo = AppGetClientInfo();
	if(addr.sin_addr.S_un.S_addr == pInfo->ClientInternalIP.v4 && NBPort(addr.sin_port) == pInfo->UDPPort) // Check source address.
	{
		stDriverNotifyHeader *pHeader = (stDriverNotifyHeader*)&job.dword2;

		if(pHeader->DMIndex != INVALID_DM_INDEX) // Relay packet.
		{
			job.type = CT_SERVER_RELAY;
			job.dword1 = job.dword3; // Set UID.
			job.dword3 = 0;
			ASSERT(DataSize <= (ETH_MAX_PACKET_SIZE - PACKET_SKIP_SIZE + sizeof(stPacketHeader)));
			pSocket->SendData(&job, sizeof(job.type) + sizeof(job.dword1) + DataSize, TPHF_RELAY);
	//		printx("---> OnIPCRelayEvent. Recv: %d Bytes\n", DataSize);
		}
		else // Driver notify.
		{
			if(pHeader->type == 1)
			{
				LARGE_INTEGER EndTime;
				stPingHostNotifyInfo *pNotifyInfo = (stPingHostNotifyInfo*)pHeader;

			//	memcpy(&EndTime, &pNotifyInfo->liTime, sizeof(EndTime));
				QueryPerformanceCounter(&EndTime);
				stClientInfo *pInfo = AppGetClientInfo();
				UINT time = (UINT)((EndTime.QuadPart - pInfo->PingHostStartTime.QuadPart) * pInfo->InvFreq);
				DWORD dw = MAKELONG(time, job.dword3); // Time, nID
				NotifyGUIMessage(GET_PING_HOST, dw);
			}
		}
	}
}

void JobHandleCtrlPacket(CMapTable *pTable, TUNNEL_SOCKET_TYPE *pSocketUDP, void *pData, UINT len, USHORT DMIndex)
{
//	printx("---> HandleCtrlPacket %d Bytes\n", len);
	stCtrlPacket *pCtrlPacket = (stCtrlPacket*)pData;

	if(pCtrlPacket->CtrlType == 1)
	{
		if(!pCtrlPacket->Reserved)
		{
			stEntry *pEntry = pTable->Entry + DMIndex, *pSourceEntry = pEntry;

			if(pEntry->flag & AIF_RELAY_HOST)
			{
				if(pEntry->rhindex >= pTable->m_Count)
					return;
				pCtrlPacket->PacketHeader.flag1 |= PHF_USE_RELAY;
				pCtrlPacket->PacketHeader.RHDestIndex = pEntry->destindex;
				pCtrlPacket->PacketHeader.SrcHostDestIndex = pEntry->dindex;
				pEntry = pTable->Entry + pEntry->rhindex; // Mod pEntry at last.
			}

			pCtrlPacket->PacketHeader.dindex = pEntry->dindex;
			pCtrlPacket->Reserved = 1; // Set response value.

			if(pSourceEntry->flag & AIF_SERVER_RELAY)
				AddJobServerRelay(pSourceEntry->uid, pCtrlPacket, len); // Let worker thread do the job.
			else
			{
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_addr.S_un.S_addr = pEntry->pip.v4;
				addr.sin_port = pEntry->port;
				pSocketUDP->SendTo(pData, len, (SOCKADDR*)&addr, sizeof(addr), 0);
			}
		}
		else
		{
			LARGE_INTEGER EndTime;
			QueryPerformanceCounter(&EndTime);
			stClientInfo *pInfo = AppGetClientInfo();
			UINT time = (UINT)((EndTime.QuadPart - pInfo->PingHostStartTime.QuadPart) * pInfo->InvFreq);
			DWORD dw = MAKELONG(time, pCtrlPacket->dw1); // Time, nID
			NotifyGUIMessage(GET_PING_HOST, dw);
		}
	}
}

void JobServerRelay(stJob *pJob, CSocketTCP *pSocket, UINT nSize)
{
//	printx("---> JobServerRelay (%s: %d Bytes)\n", IS_RESPONSE(pJob->type) ? "recv" : "send", pJob->DataLength - sizeof(pJob->type) - sizeof(pJob->dword1));

	if(IS_RESPONSE(pJob->type))
	{
		stClientInfo *pClientInfo = AppGetClientInfo();
		CMapTable *pTable = &(pClientInfo->MapTable);
		CNetworkManager *pNM = AppGetNetManager();
		UINT nPacketLen = nSize - sizeof(pJob->type) - sizeof(pJob->dword1);

		USHORT usIndex = *(USHORT*)&pJob->dword2;
		if(usIndex < pTable->m_Count && (pTable->Entry[usIndex].flag & AIF_SERVER_RELAY))
		{
			ASSERT(nPacketLen <= (ETH_MAX_PACKET_SIZE - PACKET_SKIP_SIZE + sizeof(stPacketHeader)));

			if(((stPacketHeader*)&pJob->dword2)->flag1 & PHF_CTRL_PKT)
			{
				JobHandleCtrlPacket(pTable, 0, &pJob->dword2, nPacketLen, usIndex);
				return;
			}

//*
			if(pNM->GetTunnelSocketMode() == CNetworkManager::TSM_USER_MODE)
			{
				pNM->DirectSend(0, pClientInfo->ClientInternalIP, FALSE, NBPort(pClientInfo->UDPPort), &pJob->dword2, nPacketLen);
			}
			else
			{
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_port = NBPort(pClientInfo->UDPPort);
				addr.sin_addr.S_un.S_addr = pClientInfo->ClientInternalIP.v4;

				INT nResult = pNM->GetTunnelSocket()->SendTo(&pJob->dword2, nPacketLen, (SOCKADDR*)&addr, sizeof(addr), 0);
			}
/*/
			DWORD dwWritten;
			stKBuffer<1> KBuffer;
			KBuffer.DataCount = 1;

			memcpy(KBuffer.BufferSlot[0].Buffer, pClientInfo->vmac, sizeof(mac));
			memcpy(KBuffer.BufferSlot[0].Buffer + sizeof(mac), pTable->Entry[usIndex].mac, sizeof(mac));

			KBuffer.BufferSlot[0].Len = nPacketLen + (PACKET_SKIP_SIZE - sizeof(stPacketHeader));
			memcpy(KBuffer.BufferSlot[0].Buffer + sizeof(mac) * 2, pJob->Data + sizeof(pJob->dword1) + sizeof(stPacketHeader), nPacketLen - sizeof(stPacketHeader));

			pTable->DataIn[usIndex] += (nPacketLen + (PACKET_SKIP_SIZE - sizeof(stPacketHeader)));

			HANDLE hAdapter = OpenAdapter();
			if(hAdapter != INVALID_HANDLE_VALUE)
			{
				WriteFile(hAdapter, &KBuffer, sizeof(KBuffer), &dwWritten, 0);
				CloseAdapter(hAdapter);
			}
//*/
		}
	}
	else
		pSocket->SendData(pJob, pJob->DataLength, TPHF_RELAY);
}

void JobDebugFunction(stJob *pJob, CSocketTCP *pSocket)
{
	printx("JobDebugFunction %d %d\n", pJob->type, pJob->DataLength);

	if(IS_RESPONSE(pJob->type))
	{
		return;
	}

	stClientInfo *pInfo = AppGetClientInfo();
	pInfo->bDebugReserveTunnel = TRUE;

	pSocket->SendData(&pJob->type, pJob->DataLength);
}

void JobTextChat(stJob *pJob, CSocketTCP *pSocket)
{
	CStreamBuffer sBuffer;

	if(IS_RESPONSE(pJob->type))
	{
		pJob->type = CT_TEXT_CHAT;
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	DWORD SenderUID, TargetUID;
	TCHAR String[MAX_CHAT_STRING_LENGTH + 1] = {0};
	INT iLen, iBufLen = _countof(String);

	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> SenderUID >> TargetUID >> iLen;
	ASSERT(iLen <= MAX_CHAT_STRING_LENGTH);

	CoreReadString(sBuffer, String, &iBufLen);
	sBuffer.DetachBuffer();

	stNetMember *pMember = AppGetNetManager()->FindNetMember(SenderUID);
	if(pMember == NULL || TargetUID != AppGetClientInfo()->ID1)
		return;

	// Update GUI.
	stGUIEventMsg *pMsg = AllocGUIEventMsg();
	ConvertData(&pMsg->member, pMember);
	pMsg->dwResult = pMember->UserID;
	pMsg->string = String;
	NotifyGUIMessage(GET_ON_RECEIVE_CHAT_TEXT, pMsg);
}

void JobGroupChat(stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobGroupChat: %d\n", pJob->dword1);

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	INT iLen;
	BOOL bNotifyGUI = FALSE;
	CStreamBuffer sb;
	TCHAR tbuf[MAX_GROUP_CHAT_LENGTH + 1];

	switch(pJob->dword1)
	{
		case GCRT_REQUEST:
			if(pJob->dword2 == GCCSC_OK || pJob->dword2 == GCCSC_NOTIFY) // Check result code.
			{
				CnMatrixCore *pCore = AppGetnMatrixCore();
				if(pCore->IsServiceMode() && !pCore->IsPipeConnected())
					AddJobGroupChatLeave(AppGetClientInfo()->ID1, pJob->dword3);
				else
					bNotifyGUI = TRUE;
			}
			break;

		case GCRT_INVITE:
			if(pJob->dword2 == GCCSC_OK || pJob->dword2 == GCCSC_NOTIFY || pJob->dword2 == GCCSC_RESUME)
			{
				CnMatrixCore *pCore = AppGetnMatrixCore();
				if(pCore->IsServiceMode() && !pCore->IsPipeConnected())
					AddJobGroupChatLeave(AppGetClientInfo()->ID1, pJob->dword3);
				else
					bNotifyGUI = TRUE;
			}
			break;

		case GCRT_LEAVE:
			if(/*pJob->dword2 == GCCSC_OK || */pJob->dword2 == GCCSC_NOTIFY)
				bNotifyGUI = TRUE;
			break;

		case GCRT_CLOSE:
			bNotifyGUI = TRUE;
			break;

		case GCRT_EVICT:
			bNotifyGUI = TRUE;
			break;

		case GCRT_CHAT:
			sb.AttachBuffer(pJob->Data, sizeof(pJob->Data)); // Convert UTF8 string back to TCHAR here.
			sb.Skip(sizeof(DWORD) * 5);
			iLen = _countof(tbuf);
			if((iLen = CoreReadString(sb, tbuf, &iLen)) > 0)
			{
				stGUIEventMsg *pMsg = AllocGUIEventMsg();
				BYTE *pAddr = (BYTE*)pMsg->Heap(sizeof(DWORD) * 5 + sizeof(TCHAR) * iLen);
				memcpy(pAddr, pJob->Data, sizeof(DWORD) * 5);
				memcpy(pAddr + sizeof(DWORD) * 5, tbuf, sizeof(TCHAR) * iLen);
				NotifyGUIMessage(GET_GROUP_CHAT, pMsg);
			//	bNotifyGUI = FALSE;
			}
			break;
	}

	if(bNotifyGUI)
	{
		stGUIEventMsg *pMsg = AllocGUIEventMsg();
		if(pMsg)
		{
			pMsg->Heap(pJob->DataLength, &pJob->dword1);
			NotifyGUIMessage(GET_GROUP_CHAT, pMsg);
		}
	}
}

void UpdateVNetLinkState(CNetworkManager *pNM, stVPNet *pVNet, CSocketTCP *pSocket, BOOL *pbActive)
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	CMapTable *pTable = &pClientInfo->MapTable;
	CMemberUpdateList MemberUpdateList;
	CList<stNetMember> *pMemberList = pVNet->GetMemberList();
	stNetMember *pDestMember, *pRelayHost;

	HANDLE hAdapter = OpenAdapter();
	for(POSITION pos = pMemberList->GetHeadPosition(); pos;)
	{
		pDestMember = &pMemberList->GetNext(pos);
		if(!pDestMember->IsOnline() || !pNM->GetRelayHost(pTable, pVNet, pDestMember, &pRelayHost))
			continue;

		if(!NeedConnect(pVNet, pDestMember) || !NeedConnect(pVNet, pRelayHost) || !NeedConnect(pVNet, pDestMember, pRelayHost))
			pNM->CancelRelay(hAdapter, pTable, pDestMember, pVNet->NetIDCode, MemberUpdateList);
	}

	pNM->GroupStateChanged(hAdapter, pVNet, pClientInfo, MemberUpdateList, pbActive);
	CloseAdapter(hAdapter);

	if(pNM->GetHandshakeHostCount()) // Use server to do the handshake.
	{
		stJob job;
		CStreamBuffer sBuffer;
		while(pNM->GetHandshakeHostCount())
		{
			SetHostSendJob(&job, HST_PT_HANDSHAKE, pClientInfo->ID1, &sBuffer);
			pNM->GetHandshakeData(sBuffer, HSF_PT_INFO);
			pSocket->SendData(&job, sizeof(job.type) + sBuffer.GetDataSize());
			sBuffer.DetachBuffer();
		}
	}

	if(MemberUpdateList.GetDataCount())
	{
		stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
		NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
	}
}

void UpdateMemberLinkState(CNetworkManager *pNM, stVPNet *pVNet, stNetMember *pDestMember, CSocketTCP *pSocket, BOOL bActive)
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	CMapTable *pTable = &pClientInfo->MapTable;
	CMemberUpdateList MemberUpdateList;
	stNetMember *pRelayHost;

	HANDLE hAdapter = OpenAdapter();
	if(!pVNet->IsNetOnline() || !pDestMember->IsNetOnline() || !NeedConnect(pVNet, pDestMember))
		if(pDestMember->DriverMapIndex != INVALID_DM_INDEX)
		{
			if(pDestMember->Flag & VF_RELAY_HOST)
				pNM->CancelRelayHost(pVNet, pDestMember, MemberUpdateList);
			if(pNM->GetRelayHost(pTable, pVNet, pDestMember, &pRelayHost))
				pNM->CancelRelay(hAdapter, pTable, pDestMember, pVNet->NetIDCode, MemberUpdateList);
		}

	pNM->UpdateConnectionState(hAdapter, pClientInfo, pVNet, pDestMember, MemberUpdateList, bActive);
	CloseAdapter(hAdapter);

	if(pNM->GetHandshakeHostCount()) // Use server to do the handshake.
	{
		stJob job;
		CStreamBuffer sBuffer;
		SetHostSendJob(&job, HST_PT_HANDSHAKE, pClientInfo->ID1, &sBuffer);
		pNM->GetHandshakeData(sBuffer, HSF_PT_INFO, 1);
		pSocket->SendData(&job, sizeof(job.type) + sBuffer.GetDataSize());
		sBuffer.DetachBuffer();
	}

	if(MemberUpdateList.GetDataCount())
	{
		stGUIEventMsg *pMsg = SetupGUIUpdateData(MemberUpdateList);
		NotifyGUIMessage(GET_UPDATE_MEMBER_STATE, pMsg);
	}
}

void JobSubnetSubgroup(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobSubnetSubgroup\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	stVPNet *pVNet;
	BOOL bNotifyGUI = FALSE, bActive;

	switch(pJob->dword1)
	{
		case SSCT_CREATE:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;

			pVNet->CreateGroup(pJob->dword4, (stVNetGroup*)&pJob->dword6, pJob->dword5);
			bNotifyGUI = TRUE;

			break;

		case SSCT_DELETE:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;

			pVNet->DeleteGroup(pJob->dword4);
			UpdateVNetLinkState(pNM, pVNet, pSocket, 0);
			bNotifyGUI = TRUE;

			break;

		case SSCT_MOVE:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;

			if(pJob->dword4 == pInfo->ID1)
			{
				pVNet->SetLocalGroupIndex(pJob->dword5);
				bActive = TRUE;
				UpdateVNetLinkState(pNM, pVNet, pSocket, &bActive);
			}
			else
			{
				stNetMember *pDest = pVNet->UpdateMemberGroupIndex(pJob->dword4, pJob->dword5);
				if(pDest && pDest->IsOnline())
					UpdateMemberLinkState(pNM, pVNet, pDest, pSocket, FALSE);
			}
			bNotifyGUI = TRUE;

			break;

		case SSCT_OFFLINE:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;
			ASSERT(pVNet->GetGroupCount() >= pJob->dword5 && pVNet->GetGroupCount() > 1); // Server will check this.

			if(pJob->dword4 == pInfo->ID1)
			{
				pVNet->UpdateGroupBitMask(pJob->dword5, pJob->dword6);
				bActive = TRUE;
				UpdateVNetLinkState(pNM, pVNet, pSocket, &bActive);
			}
			else
			{
				stNetMember *pDest = pVNet->FindHost(pJob->dword4);
				if(!pDest)
					break;
				if(pDest->UpdateGroupBitMask(pJob->dword5, pJob->dword6))
					UpdateMemberLinkState(pNM, pVNet, pDest, pSocket, FALSE);
			}
			bNotifyGUI = TRUE;

			break;

		case SSCT_SET_FLAG:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;

			switch(pJob->dword5)
			{
				case VGF_DEFAULT_GROUP:
					pVNet->SetDefaultGroup(pJob->dword4);
					bNotifyGUI = TRUE;
					break;

				case VGF_STATE_OFFLINE:
					pVNet->UpdateGroupFlag(pJob->dword4, pJob->dword5, pJob->dword6);
					bActive = pJob->dword2 == CSRC_SUCCESS;
					UpdateVNetLinkState(pNM, pVNet, pSocket, &bActive);
					bNotifyGUI = TRUE;
					break;

				default:
					pVNet->UpdateGroupFlag(pJob->dword4, pJob->dword5, pJob->dword6);
					bNotifyGUI = TRUE;
					break;
			}

			break;

		case SSCT_RENAME:

			if(pJob->dword2 != CSRC_SUCCESS && pJob->dword2 != CSRC_NOTIFY)
				break;
			if((pVNet = pNM->FindNet(0, pJob->dword3)) == 0)
				break;

			pVNet->UpdateGroupName(pJob->dword4, (TCHAR*)&pJob->dword6, pJob->dword5);
			bNotifyGUI = TRUE;

			break;
	}

	if(bNotifyGUI)
	{
		stGUIEventMsg *pMsg = AllocGUIEventMsg();
		if(pMsg)
		{
			pMsg->Heap(pJob->DataLength - sizeof(pJob->type), &pJob->dword1);
			NotifyGUIMessage(GET_SUBNET_SUBGROUP, pMsg);
		}
	}
}

void JobClientProfile(stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobClientProfile\n");

	if(!IS_RESPONSE(pJob->type))
	{
		pSocket->SendData(pJob, pJob->DataLength);
		return;
	}

	INT iLen;
	stGUIEventMsg *pMsg = NULL;
	TCHAR HostName[MAX_HOST_NAME_LENGTH + 1];
	CNetworkManager *pNM = AppGetNetManager();
	stClientInfo *pInfo = AppGetClientInfo();
	CStreamBuffer sb;
	sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));

	switch(pJob->dword1)
	{
		case CPT_HOST_NAME:

			sb.Skip(sizeof(DWORD) * 3);
			iLen = _countof(HostName);
			iLen = CoreReadString(sb, HostName, &iLen);

			if(pJob->dword2 == 1 || pJob->dword2 == 2) // Notify.
			{
				pNM->UpdateMemberProfile(pJob->dword3, pJob->dword1, HostName, sizeof(TCHAR) * iLen);

				pMsg = AllocGUIEventMsg();
				pJob->dword4 = iLen;
				memcpy(&pJob->dword5, HostName, sizeof(TCHAR) * iLen);
				pMsg->Heap(sizeof(DWORD) * 4 + sizeof(TCHAR) * iLen, pJob->Data);
			}

			break;
	}

	if(pMsg != NULL)
		NotifyGUIMessage(GET_CLIENT_PROFILE, pMsg);
}

void PackLoginInfo(stGUIEventMsg *pMsg) // Must match CVPNClientDlg::ReadLoginData().
{
	CStreamBuffer sb;
	BYTE buffer[1024 * 2];
	sb.AttachBuffer(buffer, sizeof(buffer));
	stClientInfo *pInfo = AppGetClientInfo();
	sb << pInfo->ID1 << pInfo->vip << pInfo->ServerCtrlFlag;
	sb.WriteString(BYTE(0), pInfo->LoginName);
	sb.WriteString(BYTE(0), pInfo->LoginPassword);
	sb.Write(&AppGetClientInfo()->RegID, sizeof(AppGetClientInfo()->RegID));

	pMsg->Heap(sb.GetDataSize(), buffer);

	ASSERT(sb.GetDataSize() <= sizeof(buffer));
	sb.DetachBuffer();
}

void JobDataExchange(stJob *pJob)
{
	CStreamBuffer sb;
	sb.AttachBuffer(pJob->Data, sizeof(pJob->Data));

	stClientInfo *pInfo = AppGetClientInfo();
	DWORD bRead, DataTypeEnum;
	sb >> bRead >> DataTypeEnum;

	if(bRead) // GUI requests to get data of the core.
	{
		stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD1);
		pMsg->dwResult = DataTypeEnum;

		switch(DataTypeEnum)
		{
			case DET_LOGIN_INFO:
				PackLoginInfo(pMsg);
				break;
		}

		NotifyGUIMessage(GET_DATA_EXCHANGE, pMsg);
	}
	else // Change setting.
	{
		switch(DataTypeEnum)
		{
			case DET_REG_ID:
			//	printx("Save RegID!\n");
				sb.Read(&pInfo->RegID, sizeof(pInfo->RegID));
				CoreSaveRegisterID(&pInfo->RegID);
				break;

			case DET_SERVER_MSG_ID:
				sb >> pInfo->dwClosedServerMsgID;
				CoreSaveClosedMsgID(pInfo->dwClosedServerMsgID);
			//	printx("Save Closed Msg ID: %d\n", pInfo->dwClosedServerMsgID);
				break;
		}
	}
}

void JobClientTimer(stClientInfo *pClientInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket, UINT &SyncTimerTick)
{
//	printx("---> JobClientTimer().\n");
	DWORD UIDArray[MAX_NETWORK_CLIENT];
	UINT nCount = 0, nReportCount = 0;

	HANDLE hAdapter = OpenAdapter();
	pNM->PunchOnce(hAdapter); // Must call this first.
	pNM->CheckTable(hAdapter, UIDArray, nCount, nReportCount);
	if(pNM->KTTimerEntry(hAdapter, pClientInfo, &pClientInfo->MapTable))
		AppGetnMatrixCore()->CoreStopTimer();
	CloseAdapter(hAdapter);

	if(!nCount && !nReportCount)
		return;

	stJob job;
	CStreamBuffer sb;
	if(nCount)
	{
		SetHostSendJob(&job, HST_MULTI_SEND, pClientInfo->ID1, &sb);
		sb << nCount << (UINT)4 << (DWORD)HSF_PT_ACK ; // Data size and data.
		sb.Write(UIDArray, sizeof(DWORD) * nCount);

		pSocket->SendData(&job, sizeof(job.type) + sb.GetDataSize());
		sb.DetachBuffer();
		printx("Ack host count: %d.\n", nCount);
	}
	if(nReportCount)
	{
		job.type = CT_CLIENT_REPORT;
		job.dword1 = CRT_TPT_RESULT;
		sb.AttachBuffer(job.Data, sizeof(job.Data));

		while(nReportCount)
		{
			sb.SetPos(sizeof(DWORD));
			pNM->GetReportData(sb);
			pSocket->SendData(&job, sizeof(job.type) + sb.GetDataSize());
			nReportCount = pNM->GetReportCount();
		}
	}
}

void JobReadTrafficInfo(stJob *pJob)
{
//	printx("---> JobReadTrafficInfo().\n");

	CMapTable *pMapTable = &(AppGetClientInfo()->MapTable);
	CNetworkManager *pNetManager = AppGetNetManager();
	USHORT nCount = pMapTable->m_Count;

	if(!nCount)
		return;

	if(pNetManager->GetTunnelSocketMode() == CNetworkManager::TSM_KERNEL_MODE)
	{
		HANDLE hAdapter = OpenAdapter();
		if(hAdapter != INVALID_HANDLE_VALUE)
		{
			ReadTrafficInfo(hAdapter, pMapTable->DataIn, pMapTable->DataOut, nCount);
			CloseAdapter(hAdapter);
		}
	}

	UINT size = sizeof(nCount) + sizeof(UINT) * 2 * nCount;
	stGUIEventMsg *pMsg = AllocGUIEventMsg();
	CStreamBuffer sBuffer;
	sBuffer.AttachBuffer((BYTE*)pMsg->Heap(size), size);

	sBuffer << nCount;
	sBuffer.Write(pMapTable->DataIn, sizeof(UINT) * nCount);
	sBuffer.Write(pMapTable->DataOut, sizeof(UINT) * nCount);

	ASSERT(sBuffer.GetDataSize() == size);
	sBuffer.DetachBuffer();

	// Update GUI.
	NotifyGUIMessage(GET_UPDATE_TRAFFIC_INFO, pMsg);
}

void JobEnableUserModeAccess(CNetworkManager *pNM, BOOL bEnable)
{
	HANDLE hAdapter = OpenAdapter();
	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		if(!pNM->SetTunnelSocketMode(hAdapter, bEnable ? CNetworkManager::TSM_USER_MODE : CNetworkManager::TSM_KERNEL_MODE))
		{
			NotifyGUIMessage(GET_CORE_ERROR, CSEC_TUNNEL_ERROR);
			AddJobLogin(LOR_ERROR, 0, 0, TRUE);
		}
		CloseAdapter(hAdapter);
	}
}

void JobUpdateConfig(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob)
{
	stConfigData *pConfigData;

	if(!pJob)
	{
		pConfigData = &pInfo->ConfigData;
		if(pConfigData->DataOutLimit)
			pInfo->iSendLimit = (UINT)((pConfigData->DataOutLimit + 0.5) * 1024 / BANDWIDTH_UL_TIME_RATE);
		else
			pInfo->iSendLimit = 0;
		pInfo->bSendParamChanged = TRUE;
		if(pConfigData->DataInLimit)
			pInfo->iRecvLimit = (UINT)((pConfigData->DataInLimit + 0.5) * 1024 / BANDWIDTH_DL_TIME_RATE);
		else
			pInfo->iRecvLimit = 0;
		pInfo->bRecvParamChanged = TRUE;
		return;
	}

	BOOL bRead = pJob->dword1;
	pConfigData = (stConfigData*)&pJob->dword2;
//	printx("---> JobUpdateConfig. %d\n", pConfigData->usTunnelUDPPort);
//	printx("Upload: %d. Download: %d.\n", pConfigData->DataOutLimit, pConfigData->DataInLimit);

	if(!bRead)
	{
		if(pInfo->ConfigData.DataOutLimit != pConfigData->DataOutLimit)
		{
			if(pConfigData->DataOutLimit)
				pInfo->iSendLimit = (UINT)((pConfigData->DataOutLimit + 0.5) * 1024 / BANDWIDTH_UL_TIME_RATE);
			else
				pInfo->iSendLimit = 0;
			pInfo->bSendParamChanged = TRUE;
		}
		if(pInfo->ConfigData.DataInLimit != pConfigData->DataInLimit)
		{
			if(pConfigData->DataInLimit)
				pInfo->iRecvLimit = (UINT)((pConfigData->DataInLimit + 0.5) * 1024 / BANDWIDTH_DL_TIME_RATE);
			else
				pInfo->iRecvLimit = 0;
			pInfo->bRecvParamChanged = TRUE;
		}

		JobEnableUserModeAccess(pNM, pConfigData->DataInLimit || pConfigData->DataOutLimit);

		if(AppGetnMatrixCore()->IsServiceMode() && pInfo->ConfigData.bAutoStart && !pConfigData->bAutoStart)
		{
			CoreSaveSvcLoginState(FALSE);
			NotifyGUIMessage(GET_SERVICE_STATE, SSC_DELETE_READY);
		}

		UINT nLen = _tcslen(pConfigData->LocalName), nLen2 = _tcslen(pInfo->ConfigData.LocalName);
		if(nLen != nLen2 || memcmp(pInfo->ConfigData.LocalName, pConfigData->LocalName, sizeof(TCHAR) * nLen))
			if(AppGetState() == AS_READY)
				AddJobUpdateHostName(pInfo->ID1, pConfigData->LocalName, nLen);

	//	printx("Send limit: %d. Recv limit: %d.\n", pInfo->iSendLimit, pInfo->iRecvLimit);
		memcpy(&pInfo->ConfigData, pJob->Data + sizeof(pJob->dword1), sizeof(stConfigData));
		CoreSaveConfigData(&pInfo->ConfigData);

	//	IPV4 v4 = pInfo->ConfigData.UDPTunnelAddress;
	//	if(v4)
	//		printx("Config IP: %d.%d.%d.%d\n", v4.b1, v4.b2, v4.b3, v4.b4);
	}
	else
	{
		stGUIEventMsg *pMsg = AllocGUIEventMsg();
		pMsg->dwResult = DET_CONFIG_DATA;
		pMsg->Heap(sizeof(stConfigData*), &pInfo->ConfigData);
		NotifyGUIMessage(GET_DATA_EXCHANGE, pMsg);
	}
}

void JobPingHost(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket)
{
//	printx("---> JobPingHost.\n");
	CMapTable *pTable = &pInfo->MapTable;
	HANDLE hAdapter;

	if(!IS_RESPONSE(pJob->type))
	{
		USHORT DMIndex, LinkState, P2PLinkState;
		DWORD uid = pJob->dword1;

		if(!pNM->GetUserExistCount(uid, &DMIndex, &LinkState, &P2PLinkState))
			return;
		if(LinkState <= LS_TRYING_TPT || DMIndex >= pTable->m_Count)
			return;
	//	printx("LinkState: %d  P2PLinkStae: %d  %d Bytes\n", LinkState, P2PLinkState, sizeof(stCtrlPacket));

		if(LinkState == LS_SERVER_RELAYED) // Must handle 3 different cases.
		{
			stJob job;
			stCtrlPacket cp;
			stEntry *pEntry = pTable->Entry + DMIndex;
			ZeroMemory(&cp, sizeof(cp));

			cp.PacketHeader.flag1 |= PHF_CTRL_PKT;
			cp.PacketHeader.dindex = pEntry->dindex;
			cp.CtrlType = 1; // Ping packet.
			cp.dw1 = pJob->dword2;

			job.type = CT_SERVER_RELAY;
			job.dword1 = uid;
			memcpy(&job.dword2, &cp, sizeof(cp));

			pSocket->SendData(&job, sizeof(job.type) + sizeof(job.dword1) + sizeof(cp), TPHF_RELAY);
		}
		else // LS_RELAYED_TUNNEL or LS_CONNECTED
		{
			ASSERT(LinkState == LS_RELAYED_TUNNEL || LinkState == LS_CONNECTED);

			hAdapter = OpenAdapter();
			if(hAdapter != INVALID_HANDLE_VALUE)
			{
				stCtrlPacket cp;
				stEntry *pEntry = pTable->Entry + DMIndex;
				ZeroMemory(&cp, sizeof(cp));

				if(LinkState == LS_RELAYED_TUNNEL)
				{
					if(pEntry->rhindex >= pTable->m_Count)
						return;
					cp.PacketHeader.flag1 = PHF_USE_RELAY;
					cp.PacketHeader.RHDestIndex = pEntry->destindex;
					cp.PacketHeader.SrcHostDestIndex = pEntry->dindex;
					pEntry = pTable->Entry + pEntry->rhindex; // Mod pEntry at last.
				}

				cp.PacketHeader.flag1 |= PHF_CTRL_PKT;
				cp.PacketHeader.dindex = pEntry->dindex;
				cp.CtrlType = 1; // Ping packet.
				cp.dw1 = pJob->dword2;

				pNM->DirectSend(hAdapter, pEntry->pip, FALSE, pEntry->port, &cp, sizeof(cp));
				CloseAdapter(hAdapter);
			}
		}

		QueryPerformanceCounter(&pInfo->PingHostStartTime);
		return;
	}
}

void JobGUIPipeEvent(stJob *pJob)
{
	printx("---> JobGUIPipeEvent. %d\n", pJob->dword1);
	// Prevent GUI to retrive net list twice in some condition.	
	AppGetnMatrixCore()->SetGUIFlag(pJob->dword1);
}

void JobDebugTest(stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobDebugTest.\n");

	CHAR buf[4000] = { 0 };
	pSocket->SendData(buf, sizeof(buf));
}

void JobServiceState(stClientInfo *pInfo, stJob *pJob)
{
	DWORD AppState = AppGetState();
//	printx("---> JobServiceState. %d\n", AppState);

	if(pJob->dword1) // Query service version.
		NotifyGUIMessage(GET_SERVICE_VERSION, AppGetVersion());
	else if(AppState == AS_READY)
	{
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_CONNECTING);
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_LOGIN_SUCCESS);
		NotifyGUIMessage(GET_UPDATE_NET_LIST, BuildDataStream(0));
		NotifyGUIMessage(GET_RELAY_INFO, BuildRelayInfo());

		if(pInfo->csServerNews != _T(""))
		{
			stGUIEventMsg *pData = AllocGUIEventMsg(stGUIEventMsg::UF_DWORD4);
			if(pData)
			{
				pData->DWORD_1 = CQT_SERVER_NEWS;
				pData->DWORD_2 = (pInfo->csServerNews.GetLength() + 1) * sizeof(TCHAR); // Size.
				pData->DWORD_3 = pInfo->dwServerMsgID;
				pData->DWORD_4 = pInfo->dwClosedServerMsgID;
				pData->Heap(pData->DWORD_2, pInfo->csServerNews.GetBuffer()); // Null-terminated string.
				NotifyGUIMessage(GET_CLIENT_QUERY, pData);
			}
		}
	}
	else if(AppState != AS_DEFAULT)
	{
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_CONNECTING);
	}
}

void JobSystemPowerEvent(stClientInfo *pInfo, stJob *pJob)
{
	printx("---> JobSystemPowerEvent: %d\n", pJob->dword1);

	static BOOL bNeedLogin = FALSE;
	BOOL bResume = pJob->dword1;

	if(bResume)
	{
		pInfo->dwAdapterWaitTime = 50;
		AddJobLogin(0, 0, 0, 0);
	}
	else
	{
		// System is going to suspend.
		bNeedLogin = (AppGetState() != AS_DEFAULT) ? TRUE : FALSE;
		if(bNeedLogin)
			AddJobLogin(LOR_SYSTEM_POWER_EVENT, 0, 0, TRUE);
	}
}

void JobSocketConnectResult(stClientInfo *pInfo, stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobOnSocketConnected. %d\n", pJob->dword2);

	if(AppGetState() != AS_CONNECTING || pInfo->CID != pJob->dword2) // Has aborted.
	{
		ASSERT(AppGetState() == AS_DEFAULT);
		return;
	}
	AppSetState(AS_DEFAULT);

	if(pJob->dword1) // 0. Success. 1. connect() failed. 2. Socket error. 3. Time out.
	{
		pSocket->CloseSocket();
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_CONNECT_FAILED);
		return;
	}

	stJob Job;
	SOCKADDR_IN addr;
	INT addrsize = sizeof(addr);
	pSocket->GetSockName((SOCKADDR*)&addr, &addrsize);
	pInfo->ClientInternalIP.v4 = addr.sin_addr.S_un.S_addr;
	pInfo->ClientInternalIP.SetIPV6(FALSE);
	pInfo->ClientInternalIP.m_port = 0; // Init.

	IPV4 v4 = addr.sin_addr.S_un.S_addr;
	printx("Local address: %d.%d.%d.%d:%d\n", v4.b1, v4.b2, v4.b3, v4.b4, ntohs(addr.sin_port));

	addrsize = sizeof(addr);
	pSocket->GetPeerName((SOCKADDR*)&addr, &addrsize);
	pInfo->ServerIP.v4 = addr.sin_addr.S_un.S_addr;
	pInfo->ServerIP.SetIPV6(FALSE);
	pInfo->ServerIP.m_port = ntohs(addr.sin_port);

	pSocket->ResetDataBuffer();
	pSocket->InitAesData("SERVER_BUILD_20111015_L64AN8ZX9Q");
	if(pInfo->bUseServerSocketEvent)
		EnableServerSocketEvent(pSocket->GetSocket(), TRUE);
	else
		CEventManager::GetEventManager()->AddSocket(pSocket, TRUE);
//		CSocketManager::GetSocketManager()->AddSocket(pSocket); //pSocket->EnableNotifyThread(!pInfo->bUseServerSocketEvent);

	if(pInfo->RegID.IsNull() && !pInfo->LoginName[0])
	{
		AddJobRegister(0, 0);
	}
	else
	{
		Job.type = CT_LOGIN;
		AppGetJobQueue()->AddJob(&Job, 0);
	}
}

DWORD WINAPI ConnectThread(LPVOID lpParameter) // Use connecting thread to control how long we stop the operation if time out.
{
	CSocketTCP *pSocket = (CSocketTCP*)lpParameter; // Don't close the socket directly in this thread.
	stClientInfo *pInfo = AppGetClientInfo();

	TCHAR ServerIP[256];
	BOOL bConnected = FALSE;
	CProfiler profiler; // Get server ip and port from profile.
	profiler.Attach(_T("config.ini"));
	profiler.ReadString(_T("server"), _T("ip"), DEFAULT_SERVER_IP, ServerIP, sizeof(ServerIP));
	USHORT port = profiler.ReadInt(_T("server"), _T("port"), 7500);
	INT ErrorCode, iLen, iResultCode = 1; // iResultCode: 0. Success. 1. connect() failed. 2. Socket error. 3. Time out.

	DWORD ctime = GetTickCount(), dwWaitTime = 100, dwCID = pInfo->CID;
	if(pInfo->dwNextAutoConnectTime > ctime && (pInfo->dwNextAutoConnectTime - ctime < RE_CONNECT_WAIT_TIME))
		do
		{
			Sleep(dwWaitTime);
			ctime = GetTickCount();
		}
		while(pInfo->dwNextAutoConnectTime > ctime && AppGetState() == AS_CONNECTING);

	pSocket->SetNonBlockingMode(TRUE);

	do
	{
		if(pSocket->ConnectEx(ServerIP, port)) // If no error occurs, connect returns zero.
		{
			if((ErrorCode = WSAGetLastError()) != WSAEWOULDBLOCK) // Non-Block mode.
			{
				ASSERT(ErrorCode != WSAEISCONN);
				printx("Socket connect() failed! Error code: %d.\n", ErrorCode);
				iResultCode = 1;
				continue;
			}

			for(;;)
			{
				if(AppGetState() != AS_CONNECTING) // Aborted.
					goto End;
				ErrorCode = pSocket->IsConnected(dwWaitTime);
				if(ErrorCode == TSCS_ERROR) // Socket error.
				{
					iLen = sizeof(iResultCode);
					getsockopt(pSocket->GetSocket(), SOL_SOCKET, SO_ERROR, (CHAR*)&iResultCode, &iLen); // Get failure reason.
					printx("select() failed! Error code: %d.\n", iResultCode);
					iResultCode = 2;
					break;
				}
				else if(ErrorCode == TSCS_CONNECTED) // Connected.
				{
					iResultCode = 0;
					bConnected = TRUE;
					pSocket->SetNonBlockingMode(FALSE);
					break;
				}
			}
		}
		else
		{
			iResultCode = 0;
			bConnected = TRUE;
		}
	}
	while(AppGetState() == AS_CONNECTING && !bConnected);

	stJob Job;
	Job.type = CT_SOCKET_CONNECT_RESULT;
	Job.dword1 = iResultCode;
	Job.dword2 = dwCID;
	AppGetJobQueue()->AddJob(&Job, sizeof(Job.dword1) + sizeof(Job.dword2));

	pInfo->dwNextAutoConnectTime = GetTickCount() + RE_CONNECT_WAIT_TIME;

End:

	while(!pInfo->hThreadHandle)
		Sleep(1);
	CloseHandle(pInfo->hThreadHandle);
	pInfo->hThreadHandle = 0;

	return 0;
}

void JobLoginEvent(stJob *pJob, CSocketTCP *pSocket)
{
	printx("---> JobLoginEvent().\n");

	UINT nLogoutReason;
	BOOL bCloseTunnel;
	USHORT usLen;
	CStreamBuffer sBuffer;
	stClientInfo *pInfo = AppGetClientInfo();
	CoreLoadRegisterID(&pInfo->RegID);

	sBuffer.AttachBuffer(pJob->Data, sizeof(pJob->Data));
	sBuffer >> nLogoutReason >> bCloseTunnel;
	sBuffer.ReadString(usLen, _countof(pInfo->LoginName) - 1, pInfo->LoginName, sizeof(pInfo->LoginName));
	sBuffer.ReadString(usLen, _countof(pInfo->LoginPassword) - 1, pInfo->LoginPassword, sizeof(pInfo->LoginPassword));
	sBuffer.DetachBuffer();

	if(nLogoutReason)
	{
		AppSetState(AS_DEFAULT);
		while(pInfo->hThreadHandle) // Wait connection thread to exit.
			Sleep(1);

		DWORD dwNotifyCode;
		switch(nLogoutReason)
		{
			case LOR_GUI:
			case LOR_SYSTEM_POWER_EVENT:
				dwNotifyCode = (AppGetState() == AS_READY) ? LRC_LOGOUT_SUCCESS : LRC_LOGOUT_NOT_LOGIN;
				break;
			case LOR_DISCONNECTED:
				dwNotifyCode = LRC_LOGOUT_DISCONNECT;
				break;
			case LOR_QUERY_ADDRESS_FAILED:
				dwNotifyCode = LRC_QUERY_ADDRESS_FAILED;
				break;
			case LOR_ERROR:
				dwNotifyCode = LRC_LOGOUT_CORE_ERROR;
				break;
		}

		EnableServerSocketEvent(pSocket->GetSocket(), FALSE);
		// This class provide simple timer function. Must remove socket before closing it or cause infinite loop.
		CEventManager::GetEventManager()->RemoveSocket(pSocket->GetSocket(), TRUE);
		pSocket->CloseSocket();

		CNetworkManager *pNM = AppGetNetManager();
		HANDLE hAdapter = OpenAdapter();
		if(pInfo->bDebugReserveTunnel)
		{
			pInfo->bDebugReserveTunnel = FALSE;
			bCloseTunnel = FALSE;
		}
		if(bCloseTunnel || !pInfo->ConfigData.KeepTunnelTime)
		{
			printx("Close tunnel!\n");
			if(hAdapter != INVALID_HANDLE_VALUE)
				pNM->CloseTunnelSocket(hAdapter);
			AppGetnMatrixCore()->CoreStopTimer();
			bCloseTunnel = TRUE;
		}
		else
		{
			printx("Tunnel reserved! (%d sec)\n", 60 * pInfo->ConfigData.KeepTunnelTime);
			if(hAdapter != INVALID_HANDLE_VALUE)
				pNM->KTMarkPending(hAdapter, 0, pInfo->ConfigData.KeepTunnelTime);
		}
		pNM->Release(bCloseTunnel);
		CloseHandle(hAdapter);
		pInfo->CID++; // Change connection ID for next connecting.
		pInfo->SetDefault();

		NotifyGUIMessage(GET_LOGIN_EVENT, dwNotifyCode);

		if(AppGetnMatrixCore()->IsServiceMode() && nLogoutReason == LOR_GUI)
			CoreSaveSvcLoginState(FALSE);
		if(nLogoutReason == LOR_DISCONNECTED && /*pInfo->ConfigData.bAutoReconnect &&*/ !pInfo->bLastLoginError) // 20130514 Always re-connect when lost connection.
			Sleep(50); // Let driver has time to close socket.
		else
			return;
	}

	if(AppGetState() != AS_DEFAULT)
		return;

	HANDLE hAdapter = OpenAdapter();
	for(; pInfo->dwAdapterWaitTime; pInfo->dwAdapterWaitTime--) // Must wait some time when system resumed.
		if(hAdapter == INVALID_HANDLE_VALUE)
		{
			Sleep(100);
			hAdapter = OpenAdapter();
		}
	if(hAdapter == INVALID_HANDLE_VALUE)
	{
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_ADAPTER_ERROR);
		return; // No need to close handle in this case.
	}
	CloseAdapter(hAdapter);
	pInfo->dwAdapterWaitTime = 0;

	pInfo->bLastLoginError = FALSE;
	AppSetState(AS_CONNECTING);

	// Test code.
	pInfo->bUseServerSocketEvent = (GetKeyState(VK_RBUTTON) < 0) ? TRUE : FALSE; // Using thread can improve the performance of the server relay.

	pSocket->CreateEx();

	// Set keep alive before connecting. Need include Mstcpip.h. Keyword: SIO_KEEPALIVE_VALS Control Code.
	tcp_keepalive inKeepAlive = {0}, outKeepAlive = {0};
	DWORD dwBytesReturned = 0, size = sizeof(tcp_keepalive);
	inKeepAlive.onoff = 1;
	inKeepAlive.keepaliveinterval = 5000;
	inKeepAlive.keepalivetime = 5000;

	if(WSAIoctl(pSocket->GetSocket(), SIO_KEEPALIVE_VALS, &inKeepAlive, size, &outKeepAlive, size, &dwBytesReturned, 0, 0) == SOCKET_ERROR)
		printx("Failed to set keep alive parameters.\n");
//	DWORD bEnable = 1;
//	m_Socket.SetSockOpt(SO_KEEPALIVE, &bEnable, sizeof(bEnable), SOL_SOCKET);

	BOOL Opt = 1;
	if(setsockopt(pSocket->GetSocket(), IPPROTO_TCP, TCP_NODELAY, (CHAR*)&Opt, sizeof(Opt)) == SOCKET_ERROR)
		printx("Error! Set TCP_NODELAY failed! ec: %d\n", WSAGetLastError());

	pInfo->hThreadHandle = CreateThread(0, 0, ConnectThread, pSocket, 0, 0);
	if(!pInfo->hThreadHandle)
	{
		AppSetState(AS_DEFAULT);
		NotifyGUIMessage(GET_LOGIN_EVENT, LRC_THREAD_FAILED);
	}
	NotifyGUIMessage(GET_LOGIN_EVENT, LRC_CONNECTING);

	if(AppGetnMatrixCore()->IsServiceMode())
		CoreSaveSvcLoginState(TRUE);
}


