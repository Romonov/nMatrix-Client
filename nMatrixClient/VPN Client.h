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


#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif


#include "CnMatrixCore.h"
#include "ntserv.h"
#include "resource.h"


class CMyService : public CNTService
{
public:

	CMyService(TCHAR *pServiceName);

	virtual void WINAPI ServiceMain(DWORD dwArgc, LPTSTR* lpszArgv);
	virtual void OnStop();
	virtual void OnPause();
	virtual void OnContinue();
	virtual void OnShutdown();
	virtual void ShowHelp();
	virtual void OnUserDefinedRequestEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);


protected:

	CnMatrixCore m_nMatrixCore;
	BOOL  m_bWantStop;
	BOOL  m_bPaused;
	DWORD m_dwBeepInternal;


};


class CVPNClientApp : public CWinAppEx
{
public:

	CVPNClientApp();

	virtual BOOL InitInstance();
	virtual int ExitInstance();

	void ShowConsole()
	{
		if(m_bHasConsole)
			return;
		m_bHasConsole = AllocConsole();
	}


	BOOL InstallService(const TCHAR *path);
	BOOL DeleteService();
	BOOL StartService();
	BOOL StopService();

	void ReadConfigData(stConfigData *pConfig);
	void SaveConfigData(stConfigData *pConfig);
	UINT ReadLanListID();

	void SaveClosedMsgID(UINT nID);
	UINT LoadClosedMsgID();

	void SaveSvcLoginState(BOOL bOnline);
	BOOL LoadSvcLoginState();


protected:

	BOOL m_bDuplicateInstance, m_bHasConsole;
	TCHAR *m_pServiceName;

	DECLARE_MESSAGE_MAP()


};


BOOL SetClipboardContent(HWND hWndNewOwner, CString &info);

BOOL AppSaveRegisterID(stRegisterID *pID);
BOOL AppLoadRegisterID(stRegisterID *pID);

BOOL HasWindowsRemoteDesktop(CString *pPath);
BOOL ExecWindowsRemoteDesktop(CString &param);


extern CVPNClientApp theApp;
extern CFont GBoldFont;


