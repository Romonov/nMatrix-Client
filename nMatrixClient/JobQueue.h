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


#define SET_RESPONSE(c) ((c) |= CT_RESPONSE)
#define IS_RESPONSE(c) ((c) & CT_RESPONSE)
#define GET_TYPE(c) ((c) & ~CT_RESPONSE)


#define DEFAULT_TIMER_PERIOD  1000
#define UPDATE_TRAFFIC_PERIOD 500

#define QUERY_ADDRESS_COUNT  10
#define QUERY_ADDRESS_PERIOD 1000


#define RE_CONNECT_WAIT_TIME 5000


#define MIN_TIMER_RESERVED_TIME 30
#define MAX_TIMER_RESERVED_TIME 60


#define QUERY_SERVER_TIME_COUNT 6


enum COMMAND_TYPE
{
	CT_REG = 1,
	CT_LOGIN,

	// Network.
	CT_DETECT_NAT_FIREWALL,
	CT_QUERY_EXTERNEL_ADDRESS,

	CT_RETRIEVE_NET_LIST,

	CT_CREATE_SUBNET,
	CT_JOIN_SUBNET,
	CT_EXIT_SUBNET,
	CT_REMOVE_USER,
	CT_OFFLINE_SUBNET,
	CT_DELETE_SUBNET,

	CT_TEXT_CHAT,
	CT_SERVER_NOTIFY,
	CT_HOST_SEND,
	CT_SETTING,

	// 2012-09-05
	CT_CLIENT_QUERY,
	CT_CLIENT_REPORT,
	CT_SERVER_TIME,
	CT_SERVER_QUERY,
	CT_SERVER_REQUEST,
	CT_REQUEST_RELAY,
	CT_SERVER_RELAY,  // If add new type behind this, must force user to update client app.
	CT_DEBUG_FUNCTION,

	CT_GROUP_CHAT,
	CT_SUBNET_SUBGROUP,
	CT_CLIENT_PROFILE,


	// Client job only.
	CT_SERVICE_STATE = 50,
	CT_SYSTEM_POWER_EVENT,
	CT_SOCKET_CONNECT_RESULT,
	CT_DATA_EXCHANGE,     // Data exchange between core and gui.
	CT_READ_TRAFFIC_INFO,
	CT_EVENT_LOGIN,
	CT_ENABLE_UM_READ,
	CT_CONFIG,
	CT_PING_HOST,
	CT_GUI_PIPE_EVENT,

	CT_DEBUG_TEST,

	CT_RESPONSE = 0x01 << 31

};


enum SERVER_NOTIFY_FUNC
{
	SNF_RESERVED = 0,
	SNF_OFFLINE_NET,
	SNF_USER_ONLINE, // Online & off-line state.
	SNF_UPDATE_ADDRESS,
	SNF_MSG,
};

enum HOST_SEND_TYPE
{
	HST_MULTI_SEND,   // Send same data to multiple host.
	HST_MULTI_TARGET, // Send different data to different host.
	HST_PT_HANDSHAKE, // Exchange index and key info to setup the tunnel.
	HST_BROAD_CAST,   // Send to member host of the virtual lan.
};

enum HOST_SEND_FUNCTION
{
	HSF_PT_ACK  = 1, // Punch through ack.
	HSF_PT_INFO = 2, // Punch through info.
};

enum SERVER_REQUEST_TYPE
{
	SRT_TEST_SOCKET = 10,
	SRT_DRIVER_STATE,
	SRT_SYSTEM_INFO,
	SRT_VERIFY_EXE,
};


struct stJob
{
	DWORD type;

	union
	{
		struct
		{
			DWORD dword1;
			DWORD dword2;
			DWORD dword3;
			DWORD dword4;
			DWORD dword5;
			DWORD dword6;
			DWORD dword7;
			DWORD dword8;
			DWORD dword9;
		};
		BYTE  Data[MAX_CMD_PACKET_SIZE];
	};

	UINT DataLength; // Including the first data 'type' of the structure.
};


class CJobQueue
{
public:

	CJobQueue()
	:m_hEvent(NULL), m_bStop(FALSE)
	{
		ASSERT(pJobQueue == NULL);
		pJobQueue = this;
	}
	~CJobQueue()
	{
		ASSERT(pJobQueue == NULL || pJobQueue == this);
		ASSERT(!GetJobCount() && m_bStop);
		pJobQueue = NULL;
		ReleaseJobSlot();
	}


	void AddJob(stJob *pJobIn, UINT DataSize, BOOL bPriority = FALSE, BOOL bDynaBuff = FALSE); // Data size doesn't include job type.
	stJob* GetJob();
	void Stop() { m_bStop = TRUE; SetEvent(m_hEvent); }
	void SetEventHandle(HANDLE hEvent) { ASSERT(!m_hEvent);	m_hEvent = hEvent; }
	UINT GetJobCount() { return m_List.GetCount() + m_PriorityList.GetCount(); }


protected:

	HANDLE m_hEvent;
	BOOL   m_bStop;
	CList<stJob*, stJob*> m_List;
	CList<stJob*, stJob*> m_PriorityList; // If jobs need to be processed in order, must use this carefully.
	CList<stJob*, stJob*> m_FreeSlotList;

	CCriticalSectionUTX m_cs, m_csJobSlot;


	stJob* AllocateJob()
	{
		stJob *pJob = NULL;

		m_csJobSlot.EnterCriticalSection();
		if(m_FreeSlotList.GetCount())
			pJob = m_FreeSlotList.RemoveHead();
		m_csJobSlot.LeaveCriticalSection();

		if (pJob == NULL)
			pJob = new stJob;

		return pJob;
	}
	void DeleteJob(stJob *pJob)
	{
		ASSERT(pJob);
		m_csJobSlot.EnterCriticalSection();
		m_FreeSlotList.AddHead(pJob);
		m_csJobSlot.LeaveCriticalSection();
	}


	friend void CnMatrixCore::Run();
	friend CJobQueue* AppGetJobQueue();
	friend void AddJobServerRelay(DWORD uid, void *pData, UINT nDataLen); // For core only.

	static CJobQueue* pJobQueue;

	void ReleaseJobSlot()
	{
		m_csJobSlot.EnterCriticalSection();
		if(m_FreeSlotList.GetCount())
		{
			for(POSITION pos = m_FreeSlotList.GetHeadPosition(); pos;)
				delete m_FreeSlotList.GetNext(pos);
			m_FreeSlotList.RemoveAll();
		}
		m_csJobSlot.LeaveCriticalSection();
	}


};


inline CJobQueue* AppGetJobQueue()
{
	return CJobQueue::pJobQueue;
}


__forceinline void SetHostSendJob(stJob *pJob, DWORD type, DWORD SenderUID, CStreamBuffer *pBuffer)
{
	pJob->type = CT_HOST_SEND;
	pJob->dword1 = type;
	pJob->dword2 = SenderUID;

	if(pBuffer)
	{
		pBuffer->AttachBuffer(pJob->Data, sizeof(pJob->Data));
		pBuffer->SetPos(sizeof(pJob->dword1) + sizeof(pJob->dword2));
	}
}


void EnableServerSocketEvent(SOCKET s, BOOL bEnable, HANDLE ServerSocketEvent = 0);
void SetupWaitableTimer();


void JobServiceState(stClientInfo *pInfo, stJob *pJob);
void JobSystemPowerEvent(stClientInfo *pInfo, stJob *pJob);
void JobSocketConnectResult(stClientInfo *pInfo, stJob *pJob, CSocketTCP *pSocket);
void JobRegister(stJob *pJob, CSocketTCP *pSocket);
void JobLogin(stClientInfo *pClientInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobLoginEvent(stJob *pJob, CSocketTCP *pSocket);
void JobDetectNatFirewallType(stJob *pJob);
void JobQueryExternelAddress(stJob *pJob);
void JobRetrieveNetList(stJob *pJob, CSocketTCP *pSocket);

void JobCreateSubnet(stJob *pJob, CSocketTCP *pSocket);
void JobJoinSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobExitSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobRemoveUser(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);

void JobOfflineSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobDeleteSubnet(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobHostSend(stJob *pJob);
void JobSetting(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobClientQuery(stClientInfo *pInfo, stJob *pJob, CSocketTCP *pSocket);
void JobClientReport(stJob *pJob, CSocketTCP *pSocket);
void JobQueryServerTime(stClientInfo *pClientInfo, stJob *pJob, CSocketTCP *pSocket, UINT &SyncTimerTick);
void JobServerQuery(stJob *pJob, CSocketTCP *pSocket);
void JobServerRequest(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobRequestRelay(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobKernelDriverEvent(CNetworkManager *pNM, CSocketTCP *pSocket);
void JobHandleCtrlPacket(CMapTable *pTable, TUNNEL_SOCKET_TYPE *pSocketUDP, void *pData, UINT len, USHORT DMIndex);
void JobServerRelay(stJob *pJob, CSocketTCP *pSocket, UINT nSize = 0);
void JobDebugFunction(stJob *pJob, CSocketTCP *pSocket);

void JobTextChat(stJob *pJob, CSocketTCP *pSocket);
void JobGroupChat(stJob *pJob, CSocketTCP *pSocket);
void JobSubnetSubgroup(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobClientProfile(stJob *pJob, CSocketTCP *pSocket);
BOOL JobServerNotify(stJob *pJob, CSocketTCP *pSocket);
//void JobHostRequest(stJob *pJob, CSocketTCP *pSocket);

void JobDataExchange(stJob *pJob);
void JobClientTimer(stClientInfo *pClientInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket, UINT &SyncTimerTick);
void JobReadTrafficInfo(stJob *pJob);
void JobEnableUserModeAccess(CNetworkManager *pNM, BOOL bEnable);
void JobUpdateConfig(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob);
void JobPingHost(stClientInfo *pInfo, CNetworkManager *pNM, stJob *pJob, CSocketTCP *pSocket);
void JobGUIPipeEvent(stJob *pJob);
void JobDebugTest(stJob *pJob, CSocketTCP *pSocket);


