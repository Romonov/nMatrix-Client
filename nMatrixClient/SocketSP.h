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


#include "aes.h"
#include "NetworkDataType.h"
#include "UTX.h"


#define MAX_CMD_PACKET_SIZE (1414 + 10) // Padding.
#define CMD_PACKET_SYMBOL 'TENV'

#define TUNNEL_SOCKET_TYPE CSocketBase


typedef void (*SocketCallback)(BYTE *pData, UINT size, void *pContext);
typedef void (*UDPSocketCallback)(BYTE *pData, UINT size, DWORD ip, USHORT port, void *pContext);
typedef void (*SocketCloseCallback)(DWORD dwError, void *pContext);


struct sCmdHeader
{
	DWORD  symbol;
	USHORT datasize;
	BYTE   padding, flag;
};


enum TCP_SOCKET_CONNECTION_STATE
{
	TSCS_ERROR = -1,
	TSCS_CONNECTED,
	TSCS_CONNECTING,
	TSCS_WAIT = TSCS_CONNECTING,
};

enum TCP_PACKET_HEADER_FLAG
{
	TPHF_ENCRYPTED = 0x01,
	TPHF_PRIORITY  = 0x01 << 1,
	TPHF_RELAY     = 0x01 << 2,
};


class CPacketQueue
{
public:

	CPacketQueue()
	{
		m_pHeadNode = NULL;
		m_pTailNode = NULL;
		m_NodeCount = 0;
	}
	~CPacketQueue()
	{
		Release();
	}

	void AddPacket(BYTE *pData, UINT DataSize)
	{
		ASSERT(DataSize);
		sPacketBuffer *pNode = AllocNode();

		pNode->pNext = NULL;
		memcpy(pNode->data, pData, DataSize);
		pNode->len = DataSize;

		// Insert into tail node.
		if(m_pTailNode != NULL)
		{
			m_pTailNode->pNext = pNode;
			m_pTailNode = pNode;
		}
		else
		{
			m_pHeadNode = m_pTailNode = pNode;
		}
		m_NodeCount++;
	}

	BYTE* GetHeadData(UINT *len)
	{
		if(m_pHeadNode != NULL)
		{
			*len = m_pHeadNode->len;
			return m_pHeadNode->data;
		}
		return NULL;
	}

	UINT RemoveHeadData()
	{
		if(m_pHeadNode != NULL)
		{
			sPacketBuffer *pFree = m_pHeadNode;
			m_pHeadNode = m_pHeadNode->pNext;
			if(m_pHeadNode == NULL)
				m_pTailNode = NULL;

			DelNode(pFree);
			--m_NodeCount;
		}

		return m_NodeCount;
	}

	void Release()
	{
		sPacketBuffer *pTemp = m_pHeadNode;
		while(pTemp != NULL)
		{
			pTemp = m_pHeadNode->pNext;
			DelNode(m_pHeadNode);
			--m_NodeCount;
			m_pHeadNode = pTemp;
		}
		ASSERT(m_pHeadNode == NULL && m_NodeCount == 0);
		m_pTailNode = NULL;
	}

	UINT GetCount() { return m_NodeCount; }


protected:

	struct sPacketBuffer
	{
		BYTE data[1500];
		UINT len;
		sPacketBuffer *pNext;
	};

	sPacketBuffer* AllocNode() { return (sPacketBuffer*)malloc(sizeof(sPacketBuffer)); }
	void DelNode(sPacketBuffer *pNode) { free(pNode); }

	sPacketBuffer *m_pHeadNode, *m_pTailNode;
	UINT m_NodeCount/*, m_CacheCount, m_MaxCacheCount*/;


};


enum
{
	OP_RESERVED,
	OP_ACCEPT,
	OP_CREATE,
	OP_CLOSE,
	OP_RECV,
	OP_SEND,
	OP_END_THREAD,
	OP_CUSTOM,

	FLAG_SYS_BUF = 1,
};


class CSocketBase;

struct stSocketBufferBase
{
	USHORT Op, Flag;
	CSocketBase *pSocket;
	WSAOVERLAPPED wo;

	sockaddr_in addr;
	INT addrlen;

	CHAR *pBuffer;
	UINT nOffset, nbuflen;

};


class CSocketBase
{
public:

	CSocketBase()
	{
		m_fd = INVALID_SOCKET;
		m_RefCount = 0;
		m_flag = 0;
		m_pReservedData = NULL;
	}
	virtual ~CSocketBase()
	{
		ASSERT(!m_RefCount);
	}

	enum SOCKET_FLAG
	{
		SF_IPV4       = 0x01,
		SF_IPV6       = 0x01 << 1,
		SF_TCP_SOCKET = 0x01 << 2,
		SF_UDP_SOCKET = 0x01 << 3,
		SF_LISTENING  = 0x01 << 4,

		SF_OVERLAPPED = 0x01 << 5,
		SF_IOCP_EVENT = 0x01 << 6,

		SF_SHUTDOWN_SEND = 0x01 << 7,
		SF_SHUTDOWN_RECV = 0x01 << 8,
		SF_SOCKET_CLOSED = 0x01 << 9,

		SF_CLOSE_NOTIFIED_BIT = 31,  // Once this flag is set, any new async op will fail and OnClose will be call only when all async op calls complete.
	};


	BOOL Accept(CSocketBase *pSocket, struct sockaddr *addr = NULL, INT *addrlen = NULL);
	BOOL Create(UINT nProtocol = SF_IPV4 | SF_TCP_SOCKET, BOOL bOverlapped = TRUE);
	BOOL Connect(const SOCKADDR* lpSockAddr, INT nSockAddrLen) { return connect(m_fd, lpSockAddr, nSockAddrLen); }
	INT  CloseSocket();
	BOOL SafeCloseSocket(DWORD dwWaitTime = INFINITE);
	INT  ShutDown(INT how = SD_BOTH);
	BOOL AsyncSend(stSocketBufferBase *pSocketBuffer, UINT nSize);
	BOOL IssueRecv(stSocketBufferBase *pSocketBuffer);
	BOOL IssueRecvFrom(stSocketBufferBase *pSocketBuffer);
	INT  IsConnected(DWORD dwWaitTime);

	BOOL CreateEx(UINT nProtocol = SF_IPV4 | SF_TCP_SOCKET, DWORD ip = 0, USHORT port = 0);
	BOOL ConnectEx(LPCTSTR lpszHostAddress, UINT nHostPort);
	BOOL GetSockNameEx(CIpAddress &addr);

	BOOL SetNonBlockingMode(DWORD dwMode) { return !ioctlsocket(m_fd, FIONBIO, &dwMode); } // Non-zero to enable nonblocking mode. The ioctlsocket returns zero if succeeded.


	SOCKET GetSocket() { return m_fd; }
	BOOL   IsValidSocket() { return m_fd != INVALID_SOCKET; }
	void*  GetReservedData() { return m_pReservedData; }
	DWORD  GetFlag() { return m_flag; }
	volatile LONG* GetFlagAddress() { return (LONG*)&m_flag; }

	INT Bind(const struct sockaddr *name, INT namelen) { return bind(m_fd, name, namelen); }
	INT Listen(INT backlog = SOMAXCONN) { return listen(m_fd, backlog); }
	INT GetSockName(struct sockaddr *name, INT *namelen) { return getsockname(m_fd, name, namelen); }
	INT GetPeerName(struct sockaddr *name, INT *namelen) { return getpeername(m_fd, name, namelen);	}
	INT Recv(char *buf, INT len, INT flags = 0)	{ return recv(m_fd, buf, len, flags); }
	INT Send(const char *buf, INT len, INT flags = 0) {	return send(m_fd, buf, len, flags);	}
	INT SendTo(const void* lpBuf, INT nBufLen, const SOCKADDR* lpSockAddr, INT nSockAddrLen, INT nFlags = 0) { return sendto(m_fd, (CHAR*)lpBuf, nBufLen, nFlags, lpSockAddr, nSockAddrLen); }

	virtual void OnClose(int nErrorCode, void *pAssocData) {}
	virtual BOOL OnReceive() { return TRUE; }
	virtual BOOL OnReceive(stSocketBufferBase *pBuffer, INT iLen, void *pAssocData) { return TRUE; }
	virtual void ReturnBuffer(stSocketBufferBase *pBuffer);
	virtual void OnIncRef(INT Op, LONG NewRef) {}
	virtual void OnDecRef(INT Op, LONG NewRef) {}

	LONG IncRef(INT op)
	{
		LONG l = InterlockedIncrement(&m_RefCount);
		OnIncRef(op, l);
		return l;
	}
	LONG DecRef(INT op)
	{
		HANDLE hEvent = m_pReservedData;
		LONG l = InterlockedDecrement(&m_RefCount);
		ASSERT(l >= 0);
		OnDecRef(op, l); // Don't access member data after calling this function.
		if(!l && hEvent)
			SetEvent(hEvent);
		return l;
	}
	LONG GetRef() { return m_RefCount; }


private:

	friend class CSocketManager;
	friend DWORD WINAPI SocketIOCPThread(LPVOID lpParameter);

	volatile SOCKET m_fd;
	volatile LONG m_RefCount;
	DWORD m_flag;
	void *m_pReservedData;


};


class CSocketTCP : public CSocketBase
{
public:

	CSocketTCP()
	:m_DataSize(0)
	{
	}
	virtual ~CSocketTCP()
	{
	}


	void SendData(void *pData, UINT size, BYTE byHeaderFlag = TPHF_ENCRYPTED); // TPHF_RELAY
	void ResetDataBuffer() { m_DataSize = 0; }

	BOOL InitAesData(CHAR *pKey);
	BOOL DecryptData(void *pIn, void *pOut, UINT size);

	virtual void OnClose(int nErrorCode, void *pAssocData);
	virtual BOOL OnReceive();

	void SetCallback(SocketCloseCallback CloseCallback, SocketCallback ReceiveCallback)
	{
		m_CloseCallback = CloseCallback;
		m_ReceiveCallback = ReceiveCallback;
	}


protected:

	UINT m_DataSize;
	BYTE m_DataBuffer[(sizeof(sCmdHeader) + MAX_CMD_PACKET_SIZE) * 2];

	SocketCloseCallback m_CloseCallback;
	SocketCallback m_ReceiveCallback;

	aes_context encptCtx, decptCtx;


};


enum NETWORK_ERROR_CODE // Don't use error code that windows socket already used.
{
	NEC_SUCCESS = -1, // Can't use value zero.
	NEC_PEER_RESET = 0,
	NEC_INVALID_HEADER = 1,
	NEC_INVALID_LENGTH,

};


class CSocketManager
{
public:

	CSocketManager()
	:m_hThread(NULL)
	{
		m_bDirty = FALSE;
		m_SocketCount = 0;
		ASSERT(pSocketManager == NULL);
		pSocketManager = this;
	}
	~CSocketManager()
	{
		ASSERT(pSocketManager == this);
		pSocketManager = NULL;
	}

	enum
	{
		MAX_SOCKET_COUNT = 5,
		MAX_EVENT_THREAD_WAIT_TIME = 200000, // In us.
	};


	void InitThread()
	{
		ASSERT(m_hThread == NULL);
		m_hThread = CreateThread(0, 0, SocketNotifyThread, this, 0, 0);
	}
	void WaitForClose()
	{
		while(m_hThread != NULL)
			Sleep(1);
	}
	BOOL DoesSocketExist(CSocketBase *pSocket)
	{
		for(UINT i = 0; i < m_SocketCount; ++i)
			if(pSocket == m_pSocketArray[i])
				return TRUE;
		return FALSE;
	}
	BOOL AddSocket(CSocketBase *pSocket)
	{
		ASSERT(pSocket->IsValidSocket());
		if(DoesSocketExist(pSocket))
			return FALSE;
		m_pSocketArray[m_SocketCount++] = pSocket;
		m_bDirty = TRUE;
		if(m_SocketCount == 1)
			InitThread();
		while(m_bDirty)
			Sleep(1);
		return TRUE;
	}


	static CSocketManager* GetSocketManager() { return pSocketManager; }


protected:

	static CSocketManager *pSocketManager;
	static DWORD WINAPI SocketNotifyThread(LPVOID lpParameter);

	volatile HANDLE m_hThread;
	volatile BOOL m_bDirty;
	UINT m_SocketCount;
	CSocketBase *m_pSocketArray[MAX_SOCKET_COUNT];


};


#define INDEX_TO_TIMER_HANDLE(i) ((HANDLE)((i) + 1)) // Reserves zero for error check.
#define TIMER_HANDLE_TO_INDEX(h) ((UINT)(h) - 1)
#define TO_TIMER(list_entry) CONTAINING_RECORD(list_entry, CTimerEventImp::stSimpleTimer, ListEntry)

#define DO_TIMER_CMD_NOTIFY(pTimer) \
{                                   \
	if(m_bUsebDedicatedTimerThread) \
	{                               \
		AddTimerCmd(pTimer);        \
		SetEvent(m_hTimerEvent);    \
	}                               \
	else                            \
		TimerCmdNotify(pTimer);     \
}

typedef unsigned long long uint64;
//typedef UINT _TimeStamp;
typedef unsigned long long _TimeStamp;
typedef void (*TimerCallbackProto)(void *pCtx);

class CTimerEventImp
{
public:

	struct stSimpleTimer
	{
		void *pReserved; // Reserved for pointer of XStack.
		list_entry ListEntry;
		UINT HandleIndex;
		DWORD Flag;
		_TimeStamp Period, DueTime;
		TimerCallbackProto cb, NewCb;
		void *pCtx, *pNewCtx;

		operator list_entry*() { return &ListEntry; }

	};

	enum
	{
		MAX_TIMER_COUNT = 16,
		EPSILON = 0,

		// Timer commands and flags.
		TC_CREATE = 1,      // This flag is useless.
		TC_DELETE = 1 << 1,
		TC_UPDATE = 1 << 2,
		TC_STOP   = 1 << 3,

		TF_PAUSED      = 1 << 4,
		TF_ONCE        = 1 << 5,
		TF_RELATIVE    = 1 << 6, // Timer will use current time stamp to calculate next due time no matter if any delay exists.
		TF_AUTO_DELETE = 1 << 7, // Timer will be deleted if time's up or stopped by other method. It's better not to store handle for later use.
								 // If a timer has TF_AUTO_DELETE flag, the handle returned can only be used to check if timer created successfully.
		TF_INIT = TF_PAUSED,
	};


	CTimerEventImp()
	{
		m_hTimerEvent = NULL;
		m_bUsebDedicatedTimerThread = FALSE;
		m_nTimerCount = 0;
		m_nRunningTimer = 0;
		INIT_LIST_HEAD(&TimerListHead);
		ZeroMemory(m_pTimerArray, sizeof(m_pTimerArray));
		m_TimerObjPool.CreatePool(MAX_TIMER_COUNT, 1);

		TObjectPool<stSimpleTimer>::stObjPool *pPool = m_TimerObjPool.AddPool(0);
		pPool->InitAllObjects(InitTimerObjCallback);

		// Make sure data is aligned.
		ASSERT(IsAligned((void*)&m_nTimerCount, sizeof(m_nTimerCount)));
		ASSERT(IsAligned(m_pTimerArray, sizeof(void*)));
	}
	virtual ~CTimerEventImp()
	{
		ASSERT(m_TimerObjPool.CountFreeObj(0) == MAX_TIMER_COUNT);
		m_TimerObjPool.ReleasePool();
	}


	BOOL CreateDedicatedTimerThread();
	void EndDedicatedTimerThread();

	HANDLE CreateTimer();
	BOOL DeleteTimer(HANDLE hTimer);
	BOOL StartTimer(HANDLE hTimer, UINT msElapse, TimerCallbackProto cb, void *pCtx, DWORD dwFlag = TF_ONCE | TF_AUTO_DELETE);
	BOOL StopTimer(HANDLE hTimer);

	HANDLE StartTimerEx(UINT msElapse, TimerCallbackProto cb, void *pCtx, BOOL bOnce); // Provide this for simple use that it will create and start timer directly.

	__forceinline BOOL StartTimer(HANDLE hTimer, UINT msElapse, TimerCallbackProto cb, void *pCtx, BOOL bOnce)
	{
		return StartTimer(hTimer, msElapse, cb, pCtx, (DWORD)(bOnce ? TF_ONCE : NULL));
	}

	inline static _TimeStamp CetTimeStamp()
	{
	//	return timeGetTime();
		FILETIME filetime;
		GetSystemTimeAsFileTime(&filetime);
		return ((((uint64)filetime.dwHighDateTime) << 32) + filetime.dwLowDateTime) / 10000; // Convert to millisecond.
	}


protected:

	BOOL m_bUsebDedicatedTimerThread; // Data for dedicated timer thread.
	volatile BOOL m_bTMTRun;
	HANDLE m_hTimerThread, m_hTimerEvent;

	volatile UINT m_nTimerCount; // Counter for timers that have been allocated.
	UINT m_nRunningTimer;        // Counter for timers that are running.
	list_head TimerListHead;
	stSimpleTimer *m_pTimerArray[MAX_TIMER_COUNT];
	TObjectPool<stSimpleTimer> m_TimerObjPool;

	XStack m_TimerCmdSource; // Using stack to store commands may cause some problem if client does something wrong.
	XQueue m_TimerCmdQueue;

	static void InitTimerObjCallback(stSimpleTimer *pTimer, UINT index) { pTimer->HandleIndex = index; }
	static void InsertTimer(list_entry &ListHead, list_entry *pNewTimer); // Use insertion sort currently. Use min heap tree to improve performance for large timer count.
	static void DebugCheckTimerList(list_head &ListHead);
	static DWORD WINAPI TimerMonitorThread(LPVOID lpParameter);

	stSimpleTimer* AllocateTimerObj() { return m_TimerObjPool.AllocObj(); }
	void DeleteTimerObj(stSimpleTimer *pTimer) { m_TimerObjPool.FreeObj(pTimer); }

	void ExecuteTimerCmd(stSimpleTimer *pTimerIn = NULL);
	UINT ProcessTimer();
	void ReleaseAllTimer(); // Before call this must sure no timer process thread is running.


	virtual void TimerCmdNotify(stSimpleTimer *pTimer) { ExecuteTimerCmd(pTimer); }

	__forceinline void AddTimerCmd(stSimpleTimer *pTimer)
	{
		m_TimerCmdSource.PushObj(pTimer);
	//	m_TimerCmdQueue.Add(pTimer);
	}
	__forceinline stSimpleTimer* GetTimerCmd()
	{
		return (stSimpleTimer*)m_TimerCmdSource.PopObject();
	//	return (stSimpleTimer*)m_TimerCmdQueue.Get();
	}

	__forceinline BOOL DeleteTimerTest(stSimpleTimer *pTimerIn)
	{
		stSimpleTimer *pTimer;
		if((pTimer = (stSimpleTimer*)InterlockedExchangePointer((void**)&m_pTimerArray[pTimerIn->HandleIndex], NULL)) != NULL)
		{
			ASSERT(pTimer == pTimerIn);
			return TRUE;
		}
		return FALSE;
	}


};


struct stNotifyThreadCmd
{
	stNotifyThreadCmd(SOCKET socket = INVALID_SOCKET, void *pCBDataIn = NULL, void *pCBDataIn2 = NULL)
	:nCmd(0), hEvent(NULL), pResult(NULL)
	{
		s = socket;
		pCallbackData = pCBDataIn;
		pCallbackData2 = pCBDataIn2;
	}

	UINT   nCmd;
	HANDLE hEvent;
	BOOL   *pResult;

	SOCKET s;
	void *pCallbackData, *pCallbackData2;

};


class CEventManager : public CTimerEventImp
{
public:

	CEventManager()
	{
		ASSERT(pEventManager == NULL);
		pEventManager = this;

		bRun = FALSE;
		nNotifyCmdCount = 0;
		CmdSocket = INVALID_SOCKET;
		nSocketCount = 0;
		usIPCPort = 0;
		hThread = 0;

		m_bTMTRun = FALSE;
		m_hTimerThread = NULL;

		ASSERT(IsAligned((void*)&nNotifyCmdCount, sizeof(nNotifyCmdCount))); // Make sure data is aligned.
	}
	virtual ~CEventManager()
	{
		ASSERT(pEventManager == this);
		pEventManager = NULL;
	}

	enum
	{
		MAX_SOCKET = 16,

		// Notify commands.
		NOTIFY_ADD_SOCKET = 1,
		NOTIFY_REMOVE_SOCKET,
		NOTIFY_TIMER_CONTROL,
		NOTIFY_END_THREAD,
	};

	typedef struct _fd_set_ex
	{
		u_int  fd_count;
		SOCKET fd_array[MAX_SOCKET];
	} fd_set_ex;


	BOOL Init(BOOL bDedicatedTimerThread) { return InitThread(bDedicatedTimerThread); }
	void Close() { EndThread(); }

	BOOL AddSocket(CSocketBase *pSocket, BOOL bWait = FALSE);
	void RemoveSocket(SOCKET fd, BOOL bWait = FALSE);

	UINT GetSocketCount() { return nSocketCount - 1; }

	static CEventManager* GetEventManager() { ASSERT(pEventManager != NULL); return pEventManager; }


protected:

	volatile BOOL bRun;
	volatile LONG nNotifyCmdCount;
	UINT nSocketCount;
	USHORT usIPCPort;
	volatile SOCKET CmdSocket;
	HANDLE hThread;
	DWORD  dwThreadID;

	static CEventManager *pEventManager;
	static DWORD WINAPI EventMonitorThread(LPVOID lpParameter);

	BOOL InitThread(BOOL bDedicatedTimerThread);
	void EndThread();

	void LogIPCError(INT iBytesSent);

	USHORT GetIPCPort() { return usIPCPort; }

	virtual void TimerCmdNotify(stSimpleTimer *pTimer)
	{
		stNotifyThreadCmd msg;
		msg.nCmd = NOTIFY_TIMER_CONTROL;
		msg.pCallbackData = pTimer;

		INT iSend = send(CmdSocket, (char*)&msg, sizeof(stNotifyThreadCmd), 0);
		if(iSend == sizeof(stNotifyThreadCmd))
			InterlockedIncrement(&nNotifyCmdCount);
		else
			LogIPCError(iSend);
	}


};


