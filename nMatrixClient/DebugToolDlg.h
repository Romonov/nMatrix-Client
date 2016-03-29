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


#include "resource.h"
#include "NamedPipe.h"
#include "UTX.h"


struct CRemoteConsole : public IPipeDataDest
{
public:
	CRemoteConsole()
	:m_NamedPipe(this)
	{
	}
	~CRemoteConsole()
	{
	}


	// Interface.
	void OnConnectingPipe()
	{
		printx("Enter OnConnectingPipe.\n");
	}
	void OnDisConnectingPipe(CPipeListener* pReader)
	{
		printx("Enter OnDisConnectingPipe.\n");
	}
	void OnIncomingData(const BYTE* pStr, DWORD nSize)
	{
		if(nSize % 2)
			printx((CHAR*)pStr);
		else
			printx((TCHAR*)pStr);
	}

	CPipeListener* GetPipeObject() { return &m_NamedPipe; }


protected:
	CPipeListener m_NamedPipe;


};


class CDebugToolDlg : public CDialog
{
public:
	CDebugToolDlg(CWnd* pParent = NULL);
	virtual ~CDebugToolDlg();


	afx_msg void OnBnClickedSetRegid();
	afx_msg void OnBnClickedGetRegid();
	afx_msg void OnBnClickedShowConsole();

	afx_msg void OnBnClickedResetAdapter();
	afx_msg void OnBnClickedRenewAdapter();
	afx_msg void OnBnClickedPrintConnName();
	afx_msg void OnBnClickedRead();
	afx_msg void OnBnClickedWrite();

	afx_msg void OnBnClickedInstallService();
	afx_msg void OnBnClickedDeleteService();
	afx_msg void OnBnClickedStartService();
	afx_msg void OnBnClickedStopService();

	afx_msg void OnBnClickedCreatePipe();
	afx_msg void OnBnClickedStartListen();
	afx_msg void OnBnClickedClosePipe();

	afx_msg void OnBnClickedUserMode();
	afx_msg void OnBnClickedKernelMode();

	afx_msg void OnBnClickedQueryTime();

	enum { IDD = IDD_DEBUG_TOOL };


protected:
	CRemoteConsole m_RemoteConsole;
	CPipeListener  *m_pNamedPipe;


	void ShowRegisterID();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()


};


