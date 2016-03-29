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
#include "DriverAPI.h"
#include "DebugToolDlg.h"


const TCHAR *DEBUG_PIPE_NAME = _T("\\\\.\\pipe\\nMatrixDbgTracer");


CDebugToolDlg::CDebugToolDlg(CWnd* pParent /*=NULL*/)
: CDialog(CDebugToolDlg::IDD, pParent)
{
	m_pNamedPipe = m_RemoteConsole.GetPipeObject();
}

CDebugToolDlg::~CDebugToolDlg()
{
}

void CDebugToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDebugToolDlg, CDialog)
	ON_BN_CLICKED(IDC_SET_REGID, &CDebugToolDlg::OnBnClickedSetRegid)
	ON_BN_CLICKED(IDC_GET_REGID, &CDebugToolDlg::OnBnClickedGetRegid)
	ON_BN_CLICKED(IDC_RESET_ADAPTER, &CDebugToolDlg::OnBnClickedResetAdapter)
	ON_BN_CLICKED(IDC_RENEW_ADAPTER, &CDebugToolDlg::OnBnClickedRenewAdapter)
	ON_BN_CLICKED(IDC_INSTALL_SERVICE, &CDebugToolDlg::OnBnClickedInstallService)
	ON_BN_CLICKED(IDC_DELETE_SERVICE, &CDebugToolDlg::OnBnClickedDeleteService)
	ON_BN_CLICKED(IDC_START_SERVICE, &CDebugToolDlg::OnBnClickedStartService)
	ON_BN_CLICKED(IDC_STOP_SERVICE, &CDebugToolDlg::OnBnClickedStopService)
	ON_BN_CLICKED(IDC_SHOW_CONSOLE, &CDebugToolDlg::OnBnClickedShowConsole)
	ON_BN_CLICKED(IDC_CREATE_PIPE, &CDebugToolDlg::OnBnClickedCreatePipe)
	ON_BN_CLICKED(IDC_START_LISTEN, &CDebugToolDlg::OnBnClickedStartListen)
	ON_BN_CLICKED(IDC_CLOSE_PIPE, &CDebugToolDlg::OnBnClickedClosePipe)
	ON_BN_CLICKED(IDC_PRINT_CONN_NAME, &CDebugToolDlg::OnBnClickedPrintConnName)
	ON_BN_CLICKED(IDC_READ, &CDebugToolDlg::OnBnClickedRead)
	ON_BN_CLICKED(IDC_WRITE, &CDebugToolDlg::OnBnClickedWrite)
	ON_BN_CLICKED(IDC_USER_MODE, &CDebugToolDlg::OnBnClickedUserMode)
	ON_BN_CLICKED(IDC_KERNEL_MODE, &CDebugToolDlg::OnBnClickedKernelMode)
	ON_BN_CLICKED(IDC_QUERY_TIME, &CDebugToolDlg::OnBnClickedQueryTime)
END_MESSAGE_MAP()


void CDebugToolDlg::OnBnClickedSetRegid()
{
	CString csRegID;
	GetDlgItemText(IDC_REGID, csRegID);
	if(csRegID == _T(""))
		return;

	if(csRegID.GetLength() != 32 + 3 || csRegID[8] != '-' || csRegID[17] != '-' || csRegID[26] != '-')
	{
		AfxMessageBox(_T("Format error! %08x-%08x-%08x-%08x"));
		return;
	}

	csRegID.Replace('-', ' ');

	stRegisterID RegID;
	_stscanf(csRegID, _T("%x %x %x %x"), &RegID.d1, &RegID.d2, &RegID.d3, &RegID.d4);

	AppSaveRegisterID(&RegID);

	AfxMessageBox(_T("Re-connect app to apply change."));

	EndDialog(0);
}

void CDebugToolDlg::OnBnClickedGetRegid()
{
	CString csRegID;
	stRegisterID RegID;
	AppLoadRegisterID(&RegID);

	csRegID.Format(_T("%08x-%08x-%08x-%08x"), RegID.d1, RegID.d2, RegID.d3, RegID.d4);
	SetDlgItemText(IDC_REGID, csRegID);
}

void CDebugToolDlg::OnBnClickedShowConsole()
{

	AddJobDebugTest();
//	((CVPNClientApp*)AfxGetApp())->ShowConsole();
//	AddJobDebugFunction(0, 0, 0);
//	AddJobDebugTest();
}

void CDebugToolDlg::OnBnClickedResetAdapter()
{
//	SetNT6NetworkName(_T("nMatrix Network"));
	return;

	//INetworkListManager *pNetworkListManager = NULL;
	//HRESULT hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager, (LPVOID *)&pNetworkListManager);

	//if(hr == S_OK)
	//{
	////	pNetworkListManager->


	//	CComPtr<IEnumNetworkConnections> pNetworkConnections;
	//	hr = pNetwork->GetNetworkConnections(&pNetworkConnections);
	//	if (SUCCEEDED(hr))
	//	{
	//		ShowNetworkConnections(pNetworkConnections);
	//	}
	//

	//	pNetworkListManager->Release();
	//}
}

void CDebugToolDlg::OnBnClickedRenewAdapter()
{
	//if(GetKeyState(VK_F2) < 0)
	//{
	//	AppGetNetManager()->WFWSetPort(TRUE, 8000, 0);
	//	printx("Open firewall port.\n");
	//}
	//else
	//{
	//	AppGetNetManager()->WFWSetPort(TRUE, 0, 8000);
	//	printx("Close firewall port.\n");
	//}

	//AppAutoStart(_T("nMatrixClient"), _T("-test"), 1);
	//AppAutoStart(_T("nMatrixClient"), 0, 0);

//	RenewAdapter(TRUE);

	//HANDLE hAdapter = OpenAdapter();
	//stDriverInternalState DriverState;
	//GetDriverState(hAdapter, &DriverState);

	//DWORD ip;
	//USHORT port;
	//BOOL bOpen;
	//GetUDPInfo(hAdapter, ip, port, bOpen);
	//CloseHandle(hAdapter);
}

void CDebugToolDlg::OnBnClickedPrintConnName()
{
//	SetNetworkConnectionName(_T(ADAPTER_DESC), _T(""));
}

void CDebugToolDlg::OnBnClickedRead()
{
//	printx("OnBnClickedRead.\n");

//	BYTE buffer[256];

//	HANDLE hDevice = OpenAdapter();
//	RegBuffer(hDevice, buffer, sizeof(buffer), FALSE);

//	DWORD dwReaded = 0;

	//printx("buffer address: %08x\n", buffer);
	//HRESULT hr = ReadFile(hDevice, buffer, 256, &dwReaded, 0);

	//printx("%s\n", buffer);
	//printx("dwReaded: %d, hr: %08x, Error code: %d\n", dwReaded, hr, GetLastError());
	//CloseHandle(hDevice);
}

void CDebugToolDlg::OnBnClickedWrite()
{
	printx("OnBnClickedWrite.\n");

	BYTE buffer[] = { 1, 3, 5, 7, 9, 11, 13, 15, 17, 19 };
	HANDLE hAdapter = OpenAdapter();

	DWORD dwWritten = 0;
	HRESULT hr = WriteFile(hAdapter, buffer, 2700, &dwWritten, 0);

	printx("dwWritten: %d, hr: %08x, Error code: %d\n", dwWritten, hr, GetLastError());

	CloseHandle(hAdapter);
}

void CDebugToolDlg::OnBnClickedQueryTime()
{
	AddJobQueryServerTime();
}

void CDebugToolDlg::OnBnClickedInstallService()
{
//	((CVPNClientApp*)AfxGetApp())->InstallService();
}

void CDebugToolDlg::OnBnClickedDeleteService()
{
//	((CVPNClientApp*)AfxGetApp())->DeleteService();
}

void CDebugToolDlg::OnBnClickedStartService()
{
//	((CVPNClientApp*)AfxGetApp())->StartService();
}

void CDebugToolDlg::OnBnClickedStopService()
{
//	((CVPNClientApp*)AfxGetApp())->StopService();
}

void CDebugToolDlg::OnBnClickedCreatePipe()
{
//	if(!m_pNamedPipe->CreateNamedPipe(DEBUG_PIPE_NAME))
//		printx("Failed to create named pipe.\n");
}

void CDebugToolDlg::OnBnClickedStartListen()
{
//	m_pNamedPipe->StartReader();
}

void CDebugToolDlg::OnBnClickedClosePipe()
{
//	m_pNamedPipe->Close(TRUE);
}

void CDebugToolDlg::OnBnClickedUserMode()
{
	BOOL bUseUMRecvBuffer = FALSE;

	if(((CButton*)GetDlgItem(IDC_CHECK_UMRB))->GetCheck() == BST_CHECKED)
		bUseUMRecvBuffer = TRUE;

	AddJobEnableUMAccess(TRUE, FALSE);
}

void CDebugToolDlg::OnBnClickedKernelMode()
{
	AddJobEnableUMAccess(FALSE, FALSE);
}


