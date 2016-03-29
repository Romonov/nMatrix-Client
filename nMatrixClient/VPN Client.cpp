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
#include "VPN Client.h"
#include "VPN ClientDlg.h"
#include "SetupDialog.h"
#include "DriverAPI.h"
#include "ntserv_msg.h"


BOOL SetClipboardContent(HWND hWndNewOwner, CString &info)
{
	HGLOBAL hMem;
	INT size = (info.GetLength() + 1) * sizeof(TCHAR);

	if(OpenClipboard(hWndNewOwner))
	{
		hMem = GlobalAlloc(GMEM_MOVEABLE, size);
		if(hMem)
		{
			void *pAddr = (CHAR*)GlobalLock(hMem);
			if(pAddr)
			{
				EmptyClipboard();
				memcpy(pAddr, info.GetBuffer(), size);
				GlobalUnlock(hMem);
				SetClipboardData(CF_UNICODETEXT, hMem);
			}
			else
				GlobalFree(hMem);
		}
		CloseClipboard();
		return TRUE;
	}

	return FALSE;
}


void DecodeRegisterID(stRegisterID *pIn, stRegisterID *pOut)
{
	memcpy(pOut, pIn, sizeof(stRegisterID));
}

void EncodeRegisterID(stRegisterID *pIn, stRegisterID *pOut)
{
	memcpy(pOut, pIn, sizeof(stRegisterID));
}

TCHAR *pRegIDSectionName = _T("ClientInfo");
TCHAR *pRegIDEntryName = _T("RegID");

BOOL AppLoadRegisterID(stRegisterID *pID)
{
	BYTE *pData;
	UINT Size;
	stRegisterID RegID;
	BOOL bResult = FALSE;

	ASSERT(pID);
	pID->ZeroInit();

	if(theApp.GetProfileBinary(pRegIDSectionName, pRegIDEntryName, &pData, &Size))
	{
		if(Size != sizeof(stRegisterID))
			theApp.WriteProfileBinary(pRegIDSectionName, pRegIDEntryName, (BYTE*)&RegID, sizeof(stRegisterID));
		else
		{
			DecodeRegisterID((stRegisterID*)pData, pID);
			bResult = TRUE;
		}
		delete[] pData;
	}

	return bResult;
}

BOOL AppSaveRegisterID(stRegisterID *pID)
{
	stRegisterID RegID;
	BOOL bResult = FALSE;

	ASSERT(pID);
	EncodeRegisterID(pID, &RegID);

	if(theApp.WriteProfileBinary(pRegIDSectionName, pRegIDEntryName, (BYTE*)&RegID, sizeof(stRegisterID)))
		bResult = TRUE;

	return bResult;
}


typedef BOOL (WINAPI *FnWow64DisableWow64FsRedirection)(__out PVOID *OldValue);
typedef BOOL (WINAPI *FnWow64RevertWow64FsRedirection)(__in PVOID OldValue);

FnWow64DisableWow64FsRedirection GFnWow64DisableWow64FsRedirection = 0;
FnWow64RevertWow64FsRedirection GFnWow64RevertWow64FsRedirection = 0;


HMODULE LoadWow64Function()
{
	HMODULE hModule = LoadLibrary(_T("kernel32.dll"));
	if(!hModule)
		return 0;

	ASSERT(!GFnWow64DisableWow64FsRedirection && !GFnWow64RevertWow64FsRedirection);

	GFnWow64DisableWow64FsRedirection = (FnWow64DisableWow64FsRedirection)GetProcAddress(hModule, "Wow64DisableWow64FsRedirection");
	GFnWow64RevertWow64FsRedirection = (FnWow64RevertWow64FsRedirection)GetProcAddress(hModule, "Wow64RevertWow64FsRedirection");

	if(!GFnWow64DisableWow64FsRedirection || !GFnWow64RevertWow64FsRedirection)
	{
		GFnWow64DisableWow64FsRedirection = 0;
		GFnWow64RevertWow64FsRedirection = 0;
		FreeLibrary(hModule);
		return 0;
	}
	return hModule;
}

void FreeWow64Function(HMODULE hModule)
{
	ASSERT(GFnWow64DisableWow64FsRedirection && GFnWow64RevertWow64FsRedirection);
	GFnWow64DisableWow64FsRedirection = 0;
	GFnWow64RevertWow64FsRedirection = 0;
	FreeLibrary(hModule);
}


BOOL HasWindowsRemoteDesktop(CString *pPath)
{
	TCHAR buffer[MAX_PATH];
	INT len = GetSystemDirectory(buffer, MAX_PATH);
	buffer[len] = 0;
	_tcscat(buffer, _T("\\mstsc.exe"));

	DWORD att = GetFileAttributes(buffer);
	if(att == INVALID_FILE_ATTRIBUTES)
	{
		if(Is64BitsOs()) // Test with real system32 directory.
		{
			HMODULE hModule = LoadWow64Function();
			if(hModule)
			{
				PVOID OldValue = NULL;
				if(GFnWow64DisableWow64FsRedirection(&OldValue))
				{
					att = GetFileAttributes(buffer);
					GFnWow64RevertWow64FsRedirection(OldValue);
					if(att != INVALID_FILE_ATTRIBUTES)
					{
						FreeWow64Function(hModule);
						goto Success;
					}
				}
				FreeWow64Function(hModule);
			}
		}
		return FALSE;
	}

Success:
	if(pPath)
		*pPath = buffer;

	return TRUE;
}

BOOL ExecWindowsRemoteDesktop(CString &param)
{
	TCHAR buffer[MAX_PATH];
	INT len = GetSystemDirectory(buffer, MAX_PATH);
	buffer[len] = 0;
	_tcscat(buffer, _T("\\mstsc.exe"));

	INT nError = 0;
	DWORD att = GetFileAttributes(buffer);
	if(att == INVALID_FILE_ATTRIBUTES)
	{
		if(Is64BitsOs()) // Test with real system32 directory.
		{
			HMODULE hModule = LoadWow64Function();
			if(hModule)
			{
				PVOID OldValue = NULL;
				if(GFnWow64DisableWow64FsRedirection(&OldValue))
				{
					att = GetFileAttributes(buffer);
					if(att != INVALID_FILE_ATTRIBUTES)
					{
						nError = (INT)ShellExecute(0, _T("open"), buffer, param, 0, SW_NORMAL);
						if(nError <= 32)
						{
							printx("ShellExecute failed! Error code: %d.\n", nError);
							nError = 0;
						}
					}
					GFnWow64RevertWow64FsRedirection(OldValue);
					FreeWow64Function(hModule);
					return nError;
				}
				FreeWow64Function(hModule);
			}
		}
		return FALSE;
	}

	nError = (INT)ShellExecute(0, _T("open"), buffer, param, 0, SW_NORMAL);
	if(nError <= 32)
		printx("ShellExecute failed! Error code: %d.\n", nError);

	return TRUE;
}


const CHAR *pErrorString1 = "OpenSCManager failed! ec: %d.\n";
const CHAR *pErrorString2 = "OpenService failed! Name: %S ec: %d.\n";
const CHAR *pErrorString3 = "%s \"%S\" failed! ec: %d.\n";
const CHAR *pInfoString = "%s \"%S\" successfully.\n";

void InstallService(const TCHAR *pPath, TCHAR *pName)
{
	CHAR *pFunctionName = "InstallService";
	SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);

	if(!hSCManager)
	{
		printx(pErrorString1, GetLastError());
		return;
	}

	SC_HANDLE schService = CreateService
	(
		hSCManager,		/* SCManager database      */
		pName,			/* name of service         */
		pName,			/* service name to display */
		SERVICE_ALL_ACCESS,      /* desired access */
		SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS, /* service type */
		SERVICE_AUTO_START,      /* start type           */
		SERVICE_ERROR_NORMAL,    /* error control type   */
		pPath,			/* service's binary        */
		NULL,           /* no load ordering group  */
		NULL,           /* no tag identifier       */
		NULL,           /* no dependencies         */
		NULL,           /* LocalSystem account     */
		NULL			/* no password             */
	);

	if(schService)
	{
		printx(pInfoString, pFunctionName, pName);
		CloseServiceHandle(schService);
	}
	else
		printx(pErrorString3, pFunctionName, pName, GetLastError());

	CloseServiceHandle(hSCManager);
}

void UninstallService(TCHAR *pName)
{
	CHAR *pFunctionName = "UninstallService";
	SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);

	if(!hSCManager)
	{
		printx(pErrorString1, GetLastError());
		return;
	}

	SC_HANDLE schService = OpenService(hSCManager, pName, SERVICE_ALL_ACCESS);
	if(!schService)
	{
		printx(pErrorString2, pName, GetLastError());
	}
	else
	{
		if(DeleteService(schService))
			printx(pInfoString, pFunctionName, pName);
		else
			printx(pErrorString3, pFunctionName, pName, GetLastError());

		CloseServiceHandle(schService);
	}

	CloseServiceHandle(hSCManager);
}

BOOL StartService(TCHAR *pName)
{
	BOOL bResult = FALSE;
	CHAR *pFunctionName = "StartService";
	SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);

	if(!hSCManager)
	{
		printx(pErrorString1, GetLastError());
		return FALSE;
	}

	SC_HANDLE schService = OpenService(hSCManager, pName, SERVICE_ALL_ACCESS);
	if(!schService)
	{
		printx(pErrorString2, pName, GetLastError());
	}
	else
	{
		if(StartService(schService, 0, 0))
		{
			bResult = TRUE;
			printx(pInfoString, pFunctionName, pName);
		}
		else
			printx(pErrorString3, pFunctionName, pName, GetLastError());

		CloseServiceHandle(schService);
	}

	CloseServiceHandle(hSCManager);

	return bResult;
}

BOOL StopService(TCHAR *pName)
{
	BOOL bResult = FALSE;
	CHAR *pFunctionName = "StopService";
	SC_HANDLE hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CREATE_SERVICE);

	if(!hSCManager)
	{
		printx(pErrorString1, GetLastError());
		return bResult;
	}

	SC_HANDLE schService = OpenService(hSCManager, pName, SERVICE_ALL_ACCESS);
	if(!schService)
	{
		printx(pErrorString2, pName, GetLastError());
	}
	else
	{
		SERVICE_STATUS status;
		if(ControlService(schService, SERVICE_CONTROL_STOP, &status))
		{
			printx(pInfoString, pFunctionName, pName);
			bResult = TRUE;
		}
		else
			printx(pErrorString3, pFunctionName, pName, GetLastError());

		CloseServiceHandle(schService);
	}

	CloseServiceHandle(hSCManager);

	return bResult;
}

#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
LONG WINAPI ExceptionFilter(__in  struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	HANDLE lhDumpFile = CreateFile(_T("DumpFile.dmp"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
	loExceptionInfo.ExceptionPointers = ExceptionInfo;
	loExceptionInfo.ThreadId = GetCurrentThreadId();
	loExceptionInfo.ClientPointers = TRUE;
	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), lhDumpFile, MiniDumpNormal, &loExceptionInfo, NULL, NULL);
	CloseHandle(lhDumpFile);

	switch(ExceptionInfo->ExceptionRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
			break;
	}

	AfxMessageBox(_T("Critical error occurred!"));

//	return EXCEPTION_CONTINUE_SEARCH;
//	return EXCEPTION_CONTINUE_EXECUTION;

#ifdef DEBUG
	return EXCEPTION_CONTINUE_SEARCH;
#else
	return EXCEPTION_EXECUTE_HANDLER; // Terminate process silently.
#endif
}


CVPNClientApp theApp;
CFont GBoldFont;
CLimitSingleInstance GMutex(APP_STRING(ASI_SINGLE_INSTANCE_MUTEX_NAME));


BEGIN_MESSAGE_MAP(CVPNClientApp, CWinAppEx)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


CVPNClientApp::CVPNClientApp()
{
	m_bDuplicateInstance = FALSE;
	m_bHasConsole = FALSE;
	m_pServiceName = WINDOWS_SERVICE_NAME;
}

BOOL CVPNClientApp::InitInstance()
{
	SetUnhandledExceptionFilter(ExceptionFilter);

	// InitCommonControlsEx() is required on Windows XP if an application manifest specifies use of ComCtl32.dll
	// version 6 or later to enable visual styles. Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

//	WSADATA wsaData;
//	WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(!AfxSocketInit())
	{
		AfxMessageBox(_T("AfxSocketInit() failed!"));
		return FALSE;
	}

#ifdef DEBUG
	m_bHasConsole = AllocConsole();
#endif

	SetRegistryKey(_T("DigiStar Studio"));
	free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(_T("nMatrixClient")); // Override default behavior.

	//printx(_T("Command line: %s\n"), GetCommandLine());
	BOOL bGUI = FALSE;
	CString param(GetCommandLine());
	param.MakeLower();
	if(param.Find(_T("-svc")) != -1)
	{
		GMutex.Release();
		CNTServiceCommandLineInfo cmdInfo;
		CMyService Service(m_pServiceName);
	//	Service.ParseCommandLine(cmdInfo);
		Service.ProcessShellCommand(cmdInfo);
		return FALSE;
	}
	if(param.Find(_T("-testsvc")) != -1)
	{
		GMutex.Release();
		CnMatrixCore core;
		core.Init(0);
		core.Run();
		return FALSE;
	}
	if(param.Find(_T("-gui")) != -1)
		bGUI = TRUE;

	AfxEnableControlContainer();
	AfxInitRichEdit();

	if(GMutex.IsAnotherInstanceRunning())
	{
		HWND hWnd = FindWindow(0, WINDOW_TITLE);
		if(hWnd)
		{
			ShowWindow(hWnd, SW_RESTORE);
			SetForegroundWindow(hWnd);
		}
		m_bDuplicateInstance = TRUE;
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size of your final executable, you should remove from the following
	// the specific initialization routines you do not need. Change the registry key under which our settings are stored.

// 2013/07/16 test code.
//	CSetupDialog SetupDialog;
//	if(SetupDialog.DoModal() == IDCANCEL /*&& !(GetKeyState(VK_F2) < 0)*/ || !SetupDialog.IsSetupSuccessful())
//		return FALSE;


	BOOL bRegEntry = SetAdapterRegInfo(0, 0, 0), bAdapterDisabled = FALSE;

	if(!FindAdapter())
	{
		if(bRegEntry)
		{
			bAdapterDisabled = TRUE;
		}
		else
		{
			CSetupDialog SetupDialog;
			if(SetupDialog.DoModal() == IDCANCEL /*&& !(GetKeyState(VK_F2) < 0)*/ || !SetupDialog.IsSetupSuccessful())
				return FALSE;
			if(!FindAdapter())
				bAdapterDisabled = TRUE;
		}
	}
	else
	{
		// Check driver version.
		SetGUILanguage(ReadLanListID());
		CSetupDialog SetupDialog;
		if(!SetupDialog.CheckDriverVersion())
		{
			AfxMessageBox(GUILoadString(IDS_STRING1501));
			return FALSE;
		}
	}
	if(bAdapterDisabled)
	{
		AfxMessageBox(_T("Adapter disabled!"));
		return FALSE;
	}

	//VerifyEmbeddedSignature();

	CVPNClientDlg dlg;
	m_pMainWnd = &dlg;
	if(bGUI)
		dlg.EnableInitWindowHook();
	dlg.DoModal();

	return FALSE;
}

int CVPNClientApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class.
	if(!m_bDuplicateInstance)
	{
		//HANDLE hDevice = OpenAdapter();
		//CloseUDP(hDevice);
		//CloseHandle(hDevice);
	}

	if(m_bHasConsole)
		FreeConsole();

	return CWinAppEx::ExitInstance();
}

BOOL CVPNClientApp::InstallService(const TCHAR *path)
{
	::InstallService(path, m_pServiceName);
	return TRUE;
}

BOOL CVPNClientApp::DeleteService()
{
	::UninstallService(m_pServiceName);
	return TRUE;
}

BOOL CVPNClientApp::StartService()
{
	::StartService(m_pServiceName);
	return TRUE;
}

BOOL CVPNClientApp::StopService()
{
	::StopService(m_pServiceName);
	return TRUE;
}

TCHAR *GClientInfo = _T("ClientInfo");
TCHAR *GServerMsg  = _T("ServerMsg");

TCHAR *GConfig     = _T("config");
TCHAR *GLanguage   = _T("Language");
TCHAR *GLocalName  = _T("Name");
TCHAR *GTAddress   = _T("TAddress");
TCHAR *GTPort      = _T("TPort");
TCHAR *GKTTime     = _T("KTTime");
TCHAR *GDataIn     = _T("DataIn");
TCHAR *GDataOut    = _T("DataOut");
TCHAR *GFlag       = _T("Flag");
TCHAR *GSvcLogin   = _T("SvcLogin");

void CVPNClientApp::ReadConfigData(stConfigData *pConfig)
{
	TCHAR *pEntryName = GConfig;

	pConfig->LanguageID = GetProfileInt(pEntryName, GLanguage, 0);

	CString LocalName = GetProfileString(pEntryName, GLocalName);
	INT len = LocalName.GetLength();
	if(len > MAX_HOST_NAME_LENGTH)
		len = MAX_HOST_NAME_LENGTH;
	memcpy(pConfig->LocalName, LocalName.GetBuffer(), len * sizeof(TCHAR));
	pConfig->LocalName[len] = 0;

	pConfig->UDPTunnelAddress = GetProfileInt(pEntryName, GTAddress, 0);
	pConfig->UDPTunnelPort = (USHORT)GetProfileInt(pEntryName, GTPort, 0);
	pConfig->KeepTunnelTime = GetProfileInt(pEntryName, GKTTime, 0);
	pConfig->DataInLimit = GetProfileInt(pEntryName, GDataIn, 0);
	pConfig->DataOutLimit = GetProfileInt(pEntryName, GDataOut, 0);

	INT iFlag = GetProfileInt(pEntryName, GFlag, 0);

	pConfig->bAutoStart = (iFlag >> 0) & 0x01;
	pConfig->bAutoReconnect = (iFlag >> 1) & 0x01;
	pConfig->bAutoUpdate = (iFlag >> 2) & 0x01;
	pConfig->bAutoSetFirewall = (iFlag >> 3) & 0x01;
}

UINT CVPNClientApp::ReadLanListID()
{
	TCHAR *pEntryName = GConfig;
	return GetProfileInt(pEntryName, GLanguage, 0);
}

void CVPNClientApp::SaveConfigData(stConfigData *pConfig)
{
	TCHAR *pEntryName = GConfig;

	WriteProfileInt(pEntryName, GLanguage, pConfig->LanguageID);
	WriteProfileString(pEntryName, GLocalName, pConfig->LocalName);
	WriteProfileInt(pEntryName, GTAddress, pConfig->UDPTunnelAddress);
	WriteProfileInt(pEntryName, GTPort, pConfig->UDPTunnelPort);
	WriteProfileInt(pEntryName, GKTTime, pConfig->KeepTunnelTime);
	WriteProfileInt(pEntryName, GDataIn, pConfig->DataInLimit);
	WriteProfileInt(pEntryName, GDataOut, pConfig->DataOutLimit);

	INT iFlag = 0;
	iFlag |= (pConfig->bAutoStart << 0);
	iFlag |= (pConfig->bAutoReconnect << 1);
	iFlag |= (pConfig->bAutoUpdate << 2);
	iFlag |= (pConfig->bAutoSetFirewall << 3);

	WriteProfileInt(pEntryName, GFlag, iFlag);
}

void CVPNClientApp::SaveClosedMsgID(UINT nID)
{
	WriteProfileInt(GClientInfo, GServerMsg, nID);
}

UINT CVPNClientApp::LoadClosedMsgID()
{
	return GetProfileInt(GClientInfo, GServerMsg, 0);
}

void CVPNClientApp::SaveSvcLoginState(BOOL bOnline)
{
	INT i = bOnline ? 1 : 0;
	WriteProfileInt(GConfig, GSvcLogin, i);
}

BOOL CVPNClientApp::LoadSvcLoginState()
{
	return GetProfileInt(GConfig, GSvcLogin, 0);
}


CMyService::CMyService(TCHAR *pServiceName)
: CNTService(_T("nMatrix"), _T("nMatrix"), SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_POWEREVENT | SERVICE_ACCEPT_SHUTDOWN /*| SERVICE_ACCEPT_PAUSE_CONTINUE*/, pServiceName)
{
	// Simple boolean which is set to request the service to stop.
	m_bWantStop = FALSE;
	m_bPaused = FALSE;
	m_dwBeepInternal = 2000;
}

void CMyService::ServiceMain(DWORD /*dwArgc*/, LPTSTR* /*lpszArgv*/)
{
	RegisterCtrlHandlerEx();

	// Pretend that starting up takes some time.
	ReportStatusToSCM(SERVICE_START_PENDING, NO_ERROR, 0, 1, 0);
//	Sleep(1000);

	m_nMatrixCore.Init(0);
	ReportStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 1, 0);

	// Report to the event log that the service has started successfully.
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_STARTED, m_sDisplayName);
/*
	// The tight loop which constitutes the service.
	BOOL bOldPause = m_bPaused;
	while (!m_bWantStop)
	{
		// As a demo, we just do a message beep.
		if(!m_bPaused)
			MessageBeep(0xFFFFFFFF);

		Sleep(m_dwBeepInternal);

		if(m_bPaused != bOldPause) // SCM has requested a Pause / Continue.
		{
			if(m_bPaused)
			{
				ReportStatusToSCM(SERVICE_PAUSED, NO_ERROR, 0, 1, 0);
				// Report to the event log that the service has paused successfully.
				m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_PAUSED, m_sDisplayName);
			}
			else
			{
				ReportStatusToSCM(SERVICE_RUNNING, NO_ERROR, 0, 1, 0);
				// Report to the event log that the service has stopped continued.
				m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_CONTINUED, m_sDisplayName);
			}
		}

		bOldPause = m_bPaused;
	}
*/

	m_nMatrixCore.Run();


	// Pretend that closing down takes some time.
	ReportStatusToSCM(SERVICE_STOP_PENDING, NO_ERROR, 0, 1, 0);
//	Sleep(1000);
	ReportStatusToSCM(SERVICE_STOPPED, NO_ERROR, 0, 1, 0);

	// Report to the event log that the service has stopped successfully.
	m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, CNTS_MSG_SERVICE_STOPPED, m_sDisplayName);
}

void CMyService::OnStop()
{
	CSingleLock l(&m_CritSect, TRUE); // Synchronise access to the variables.

	m_nMatrixCore.Close();

	// Change the current state to STOP_PENDING
	m_dwCurrentState = SERVICE_STOP_PENDING;

	m_bWantStop = TRUE;
}

void CMyService::OnPause()
{
	CSingleLock l(&m_CritSect, TRUE); // Synchronise access to the variables.

	m_dwCurrentState = SERVICE_PAUSE_PENDING;

	m_bPaused = TRUE;
}

void CMyService::OnContinue()
{
	CSingleLock l(&m_CritSect, TRUE); // Synchronise access to the variables.

	m_dwCurrentState = SERVICE_CONTINUE_PENDING;

	m_bPaused = FALSE;
}

void CMyService::OnShutdown()
{
	printx("---> SVC: OnShutdown.\n");
	m_nMatrixCore.Close();
}

void CMyService::OnUserDefinedRequestEx(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)
{
	//// Any value greater than 200 increments the doubles the beep frequency otherwise the frequency is halved.
	//if (dwControl > 200)
	//	m_dwBeepInternal /= 2;
	//else
	//	m_dwBeepInternal *= 2;

	//// Report to the event log that the beep interval has been changed.
	//CString sInterval;
	//sInterval.Format(_T("%d"), m_dwBeepInternal);
	//m_EventLogSource.Report(EVENTLOG_INFORMATION_TYPE, MSG_SERVICE_SET_FREQUENCY, sInterval);

	if(dwControl == SERVICE_CONTROL_POWEREVENT)
	{
		//printx("Power event notified!\n");
		switch(dwEventType)
		{
			case PBT_APMSUSPEND:
				AddJobSystemPowerEvent(FALSE);
				//printx("System enter suspended state!\n");
				break;

			case PBT_APMRESUMEAUTOMATIC:
				AddJobSystemPowerEvent(TRUE);
				//printx("System resumed!\n");
				break;
		}
	}
}

void CMyService::ShowHelp()
{
//	AfxMessageBox(_T("A demo service which just beeps the speaker\nUsage: testsrv [-install | -uninstall | -help]\n"));
}


