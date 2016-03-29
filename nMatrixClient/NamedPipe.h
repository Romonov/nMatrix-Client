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


#define MAX_READ_PIPE_BUFFER_SIZE (1024 * 10) // The message size is up to 65535 bytes.


class CPipeListener;


interface IPipeDataDest
{
	virtual void OnConnectingPipe() = 0;
	virtual void OnDisConnectingPipe(CPipeListener* pReader) = 0;
	virtual void OnIncomingData(const BYTE* pStr, DWORD nSize) = 0;
};


class CPipeListener
{
public:

	CPipeListener(IPipeDataDest* pDest = 0);
	~CPipeListener();


	BOOL CreateNamedPipe(const TCHAR *pPipeName);
	BOOL Connect(const TCHAR *pPipeName);
	BOOL WriteMsg(const BYTE *pData, UINT nCount);
	BOOL Close(BOOL bWaitThreadTerminate);
	BOOL StartReader(BOOL bPureRead = FALSE);

	BOOL IsListen() { return m_bStart; }
	void SetInterface(IPipeDataDest* pDest) { m_pDest = pDest; }


protected:

	static DWORD WINAPI ReadThread(LPVOID lpParameter);
	BOOL ReadPipe();     // For service.
	BOOL PureReadPipe(); // For GUI.

	BOOL    m_bStart, m_bPureRead;
	DWORD	m_nID;
	HANDLE	m_hNamedPipe, m_hThread;
	IPipeDataDest* m_pDest;
	OVERLAPPED m_OverlappedIO;


};


