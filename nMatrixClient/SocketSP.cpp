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
#include "UTX.h"
#include "SocketSP.h"


CSocketManager* CSocketManager::pSocketManager = 0;


BOOL CSocketBase::Accept(CSocketBase *pSocket, struct sockaddr *addr, INT *addrlen)
{
	ASSERT(pSocket->m_fd == INVALID_SOCKET);

	if((pSocket->m_fd = accept(m_fd, addr, addrlen)) != INVALID_SOCKET)
	{
		pSocket->m_flag = m_flag;
		pSocket->IncRef(OP_ACCEPT);
		return TRUE;
	}

	return FALSE;
}

BOOL CSocketBase::Create(UINT nProtocol, BOOL bOverlapped)
{
	ASSERT(m_fd == INVALID_SOCKET);
	m_pReservedData = 0;

	INT af = (nProtocol & SF_IPV4) ? AF_INET : AF_INET6;
	DWORD dwFlags = bOverlapped ? WSA_FLAG_OVERLAPPED : 0;

	if(bOverlapped)
		nProtocol |= SF_OVERLAPPED;

	if(nProtocol & SF_TCP_SOCKET)
		m_fd = WSASocket(af, SOCK_STREAM, IPPROTO_TCP, 0, 0, dwFlags);
	else
		m_fd = WSASocket(af, SOCK_DGRAM, IPPROTO_UDP, 0, 0, dwFlags);

	if(m_fd != INVALID_SOCKET)
	{
		m_flag = nProtocol;
		IncRef(OP_CREATE);
		return TRUE;
	}

	return FALSE;
}

INT CSocketBase::CloseSocket()
{
	SOCKET s;
	INT iResult;
	m_flag |= SF_SOCKET_CLOSED; // Update flag first so we won't get notification after closing socket directly in most cases.
	if((s = InterlockedExchange((LONG*)&m_fd, INVALID_SOCKET)) != INVALID_SOCKET)
	{
		iResult = closesocket(s);
		DecRef(OP_CLOSE);
		return iResult;
	}

	WSASetLastError(WSAENOTSOCK);
	return SOCKET_ERROR;
}

BOOL CSocketBase::SafeCloseSocket(DWORD dwWaitTime)
{
	if(m_fd == INVALID_SOCKET)
		return FALSE;
	HANDLE hEvent = CreateEvent(0, FALSE, FALSE, 0);
	if(!hEvent)
		return FALSE;

	ASSERT(!m_pReservedData);
	m_pReservedData = hEvent;

	if(CloseSocket() != SOCKET_ERROR)
		WaitForSingleObject(hEvent, dwWaitTime);

	CloseHandle(hEvent);

	return TRUE;
}

INT CSocketBase::ShutDown(INT how)
{
	DWORD dwOriginalFlag = m_flag;
	switch(how)
	{
		case SD_RECEIVE:
			m_flag |= SF_SHUTDOWN_RECV;
			break;
		case SD_SEND:
			m_flag |= SF_SHUTDOWN_SEND;
			break;
		case SD_BOTH:
			m_flag |= (SF_SHUTDOWN_RECV |SF_SHUTDOWN_SEND);
			break;
	}
	INT iResult = shutdown(m_fd, how);
	if(iResult) // Error occurred.
		m_flag = dwOriginalFlag;
	return iResult;
}

BOOL CSocketBase::AsyncSend(stSocketBufferBase *pSocketBuffer, UINT nSize)
{
	ASSERT( (m_flag & CSocketBase::SF_IOCP_EVENT) || m_fd == INVALID_SOCKET );

	BOOL bResult = FALSE;
	WSABUF wb;

	wb.buf = pSocketBuffer->pBuffer + pSocketBuffer->nOffset;
	wb.len = nSize;
	pSocketBuffer->Op = OP_SEND;
	pSocketBuffer->pSocket = this;
	ZeroMemory(&pSocketBuffer->wo, sizeof(pSocketBuffer->wo));

	if(!(m_flag & (1 << SF_CLOSE_NOTIFIED_BIT)))
	{
		IncRef(OP_SEND);
		INT iResult = WSASend(m_fd, &wb, 1, 0, 0, &pSocketBuffer->wo, 0);
		if(!iResult || WSAGetLastError() == WSA_IO_PENDING)
			bResult = TRUE;
		else
			DecRef(OP_SEND);
	}

	return bResult;
}

BOOL CSocketBase::IssueRecv(stSocketBufferBase *pSocketBuffer)
{
	ASSERT(m_flag & CSocketBase::SF_IOCP_EVENT);
	ASSERT(pSocketBuffer->nbuflen);

	BOOL bResult = FALSE;
	WSABUF wb;
	DWORD dwFlags = 0;

	wb.buf = pSocketBuffer->pBuffer + pSocketBuffer->nOffset;
	wb.len = pSocketBuffer->nbuflen;
	pSocketBuffer->Op = OP_RECV;
	pSocketBuffer->pSocket = this;
	ZeroMemory(&pSocketBuffer->wo, sizeof(pSocketBuffer->wo));

	if(!(m_flag & (1 << SF_CLOSE_NOTIFIED_BIT)))
	{
		IncRef(OP_RECV);
		INT iResult = ::WSARecv(m_fd, &wb, 1, 0, &dwFlags, &pSocketBuffer->wo, 0);
		if(!iResult || WSAGetLastError() == WSA_IO_PENDING)
			bResult = TRUE;
		else
			DecRef(OP_RECV);
	}

	return bResult;
}

BOOL CSocketBase::IssueRecvFrom(stSocketBufferBase *pSocketBuffer)
{
	ASSERT(m_flag & CSocketBase::SF_IOCP_EVENT);
	ASSERT(pSocketBuffer->nbuflen);

	BOOL bResult = FALSE;
	WSABUF wb;
	DWORD dwFlags = 0;

	wb.buf = pSocketBuffer->pBuffer + pSocketBuffer->nOffset;
	wb.len = pSocketBuffer->nbuflen;
	pSocketBuffer->Op = OP_RECV;
	pSocketBuffer->pSocket = this;
	ZeroMemory(&pSocketBuffer->wo, sizeof(pSocketBuffer->wo));
	pSocketBuffer->addrlen = sizeof(pSocketBuffer->addr);

	if(!(m_flag & (1 << SF_CLOSE_NOTIFIED_BIT)))
	{
		IncRef(OP_RECV);
		INT iResult = ::WSARecvFrom(m_fd, &wb, 1, 0, &dwFlags, (sockaddr*)&pSocketBuffer->addr, &pSocketBuffer->addrlen, &pSocketBuffer->wo, 0);
		if(!iResult || WSAGetLastError() == WSA_IO_PENDING)
			bResult = TRUE;
		else
			DecRef(OP_RECV);
	}

	return bResult;
}

INT CSocketBase::IsConnected(DWORD dwWaitTime)
{
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = dwWaitTime * 1000;
	SOCKET fd = GetSocket();
	fd_set WriteSet, ExceptSet;
	FD_ZERO(&WriteSet);
	FD_ZERO(&ExceptSet);
	FD_SET(fd, &WriteSet);
	FD_SET(fd, &ExceptSet);

	ASSERT(m_flag & SF_TCP_SOCKET);

	if(select(fd, 0, &WriteSet, &ExceptSet, &tv) == SOCKET_ERROR)
		return TSCS_ERROR;
	if(FD_ISSET(fd, &WriteSet))
		return TSCS_CONNECTED;
	if(FD_ISSET(fd, &ExceptSet))
		return TSCS_ERROR;

	return TSCS_WAIT;
}

BOOL CSocketBase::CreateEx(UINT nProtocol, DWORD ip, USHORT port)
{
	ASSERT(!IsValidSocket());

	if(Create(nProtocol))
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = ip ? ip : INADDR_ANY;
		addr.sin_port = htons(port);

		if(Bind((sockaddr*)&addr, sizeof(addr)) !=  SOCKET_ERROR)
			return TRUE;

		CloseSocket();
	}

	return FALSE;
}

BOOL CSocketBase::ConnectEx(LPCTSTR lpszHostAddress, UINT nHostPort)
{
	USES_CONVERSION_EX;

	ASSERT(lpszHostAddress != NULL);

	if (lpszHostAddress == NULL)
	{
		WSASetLastError (WSAEINVAL);
		return FALSE;
	}

	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));

	LPSTR lpszAscii = T2A_EX((LPTSTR)lpszHostAddress, _ATL_SAFE_ALLOCA_DEF_THRESHOLD);
	if (lpszAscii == NULL)
	{
		WSASetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);

	if (sockAddr.sin_addr.s_addr == INADDR_NONE)
	{
		LPHOSTENT lphost;
		lphost = gethostbyname(lpszAscii);
		if (lphost != NULL)
			sockAddr.sin_addr.s_addr = ((LPIN_ADDR)lphost->h_addr)->s_addr;
		else
		{
			WSASetLastError(WSAEINVAL);
			return FALSE;
		}
	}

	sockAddr.sin_port = htons((u_short)nHostPort);

	return Connect((SOCKADDR*)&sockAddr, sizeof(sockAddr));
}

BOOL CSocketBase::GetSockNameEx(CIpAddress &addr)
{
	SOCKADDR_IN saddr;
	memset(&saddr, 0, sizeof(saddr));
	INT nSockAddrLen = sizeof(saddr);
	if(!GetSockName((SOCKADDR*)&saddr, &nSockAddrLen))
	{
		addr.m_bIsIPV6 = FALSE;
		addr.v4 = saddr.sin_addr.S_un.S_addr;
		addr.m_port = ntohs(saddr.sin_port);
		return TRUE;
	}
	return FALSE;
}

void CSocketBase::ReturnBuffer(stSocketBufferBase *pBuffer)
{
//	if(pBuffer->Flag & FLAG_SYS_BUF)
//		AppGetSocketManager()->FreeSocketBuffer((stSocketBuffer*)pBuffer);
}


void CSocketTCP::SendData(void *pData, UINT size, BYTE byHeaderFlag)
{
	UCHAR iv[16] = {0};
	UINT newSize, padding = 0;
	BYTE buffer[sizeof(sCmdHeader) + MAX_CMD_PACKET_SIZE * 5];

	if(ENCRYPT_PACKET_DATA && (byHeaderFlag & TPHF_ENCRYPTED))
	{
		newSize = (size & 0x0f) ? ((size >> 4) + 1) << 4 : size;
		padding = newSize - size;
		INT nResult = aes_crypt_cbc(&encptCtx, AES_ENCRYPT, newSize, iv, (UCHAR*)pData, &buffer[sizeof(sCmdHeader)]);
	}
	else
	{
		newSize = size;
		memcpy(&buffer[sizeof(sCmdHeader)], pData, size);
		byHeaderFlag &= ~TPHF_ENCRYPTED; // Encryption was disabled for some reason.
	}
//	ASSERT(newSize <= MAX_CMD_PACKET_SIZE);

	sCmdHeader header;
	header.symbol = CMD_PACKET_SYMBOL;
	header.datasize = newSize;
	header.padding = padding;
	header.flag = byHeaderFlag;
	memcpy(buffer, &header, sizeof(header));
	INT nSend = Send((CHAR*)buffer, sizeof(header) + newSize);
//	printx("CSocketBase::SendData() %d bytes ---> %d bytes.\n", nSend, newSize + sizeof(header));
}

BOOL CSocketTCP::InitAesData(CHAR *pKey)
{
	aes_setkey_enc(&encptCtx, (UCHAR*)pKey, AES_KEY_LENGTH);
	aes_setkey_dec(&decptCtx, (UCHAR*)pKey, AES_KEY_LENGTH);

	return TRUE;
}

BOOL CSocketTCP::DecryptData(void *pIn, void *pOut, UINT size)
{
	UCHAR iv[16] = {0};
	INT nResult;

	if(size % 16)
	{
		printx("DecryptData(): size error: %d.\n", size);
		return FALSE;
	}

	nResult = aes_crypt_cbc(&decptCtx, AES_DECRYPT, size, iv, (UCHAR*)pIn, (UCHAR*)pOut);

	return TRUE;
}

void CSocketTCP::OnClose(int nErrorCode, void *pAssocData)
{
	if(m_CloseCallback)
		m_CloseCallback(nErrorCode, this);
}

BOOL CSocketTCP::OnReceive()
{
	INT iRead;
	UINT nPacketSize;
	sCmdHeader *pHeader = (sCmdHeader*)m_DataBuffer;

	iRead = Recv((CHAR*)m_DataBuffer + m_DataSize, sizeof(m_DataBuffer) - m_DataSize);
	if(!iRead || iRead == -1)
		return FALSE;

//	printx("Received data: %d\n", nRead);
	m_DataSize += iRead;
	ASSERT(m_DataSize <= sizeof(m_DataBuffer));

	do
	{
		if(m_DataSize <= sizeof(sCmdHeader))
			break;

		nPacketSize = sizeof(sCmdHeader) + pHeader->datasize;

		if(nPacketSize > m_DataSize)
			break;
		else
		{
			// Receive multiple command.
			if(pHeader->flag & TPHF_ENCRYPTED)
				DecryptData(pHeader + 1, pHeader + 1, pHeader->datasize);
			m_ReceiveCallback((BYTE*)(pHeader + 1), pHeader->datasize - pHeader->padding, this);
			m_DataSize -= nPacketSize;

			if(m_DataSize)
			{
				memmove(m_DataBuffer, m_DataBuffer + nPacketSize, m_DataSize);
				continue;
			}

			break;
		}
	}
	while(1);

	return TRUE;
}


DWORD WINAPI CSocketManager::SocketNotifyThread(LPVOID lpParameter)
{
	INT iResult;
	DWORD dwErrorCode;
	fd_set readset;
	CSocketManager *pSocketManager = (CSocketManager*)lpParameter;
	volatile UINT &nSocketCount = pSocketManager->m_SocketCount, i, nNewCount;
	CSocketBase *SocketObjectArray[CSocketManager::MAX_SOCKET_COUNT], **pSocketSrc = pSocketManager->m_pSocketArray;
	SOCKET SocketArray[CSocketManager::MAX_SOCKET_COUNT];
	timeval ti = { 0, CSocketManager::MAX_EVENT_THREAD_WAIT_TIME };

	printx("---> SocketNotifyThread. TID: %d.\n", GetCurrentThreadId());

	while(nSocketCount)
	{
		if(pSocketManager->m_bDirty)
		{
			for(i = nNewCount = 0; i < nSocketCount; ++i)
				if(pSocketSrc[i])
				{
					SocketObjectArray[nNewCount] = pSocketSrc[i];
					SocketArray[nNewCount++] = pSocketSrc[i]->GetSocket();
				}
			if(nSocketCount = nNewCount)
				memcpy(pSocketSrc, SocketObjectArray, sizeof(CSocketBase*) * nNewCount);
			pSocketManager->m_bDirty = FALSE;
			continue;
		}

		readset.fd_count = nSocketCount;
		memcpy(readset.fd_array, SocketArray, sizeof(SOCKET) * nSocketCount);

		iResult = select(0, (fd_set*)&readset, 0, 0, &ti);

		if(iResult == 0) // Time out.
			continue;
		if(iResult == SOCKET_ERROR)
		{
			printx("Select failed in SocketNotifyThread! ec: %d\n", WSAGetLastError());
			continue;
		}

		for(UINT i = 0; i < nSocketCount; ++i)
			if(FD_ISSET(SocketArray[i], &readset))
				if(SocketObjectArray[i]->OnReceive() == 0)
				{
					if((dwErrorCode = WSAGetLastError()) != WSAENOTSOCK)
						SocketObjectArray[i]->OnClose(dwErrorCode, 0);
					pSocketSrc[i] = 0;
					pSocketManager->m_bDirty = TRUE;
				}
	}

	CloseHandle(pSocketManager->m_hThread);
	pSocketManager->m_hThread = NULL;

	printx("<--- SocketNotifyThread. TID: %d.\n", GetCurrentThreadId());

	return 0;
}


HANDLE CTimerEventImp::CreateTimer()
{
	do
	{
		UINT index, count = InterlockedIncrementEx(&m_nTimerCount);
		if(count > MAX_TIMER_COUNT)
			break;

		stSimpleTimer *pTimer = AllocateTimerObj();
		index = pTimer->HandleIndex;
		ASSERT(m_pTimerArray[index] == NULL);
		m_pTimerArray[index] = pTimer;
		pTimer->Flag = TF_INIT;

		return INDEX_TO_TIMER_HANDLE(index);
	}
	while(0);

	InterlockedDecrementEx(&m_nTimerCount);

	return NULL;
}

BOOL CTimerEventImp::DeleteTimer(HANDLE hTimer)
{
	UINT index = TIMER_HANDLE_TO_INDEX(hTimer);
	if(index > MAX_TIMER_COUNT)
		return FALSE;

	// Get timer object from handle table.
	stSimpleTimer *pTimer;
	if((pTimer = (stSimpleTimer*)InterlockedExchangePointer((void**)&m_pTimerArray[index], NULL)) == NULL)
		return FALSE;

	ASSERT(pTimer->HandleIndex == index);
	if(pTimer->Flag & TF_PAUSED) // Not running.
	{
		DeleteTimerObj(pTimer);
		InterlockedDecrementEx(&m_nTimerCount);
	}
	else
	{
		pTimer->Flag |= TC_DELETE;
		DO_TIMER_CMD_NOTIFY(pTimer);
	}

	return TRUE;
}

BOOL CTimerEventImp::StartTimer(HANDLE hTimer, UINT msElapse, TimerCallbackProto cb, void *pCtx, DWORD dwFlag)
{
	ASSERT(msElapse > 0);
	ASSERT( (dwFlag & ~(TF_ONCE | TF_RELATIVE | TF_AUTO_DELETE)) == 0 );

	UINT index = TIMER_HANDLE_TO_INDEX(hTimer);
	if(index > MAX_TIMER_COUNT || m_pTimerArray[index] == NULL)
		return FALSE;

	stSimpleTimer *pTimer = m_pTimerArray[index];
//	if(!(pTimer->Flag & TF_PAUSED)) // The method provides timer reset.
//			return FALSE;

	pTimer->Flag |= (TC_UPDATE | dwFlag);
	pTimer->Period = msElapse;
	pTimer->NewCb = cb;
	pTimer->pNewCtx = pCtx;

	DO_TIMER_CMD_NOTIFY(pTimer);

	return TRUE;
}

BOOL CTimerEventImp::StopTimer(HANDLE hTimer)
{
	UINT index = TIMER_HANDLE_TO_INDEX(hTimer);
	if(index > MAX_TIMER_COUNT || m_pTimerArray[index] == NULL)
		return FALSE;

	stSimpleTimer *pTimer = m_pTimerArray[index];
	if(pTimer->Flag & TF_PAUSED)
		return FALSE;

	pTimer->Flag |= TC_STOP;
	DO_TIMER_CMD_NOTIFY(pTimer);

	return TRUE;
}

HANDLE CTimerEventImp::StartTimerEx(UINT msElapse, TimerCallbackProto cb, void *pCtx, BOOL bOnce)
{
	do
	{
		UINT index, count = InterlockedIncrementEx(&m_nTimerCount);
		if(count > MAX_TIMER_COUNT)
			break;

		stSimpleTimer *pTimer = AllocateTimerObj();
		index = pTimer->HandleIndex;
		ASSERT(m_pTimerArray[index] == NULL);
		m_pTimerArray[index] = pTimer;

		pTimer->Flag = (bOnce) ? (TF_INIT | TC_UPDATE | TF_ONCE) : (TF_INIT | TC_UPDATE);
		pTimer->Period = msElapse;
		pTimer->NewCb = cb;
		pTimer->pNewCtx = pCtx;

		DO_TIMER_CMD_NOTIFY(pTimer);

		return INDEX_TO_TIMER_HANDLE(index);
	}
	while(0);

	InterlockedDecrementEx(&m_nTimerCount);

	return NULL;
}

void CTimerEventImp::InsertTimer(list_entry &ListHead, list_entry *pNewTimer)
{
	for(list_entry *pNode = ListHead.next; IS_VALID_ENTRY(pNode, ListHead); pNode = pNode->next)
		if( TO_TIMER(pNewTimer)->DueTime <= TO_TIMER(pNode)->DueTime )
		{
			list_insert_before(pNewTimer, pNode);
			return;
		}

	list_add_tail(pNewTimer, &ListHead);
}

void CTimerEventImp::DebugCheckTimerList(list_head &ListHead)
{
#ifdef DEBUG

	UINT count = 0;
	_TimeStamp max = 0;
	stSimpleTimer *pTimer, *pArray[MAX_TIMER_COUNT] = { 0 }; // In debug mode we can see each object directly from pointer array.

	for(list_entry *entry = ListHead.next; IS_VALID_ENTRY(entry, ListHead); entry = entry->next)
	{
		pArray[count] = pTimer = TO_TIMER(entry);
		ASSERT(pTimer->DueTime >= max);
		max = pTimer->DueTime;
		++count;
	}

	printx("Timer count: %u\n", count);

#endif
}

void CTimerEventImp::ExecuteTimerCmd(stSimpleTimer *pTimerIn)
{
	stSimpleTimer *pTimer = (pTimerIn != NULL) ? pTimerIn : GetTimerCmd();

	for(; pTimer != NULL; pTimer = GetTimerCmd())
	{
		if(pTimer->Flag & TC_DELETE)
		{
		//	ASSERT(!(pTimer->Flag & TF_PAUSED)); // Timer may enter paused state if it's once only.
			if(!(pTimer->Flag & TF_PAUSED))
				list_remove(*pTimer);
			--m_nRunningTimer;
			DeleteTimerObj(pTimer);
			InterlockedDecrementEx(&m_nTimerCount);
		}
		else if(pTimer->Flag & TC_UPDATE)
		{
			pTimer->DueTime = pTimer->Period + CetTimeStamp();
			pTimer->cb = pTimer->NewCb;
			pTimer->pCtx = pTimer->pNewCtx;
			if(pTimer->Flag & TF_PAUSED)
			{
				++m_nRunningTimer;
				pTimer->Flag &= ~TF_PAUSED;
			}
			else
			{
				list_remove(*pTimer);
			}
			InsertTimer(TimerListHead, *pTimer);
			pTimer->Flag &= ~TC_UPDATE;
		}
		else if(pTimer->Flag & TC_STOP)
		{
			ASSERT(!(pTimer->Flag & TF_PAUSED));

			list_remove(*pTimer);
			--m_nRunningTimer;

			if((pTimer->Flag & TF_AUTO_DELETE) && DeleteTimerTest(pTimer))
			{
				DeleteTimerObj(pTimer);
				InterlockedDecrementEx(&m_nTimerCount);
			}
			else
			{
				pTimer->Flag |= TF_PAUSED;
				pTimer->Flag &= ~TC_STOP;
			}
		}
	}
}

UINT CTimerEventImp::ProcessTimer()
{
	_TimeStamp tstamp = CetTimeStamp();
	list_head TimerListHeadReserved;
	INIT_LIST_HEAD(&TimerListHeadReserved);

	if(m_nRunningTimer && TO_TIMER(TimerListHead.next)->DueTime <= tstamp + EPSILON)
	{
		for(list_entry *pEntry = TimerListHead.next, *pNext; IS_VALID_ENTRY(pEntry, TimerListHead); pEntry = pNext)
		{
			pNext = pEntry->next;
			stSimpleTimer *pTimer = TO_TIMER(pEntry);

			if(pTimer->DueTime <= tstamp + EPSILON)
			{
				list_remove(pEntry);

				pTimer->cb(pTimer->pCtx);
				if(pTimer->Flag & (TF_ONCE | TF_AUTO_DELETE))
				{
					--m_nRunningTimer;

					if((pTimer->Flag & TF_AUTO_DELETE) && DeleteTimerTest(pTimer))
					{
						DeleteTimerObj(pTimer);
						InterlockedDecrementEx(&m_nTimerCount);
					}
					else
					{
						pTimer->Flag |= TF_PAUSED;
					}
				}
				else
				{
					if(pTimer->Flag & TF_RELATIVE)
						pTimer->DueTime = tstamp + pTimer->Period;
					else
						do
							pTimer->DueTime += pTimer->Period;
						while(pTimer->DueTime <= tstamp); // Prevent large delay when system is busy.

					list_add_single(pEntry, &TimerListHeadReserved); // Use post process to keep precision.
				}
				continue;
			}

			break;
		}

		if(!LIST_IS_EMPTY(TimerListHeadReserved))
			for(list_entry *pEntry = TimerListHeadReserved.next, *pNext; IS_VALID_ENTRY(pEntry, TimerListHeadReserved); pEntry = pNext)
			{
				pNext = pEntry->next;
				InsertTimer(TimerListHead, pEntry);
			}
	}

	UINT dwWaitTime = m_nRunningTimer ? (UINT)(TO_TIMER(TimerListHead.next)->DueTime - tstamp) : 0;

	return dwWaitTime;
}

void CTimerEventImp::ReleaseAllTimer()
{
	stSimpleTimer *pTimer;
	for(list_entry *pEntry = TimerListHead.next, *pNext; IS_VALID_ENTRY(pEntry, TimerListHead); pEntry = pNext) // Delete timers that are running.
	{
		pNext = pEntry->next;
		pTimer = TO_TIMER(pEntry);

		ASSERT(m_pTimerArray[pTimer->HandleIndex] != NULL);
		m_pTimerArray[pTimer->HandleIndex] = NULL;

		DeleteTimerObj(TO_TIMER(pEntry)); // Delete object at last.
	}

	INIT_LIST_HEAD(&TimerListHead);

	for(UINT i = 0; i < MAX_TIMER_COUNT; ++i) // Search and delete idle timers.
		if(m_pTimerArray[i] != NULL)
		{
			DeleteTimerObj(m_pTimerArray[i]);
			m_pTimerArray[i] = NULL;
		}

	m_nTimerCount = m_nRunningTimer = 0;
}

DWORD WINAPI CTimerEventImp::TimerMonitorThread(LPVOID lpParameter)
{
	CTimerEventImp *pTimerEventImp = (CTimerEventImp*)lpParameter;
	DWORD dwRet, dwWaitTime = INFINITE;
	HANDLE hWaitEvent = pTimerEventImp->m_hTimerEvent;
	volatile BOOL &bRun = pTimerEventImp->m_bTMTRun;

	for(bRun = TRUE; bRun;)
	{
		// For test purpose only, its precision is affected by windows thread scheduler like GetTickCount.
		dwRet = WaitForSingleObject(hWaitEvent, dwWaitTime);

		if(dwRet == WAIT_OBJECT_0)
			pTimerEventImp->ExecuteTimerCmd();

		dwWaitTime = pTimerEventImp->ProcessTimer();
		if(dwWaitTime == 0)
			dwWaitTime = INFINITE;
	}

	return 0;
}

BOOL CTimerEventImp::CreateDedicatedTimerThread()
{
	ASSERT(m_hTimerThread == NULL && m_hTimerEvent == NULL);

	if((m_hTimerEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
		return FALSE;
	if((m_hTimerThread = CreateThread(0, 0, TimerMonitorThread, this, 0, NULL)) != NULL)
	{
		m_bUsebDedicatedTimerThread = TRUE;
		while(!m_bTMTRun)
			Sleep(1);
		return TRUE;
	}

	CloseHandle(m_hTimerEvent);
	m_hTimerEvent = NULL;

	return FALSE;
}

void CTimerEventImp::EndDedicatedTimerThread()
{
	if(m_hTimerThread == NULL && m_hTimerEvent == NULL && !m_bTMTRun)
		return;
	m_bTMTRun = FALSE;
	SetEvent(m_hTimerEvent);
	WaitForSingleObject(m_hTimerThread, 2000);
	CloseHandle(m_hTimerThread);
	CloseHandle(m_hTimerEvent);
	m_hTimerThread = NULL;
	m_hTimerEvent = NULL;
	m_bUsebDedicatedTimerThread = FALSE;
}


CEventManager* CEventManager::pEventManager = NULL;


void OnRemoveSocketFromNotifyThread(volatile UINT &nSocketCount, UINT nIndex, SOCKET *pSocketArray, void **CbDataArray)
{
	UINT i = nIndex;
	if(nSocketCount > i + 1)
	{
		memmove(&pSocketArray[i], &pSocketArray[i + 1], sizeof(SOCKET) * (nSocketCount - i - 1));
		memmove(&CbDataArray[i], &CbDataArray[i + 1], sizeof(void*) * (nSocketCount - i - 1));
	}
	--nSocketCount;
}

BOOL OnTcpSocketEvent(SOCKET fd, void *pData)
{
//	printx("---> OnTcpSocketEvent. Thread ID: %d.\n", GetCurrentThreadId());
	CSocketTCP *pSocket = (CSocketTCP*)pData;
	ASSERT(pSocket->GetSocket() == fd);

	BOOL bSuccess = pSocket->OnReceive();
	if(!bSuccess)
		pSocket->OnClose(WSAGetLastError(), 0);
	return bSuccess;
}

BOOL CEventManager::AddSocket(CSocketBase *pSocket, BOOL bWait)
{
	BOOL bResult = FALSE;
	stNotifyThreadCmd msg(pSocket->GetSocket(), pSocket);
	msg.nCmd = CEventManager::NOTIFY_ADD_SOCKET;
	if(bWait)
	{
		msg.pResult = &bResult;
		msg.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	INT iSend = send(CmdSocket, (char*)&msg, sizeof(msg), 0); // Use socket to do inter-thread communication cost much cpu time.
	if(iSend != sizeof(msg))
		LogIPCError(iSend);
	else
	{
		InterlockedIncrement(&nNotifyCmdCount);
		if(bWait)
			WaitForSingleObject(msg.hEvent, INFINITE);
		else
			bResult = TRUE;
	}

	if(msg.hEvent)
		CloseHandle(msg.hEvent);

	return bResult;
}

void CEventManager::RemoveSocket(SOCKET fd, BOOL bWait)
{
	BOOL bResult = FALSE;
	stNotifyThreadCmd msg(fd);
	msg.nCmd = CEventManager::NOTIFY_REMOVE_SOCKET;
	if(bWait)
	{
		msg.pResult = &bResult;
		msg.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	INT iSend = send(CmdSocket, (char*)&msg, sizeof(msg), 0);
	if(iSend != sizeof(msg))
		LogIPCError(iSend);
	else
	{
		InterlockedIncrement(&nNotifyCmdCount);
		if(bWait)
			WaitForSingleObject(msg.hEvent, INFINITE);
		else
			bResult = TRUE;
	}

	if(msg.hEvent != NULL)
		CloseHandle(msg.hEvent);
}

DWORD WINAPI CEventManager::EventMonitorThread(LPVOID lpParameter)
{
	CEventManager *pEventManager = (CEventManager*)lpParameter;
	volatile BOOL &bRun = pEventManager->bRun;

	INT iResult, iDataSize;
	stNotifyThreadCmd msg;
	SOCKET NotifySocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(NotifySocket != INVALID_SOCKET)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = IP(127, 0, 0, 1);
		addr.sin_port = htons(pEventManager->GetIPCPort());
		bRun = (connect(NotifySocket, (sockaddr*)&addr, sizeof(addr)) == 0);
	}

	BOOL bResult;
	fd_set_ex readset;
	volatile UINT &nSocketCount = pEventManager->nSocketCount;
	volatile LONG &nNotifyCmdCount = pEventManager->nNotifyCmdCount;
	SOCKET SocketArray[CEventManager::MAX_SOCKET + 1];
	void *CbData[CEventManager::MAX_SOCKET + 1] = { 0 };

	timeval tv;
	tv.tv_sec = tv.tv_usec = 0;
	SocketArray[0] = NotifySocket;
	nSocketCount = 1;

	for(; bRun;)
	{
		readset.fd_count = nSocketCount;
		memcpy(readset.fd_array, SocketArray, sizeof(SOCKET) * nSocketCount);

		iResult = select(0, (fd_set*)&readset, 0, 0, tv.tv_usec ? &tv : NULL);

		for(bResult = FALSE; nNotifyCmdCount; bResult = FALSE)
		{
			iDataSize = recv(NotifySocket, (char*)&msg, sizeof(msg), 0);
			ASSERT(iDataSize == sizeof(msg));
			InterlockedDecrement(&nNotifyCmdCount);

			switch(msg.nCmd)
			{
				case NOTIFY_ADD_SOCKET:
					if(nSocketCount < _countof(SocketArray))
					{
						SocketArray[nSocketCount] = msg.s;
						CbData[nSocketCount] = msg.pCallbackData;
						++nSocketCount;
						bResult = TRUE;
					}
					break;

				case NOTIFY_REMOVE_SOCKET:
					for(UINT i = 1; i < nSocketCount; ++i)
						if(SocketArray[i] == msg.s)
						{
							bResult = TRUE;
							OnRemoveSocketFromNotifyThread(nSocketCount, i, SocketArray, CbData);
							break;
						}
					break;

				case NOTIFY_TIMER_CONTROL:
					pEventManager->ExecuteTimerCmd((stSimpleTimer*)msg.pCallbackData);
					break;

				case NOTIFY_END_THREAD:
					goto End;
					break;
			}

			if(msg.hEvent)
			{
				*msg.pResult = bResult;
				SetEvent(msg.hEvent);
			}
		}

		if(iResult == SOCKET_ERROR)
		{
			INT iErr = WSAGetLastError();
			ASSERT(iErr != WSAENOTSOCK); // If this assert failed, make sure you remove socket before closing it.
			printx("Select failed in EventMonitorThread! ec: %d\n", iErr);
			continue;
		}
		else if(!iResult) // Time out.
		{
		}
		else if(iResult)
		{
			for(UINT i = 1; i < nSocketCount; ++i)
				if(FD_ISSET(SocketArray[i], &readset))
					if(OnTcpSocketEvent(SocketArray[i], CbData[i]) == 0)
						OnRemoveSocketFromNotifyThread(nSocketCount, i, SocketArray, CbData);
		}

		if(!pEventManager->m_bUsebDedicatedTimerThread)
			tv.tv_usec =  pEventManager->ProcessTimer() * 1000;
	}

End:

	pEventManager->bRun = FALSE;
	if(NotifySocket != INVALID_SOCKET)
		closesocket(NotifySocket);
	if(pEventManager->CmdSocket != INVALID_SOCKET)
	{
		closesocket(pEventManager->CmdSocket);
		pEventManager->CmdSocket = INVALID_SOCKET;
	}

	printx("<--- SocketNotifyThread. TID: %d.\n", pEventManager->dwThreadID);

	return 0;
}

BOOL CEventManager::InitThread(BOOL bDedicatedTimerThread)
{
	ASSERT(!hThread && usIPCPort == 0);
	BOOL bResult = FALSE;

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(ListenSocket == INVALID_SOCKET)
		return FALSE;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = IP(127, 0, 0, 1);
	addr.sin_port = 0; // Let os choose the port.

	do
	{
		if(bind(ListenSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
			break;
		INT namelen = sizeof(addr);
		if(getsockname(ListenSocket, (SOCKADDR*)&addr, &namelen) == SOCKET_ERROR)
			break;
		if(listen(ListenSocket, 1) == SOCKET_ERROR)
			break;

		usIPCPort = ntohs(addr.sin_port);

		hThread = CreateThread(0, 0, EventMonitorThread, this, 0, &dwThreadID);
		if(!hThread)
			break;

		if((CmdSocket = accept(ListenSocket, 0, 0)) != INVALID_SOCKET)
			bResult = TRUE;

		if(bDedicatedTimerThread)
			bResult = CreateDedicatedTimerThread();
	}
	while(0);

	usIPCPort = 0;
	if(ListenSocket != INVALID_SOCKET)
		closesocket(ListenSocket);

	return bResult;
}

void CEventManager::EndThread()
{
	ASSERT(hThread);

	stNotifyThreadCmd msg;
	msg.nCmd = CEventManager::NOTIFY_END_THREAD;

	INT iSend = send(CmdSocket, (char*)&msg, sizeof(msg), 0);
	if(iSend == sizeof(msg))
		InterlockedIncrement(&nNotifyCmdCount);
	else
		LogIPCError(iSend);

	WaitForSingleObject(hThread, 2000);
	CloseHandle(hThread);
	hThread = 0;
	nNotifyCmdCount = 0;
	nSocketCount = 0;

	if(m_bUsebDedicatedTimerThread)
		EndDedicatedTimerThread();

	// Release timer objects so we won't get an assertion failure warning.
	ReleaseAllTimer();
}

void CEventManager::LogIPCError(INT iBytesSent)
{
	if(iBytesSent == SOCKET_ERROR)
		printx("IPC error: %d\n", WSAGetLastError());
	else
		printx("IPC error: %d / %d\n", iBytesSent, sizeof(stNotifyThreadCmd));
}


