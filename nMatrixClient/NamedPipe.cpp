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
#include "NamedPipe.h"
#include "UTX.h"


CPipeListener::CPipeListener(IPipeDataDest* pDest)
{
	m_bStart = FALSE;
	m_bPureRead = FALSE;
	m_nID = 0;
	m_hNamedPipe = INVALID_HANDLE_VALUE;
	m_hThread = 0;
	m_pDest = pDest;

	ZeroMemory(&m_OverlappedIO, sizeof(m_OverlappedIO));
}

CPipeListener::~CPipeListener()
{
	Close(TRUE);
}

BOOL CPipeListener::CreateNamedPipe(const TCHAR *pPipeName)
{
	if(m_hNamedPipe != INVALID_HANDLE_VALUE)
		return FALSE;

//	m_hNamedPipe = ::CreateNamedPipe(pPipeName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, 200, NULL);
	m_hNamedPipe = ::CreateNamedPipe(pPipeName, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE | PIPE_WAIT,
									 PIPE_UNLIMITED_INSTANCES, 0, 0, 200, NULL);

	if(m_hNamedPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	return TRUE;
}

BOOL CPipeListener::Connect(const TCHAR *pPipeName)
{
	ASSERT(m_hNamedPipe == INVALID_HANDLE_VALUE);

	m_hNamedPipe = CreateFile(pPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

	if(m_hNamedPipe != INVALID_HANDLE_VALUE) // OnDisConnectingPipe won't be called at active side currently.
		m_pDest->OnConnectingPipe();

	return m_hNamedPipe != INVALID_HANDLE_VALUE;
}

BOOL CPipeListener::WriteMsg(const BYTE *pData, UINT nCount)
{
	if(m_hNamedPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	ASSERT(nCount >= sizeof(UINT));
	DWORD nWritten = 0, tArray[2] = {0};

	if(nCount > MAX_READ_PIPE_BUFFER_SIZE)
	{
	//	printx("PipeSend: %u bytes.\n", nCount);
		WriteFile(m_hNamedPipe, &nCount, sizeof(nCount), &nWritten, NULL); // Send data size first.

		INT pos, i, SendCount = nCount / MAX_READ_PIPE_BUFFER_SIZE;
		if(nCount % MAX_READ_PIPE_BUFFER_SIZE)
			SendCount++;

		for(i = 0, pos = 0; i < SendCount; ++i)
		{
			if(i == SendCount - 1)
			{
				WriteFile(m_hNamedPipe, pData + pos, nCount - pos, &nWritten, NULL);
				break;
			}

			WriteFile(m_hNamedPipe, pData + pos, MAX_READ_PIPE_BUFFER_SIZE, &nWritten, NULL);
			pos += MAX_READ_PIPE_BUFFER_SIZE;
			ASSERT(nWritten == MAX_READ_PIPE_BUFFER_SIZE);
		}
	}
	else if(nCount == sizeof(DWORD))
	{
		tArray[0] = *(DWORD*)pData;
		WriteFile(m_hNamedPipe, tArray, sizeof(tArray), &nWritten, NULL);
	}
	else
	{
		WriteFile(m_hNamedPipe, pData, nCount, &nWritten, NULL);
	//	ASSERT(nWritten == nCount);
	}

	return TRUE;
}

BOOL CPipeListener::Close(BOOL bWaitThreadTerminate)
{
	m_bStart = FALSE;

	if(m_OverlappedIO.hEvent)
		SetEvent(m_OverlappedIO.hEvent);

	if(m_hNamedPipe != INVALID_HANDLE_VALUE) // Close pipe first.
	{
		CloseHandle(m_hNamedPipe);
		m_hNamedPipe = INVALID_HANDLE_VALUE;
	}

	if(m_hThread && bWaitThreadTerminate)
	{
		printx("Wait for pipe thread termination.\n");
		DWORD dwResult = WaitForSingleObject(m_hThread, 5000);
		if(dwResult != WAIT_OBJECT_0)
			printx("CPipeListener::Close::WaitForSingleObject failed! Result: %08x. Error code: %08x.\n", dwResult, GetLastError());
	}
	SAFE_CLOSE_HANDLE(m_hThread);

	return TRUE;
}

BOOL CPipeListener::StartReader(BOOL bPureRead)
{
	if(m_bStart || m_hNamedPipe == INVALID_HANDLE_VALUE)
		return FALSE;
	m_bStart = TRUE;
	m_bPureRead = bPureRead;
	m_hThread = CreateThread(NULL, 0, ReadThread, this, 0, &m_nID);
	return m_hThread != NULL;
}

BOOL CPipeListener::ReadPipe()
{
	BOOL bResult;
	DWORD dwError;

Start:
	ASSERT(!m_OverlappedIO.hEvent);
	m_OverlappedIO.hEvent = CreateEvent(0, FALSE, FALSE, 0);

	if(!ConnectNamedPipe(m_hNamedPipe, &m_OverlappedIO))
	{
		dwError = GetLastError();
		switch(dwError)
		{
			case ERROR_IO_PENDING:
				printx("Enter wait event.\n");
				bResult = WaitForSingleObject(m_OverlappedIO.hEvent, INFINITE);
				break;

			case ERROR_PIPE_CONNECTED: // Success.
				break;

			default:
				SAFE_CLOSE_HANDLE(m_OverlappedIO.hEvent);
				printx("Unknow error occurred: %d in ReadPipe.\n", dwError);
				return FALSE;
		}
	}
	SAFE_CLOSE_HANDLE(m_OverlappedIO.hEvent);

	if(!m_bStart)
		return FALSE;

	if(m_pDest)
		m_pDest->OnConnectingPipe();

	BYTE SBuffer[MAX_READ_PIPE_BUFFER_SIZE], *pDynaBuffer = 0;
	DWORD dwReaded, BufferSize, pos;
	m_OverlappedIO.hEvent = CreateEvent(0, FALSE, FALSE, 0);

	while(1)
	{
		dwReaded = 0;
		if(pDynaBuffer)
			bResult = ReadFile(m_hNamedPipe, pDynaBuffer + pos, BufferSize - pos, &dwReaded, &m_OverlappedIO);
		else
			bResult = ReadFile(m_hNamedPipe, SBuffer, sizeof(SBuffer), &dwReaded, &m_OverlappedIO);

		if(!bResult)
		{
			dwError = GetLastError();
			if(dwError == ERROR_IO_PENDING)
			{
				if(!GetOverlappedResult(m_hNamedPipe, &m_OverlappedIO, &dwReaded, TRUE))
				{
					printx("GetOverlappedResult failed in ReadPipe()! Error code: %d.\n", GetLastError());
					break;
				}
				if(!m_bStart)
					break;
			}
			else
			{
				printx("Unknow error occurred in ReadPipe()! Error code: %d.\n", dwError);
				break;
			}
		}

		if(dwReaded == 4 && !pDynaBuffer) // The transmission size may be 4 bytes in the last step.
		{
			BufferSize = *(DWORD*)SBuffer;
			printx("Allocate pipe message! Size: %d\n", BufferSize);
			pDynaBuffer = (BYTE*)malloc(BufferSize);
			pos = 0;
			continue;
		}
		if(pDynaBuffer)
		{
			pos += dwReaded;
			if(pos != BufferSize)
				continue;
			if(m_pDest)
				m_pDest->OnIncomingData(pDynaBuffer, BufferSize);
			free(pDynaBuffer);
			pDynaBuffer = 0;
			continue;
		}
		if(m_pDest && dwReaded)
			m_pDest->OnIncomingData(SBuffer, dwReaded);
	}

	if(pDynaBuffer)
		free(pDynaBuffer);

	SAFE_CLOSE_HANDLE(m_OverlappedIO.hEvent);

	if(m_hNamedPipe != INVALID_HANDLE_VALUE)
		if(!DisconnectNamedPipe(m_hNamedPipe))
			printx("DisconnectNamedPipe failed! Error code: %d.\n", GetLastError());

	if(m_pDest)
		m_pDest->OnDisConnectingPipe(this);

	if(m_bStart)
		goto Start;

	return TRUE;
}

BOOL CPipeListener::PureReadPipe()
{
	if(!m_bStart)
		return FALSE;

	ASSERT(!m_OverlappedIO.hEvent);
	m_OverlappedIO.hEvent = CreateEvent(0, FALSE, FALSE, 0);

	BOOL bResult;
	DWORD dwReaded, dwError, BufferSize, pos;
	BYTE SBuffer[MAX_READ_PIPE_BUFFER_SIZE], *pDynaBuffer = 0;

	while(1)
	{
		dwReaded = 0;
		if(pDynaBuffer)
			bResult = ReadFile(m_hNamedPipe, pDynaBuffer + pos, BufferSize - pos, &dwReaded, &m_OverlappedIO);
		else
			bResult = ReadFile(m_hNamedPipe, SBuffer, sizeof(SBuffer), &dwReaded, &m_OverlappedIO);

		if(!bResult)
		{
			dwError = GetLastError();
			if(dwError == ERROR_IO_PENDING)
			{
				if(!GetOverlappedResult(m_hNamedPipe, &m_OverlappedIO, &dwReaded, TRUE))
				{
					printx("GetOverlappedResult failed in ReadPipe()! Error code: %d.\n", GetLastError());
					break;
				}
				if(!m_bStart)
					break;
			}
			else
			{
				printx("Unknow error occurred in ReadPipe()! Error code: %d.\n", dwError);
				break;
			}
		}

		if(dwReaded == 4 && !pDynaBuffer) // The transmission size may be 4 bytes in the last step.
		{
			BufferSize = *(DWORD*)SBuffer;
			printx("Allocate pipe message! Size: %d\n", BufferSize);
			pDynaBuffer = (BYTE*)malloc(BufferSize);
			pos = 0;
			continue;
		}
		if(pDynaBuffer)
		{
			pos += dwReaded;
			if(pos != BufferSize)
				continue;
			if(m_pDest)
				m_pDest->OnIncomingData(pDynaBuffer, BufferSize);
			free(pDynaBuffer);
			pDynaBuffer = 0;
			continue;
		}
		if(m_pDest && dwReaded)
			m_pDest->OnIncomingData(SBuffer, dwReaded);
	}

	if(pDynaBuffer)
		free(pDynaBuffer);

	SAFE_CLOSE_HANDLE(m_OverlappedIO.hEvent);

	return TRUE;
}

DWORD WINAPI CPipeListener::ReadThread(LPVOID lpParameter)
{
	CPipeListener* pThis = (CPipeListener*)lpParameter;

	if(pThis->m_bPureRead)
		pThis->PureReadPipe();
	else
		pThis->ReadPipe();

	printx("Exit CPipeListener::ReadThread.\n");

	return 0;
}


