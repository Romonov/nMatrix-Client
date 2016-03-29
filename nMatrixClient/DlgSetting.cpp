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
#include "DlgSetting.h"
#include "SetupDialog.h"


#pragma comment(lib, "Version.lib")


CDlgSettingBasic::CDlgSettingBasic(CWnd* pParent /*=NULL*/)
:CDialog(CDlgSetting::IDD, pParent)
{
}

CDlgSettingBasic::~CDlgSettingBasic()
{
}

void CDlgSettingBasic::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_HOSTNAME, m_LocalName, sizeof(m_LocalName));

	INT n;
	if(pDX->m_bSaveAndValidate)
	{
		m_LanListIndex = ((CComboBox*)GetDlgItem(IDC_COMBO))->GetCurSel();

		CString text;
		DDX_IPAddress(pDX, IDC_UDP_TUNNEL_IP, m_UDPAddress);
		GetDlgItemText(IDC_UDP_PORT, text);
		if(text != _T(""))
		{
			DDX_Text(pDX, IDC_UDP_PORT, m_UDPPort);
			DDV_MinMaxInt(pDX, m_UDPPort, 0, 65535); // The dynamic or private ports are those from 49152 through 65535.
		}
		else
			m_UDPPort = 0;

		GetDlgItemText(IDC_KT_TIME, text);
		if(text != _T(""))
		{
			DDX_Text(pDX, IDC_KT_TIME, m_KeepTunnelTime);
			DDV_MinMaxInt(pDX, m_KeepTunnelTime, 0, 5);
		}
		else
			m_KeepTunnelTime = 0;

		DDX_Check(pDX, IDC_CHECK_AUTOSTART, n);
		m_bAutoStart = n ? true : false;

		DDX_Check(pDX, IDC_CHECK_AUTOUPDATE, n);
		m_bAutoUpdate = n ? true : false;

		DDX_Check(pDX, IDC_CHECK_AUTO_SETFW, n);
		m_bAutoSetFirewall = n ? true : false;
	}
	else // Init dialog.
	{
		if(m_UDPAddress)
			DDX_IPAddress(pDX, IDC_UDP_TUNNEL_IP, m_UDPAddress);
		if(m_UDPPort)
			DDX_Text(pDX, IDC_UDP_PORT, m_UDPPort);
		if(m_KeepTunnelTime)
			DDX_Text(pDX, IDC_KT_TIME, m_KeepTunnelTime);

		n = m_bAutoStart;
		DDX_Check(pDX, IDC_CHECK_AUTOSTART, n);

		n = m_bAutoUpdate;
		DDX_Check(pDX, IDC_CHECK_AUTOUPDATE, n);

		n = m_bAutoSetFirewall;
		DDX_Check(pDX, IDC_CHECK_AUTO_SETFW, n);
	}
}


BEGIN_MESSAGE_MAP(CDlgSettingBasic, CDialog)
END_MESSAGE_MAP()


BOOL CDlgSettingBasic::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE))
			return 0;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CDlgSettingBasic::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetDlgItemText(IDC_GROUP, GUILoadString(IDS_BASIC));

	SetDlgItemText(IDC_GUI_LANG, GUILoadString(IDS_GUI_LANG) + csGColon);
	SetDlgItemText(IDC_S_HOSTNAME, GUILoadString(IDS_LOCAL_NAME) + csGColon);
	SetDlgItemText(IDC_TUNNELADDRESS, GUILoadString(IDS_UDP_TUNNEL_ADDRESS) + csGColon);

	SetDlgItemText(IDC_S_KT_TIME, GUILoadString(IDS_KT_TIME) + csGColon);

	SetDlgItemText(IDC_CHECK_AUTOSTART, GUILoadString(IDS_AUTOSTART));
	SetDlgItemText(IDC_CHECK_RECONNECT, GUILoadString(IDS_RECONNECT));
	SetDlgItemText(IDC_CHECK_AUTOUPDATE, GUILoadString(IDS_AUTOUPDATE));
	SetDlgItemText(IDC_CHECK_AUTO_SETFW, GUILoadString(IDS_AUTO_SETFW));

	CComboBox *pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO);
	for(UINT i = 0; i < LLI_TOTAL; ++i)
		pComboBox->AddString(GLanguageName[i]);
	pComboBox->SetCurSel(m_LanListIndex);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


CDlgSettingBandwidth::CDlgSettingBandwidth(CWnd* pParent /*=NULL*/)
:CDialog(CDlgSetting::IDD, pParent)
{
}

CDlgSettingBandwidth::~CDlgSettingBandwidth()
{
}

void CDlgSettingBandwidth::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Text(pDX, IDC_INGOING_RATE, m_DataInLimit);
	DDX_Text(pDX, IDC_OUTGOING_RATE, m_DataOutLimit);

	if(pDX->m_bSaveAndValidate)
	{
		m_DataInLimit = (m_DataInLimit > MAX_VALUE) ? MAX_VALUE : m_DataInLimit;
		m_DataOutLimit = (m_DataOutLimit > MAX_VALUE) ? MAX_VALUE : m_DataOutLimit;
	}
}


BEGIN_MESSAGE_MAP(CDlgSettingBandwidth, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_DATAIN, &CDlgSettingBandwidth::OnDeltaposSpinDatain)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_DATAOUT, &CDlgSettingBandwidth::OnDeltaposSpinDataout)
END_MESSAGE_MAP()


BOOL CDlgSettingBandwidth::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE))
			return 0;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CDlgSettingBandwidth::OnInitDialog()
{
	CDialog::OnInitDialog();

	GetDlgItem(IDC_GROUP)->SetWindowText(GUILoadString(IDS_BANDWIDTH));
	GetDlgItem(IDC_MAX_INGOING)->SetWindowText(GUILoadString(IDS_MAX_INGOING));
	GetDlgItem(IDC_MAX_OUTGOING)->SetWindowText(GUILoadString(IDS_MAX_OUTGOING));

	CSpinButtonCtrl *pSpinIn = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_DATAIN), *pSpinOut = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_DATAOUT);

	pSpinIn->SetRange32(MIN_VALUE, MAX_VALUE);
	pSpinOut->SetRange32(MIN_VALUE, MAX_VALUE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


CDlgSetting::CDlgSetting(CVPNClientDlg *pMainDlg, CnMatrixCore *pCore, stConfigData *pConfig)
:CDialog(CDlgSetting::IDD, pMainDlg), m_pMainDlg(pMainDlg), m_pCore(pCore), m_pConfig(pConfig)
{
	ASSERT(pConfig);
}

CDlgSetting::~CDlgSetting()
{
}

void CDlgSetting::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgSetting, CDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CATEGORY, &CDlgSetting::OnLvnItemchangedListCategory)
	ON_NOTIFY(NM_CLICK, IDC_LIST_CATEGORY, &CDlgSetting::OnNMClickListCategory)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_CATEGORY, &CDlgSetting::OnNMDblclkListCategory)
	ON_BN_CLICKED(IDOK, &CDlgSetting::OnBnClickedOk)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_CATEGORY, &CDlgSetting::OnNMRClickListCategory)
	ON_NOTIFY(NM_RDBLCLK, IDC_LIST_CATEGORY, &CDlgSetting::OnNMRDblclkListCategory)
END_MESSAGE_MAP()


BOOL CDlgSetting::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(GUILoadString(IDS_SETTINGS));
	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));
	GetDlgItem(IDCANCEL)->SetWindowText(GUILoadString(IDS_CANCEL));

	// Init data.
	m_DlgBasic.m_LanListIndex = m_pConfig->LanguageID;
	memcpy(m_DlgBasic.m_LocalName, m_pConfig->LocalName, sizeof(m_pConfig->LocalName));
	m_DlgBasic.m_UDPAddress = m_pConfig->UDPTunnelAddress;
	m_DlgBasic.m_KeepTunnelTime = m_pConfig->KeepTunnelTime;
	ChangeByteOrder(m_DlgBasic.m_UDPAddress);

	m_DlgBasic.m_UDPPort = m_pConfig->UDPTunnelPort;
	m_DlgBandwidth.m_DataInLimit = m_pConfig->DataInLimit;
	m_DlgBandwidth.m_DataOutLimit = m_pConfig->DataOutLimit;

	// Create dlg after init data.
	m_DlgBasic.m_bAutoStart = m_pConfig->bAutoStart;
	m_DlgBasic.m_bAutoReconnect = m_pConfig->bAutoReconnect;
	m_DlgBasic.m_bAutoUpdate = m_pConfig->bAutoUpdate;
	m_DlgBasic.m_bAutoSetFirewall = m_pConfig->bAutoSetFirewall;

	m_DlgBasic.Create(CDlgSettingBasic::IDD, this);
	m_DlgBandwidth.Create(CDlgSettingBandwidth::IDD, this);

	m_DlgBasic.SetWindowPos(0, 95, 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	m_DlgBandwidth.SetWindowPos(0, 95, 10, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	// Init list ctrl.
	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_LIST_CATEGORY);

	m_ImageList.Create(32, 32, ILC_COLOR8 | ILC_MASK, 3, 15);
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_BASIC));
	m_ImageList.Add(AfxGetApp()->LoadIcon(IDI_BANDWIDTH));
	pList->SetImageList(&m_ImageList, LVSIL_NORMAL);

	pList->InsertItem(LVIF_IMAGE | LVIF_TEXT | LVIF_STATE, 0, GUILoadString(IDS_BASIC), LVSIL_NORMAL | LVIS_SELECTED, -1, 0, 0);
	pList->InsertItem(1, GUILoadString(IDS_BANDWIDTH), 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


BOOL StartServiceMode(BOOL &bVersionOK)
{
	BOOL bResult = FALSE;
	CString csPath, csParam;

	if(SetupServiceFile(bVersionOK, csPath) && bVersionOK)
	{
		// Install service.
		csParam = csPath + _T(" -svc");
		((CVPNClientApp*)AfxGetApp())->InstallService(csParam);
		bResult = ((CVPNClientApp*)AfxGetApp())->StartService();

		AppAutoStart(_T("nMatrix"), csPath, _T("-gui"), TRUE);
	}

	return bResult;
}

struct stThreadContext
{
	stThreadContext(CVPNClientDlg *pDlg, CnMatrixCore *pnMatrixCore, CInfoDialog *pData1, stConfigData *pData2)
	:pMainDlg(pDlg), pCore(pnMatrixCore), pInfoDialog(pData1), pConfigData(pData2)
	{
	}

	CVPNClientDlg *pMainDlg;
	CnMatrixCore  *pCore;
	CInfoDialog   *pInfoDialog;
	stConfigData  *pConfigData;
};

DWORD WINAPI InitServiceThread(LPVOID lpParameter)
{
	CVPNClientDlg *pMainDlg = ((stThreadContext*)lpParameter)->pMainDlg;
	CnMatrixCore  *pnMatrixCore = ((stThreadContext*)lpParameter)->pCore;
	CInfoDialog   *pInfoDialog = ((stThreadContext*)lpParameter)->pInfoDialog;
	stConfigData  *pConfigData = ((stThreadContext*)lpParameter)->pConfigData;

	Sleep(THREAD_INIT_WAIT_TIME); // Wait info dialog first.

	BOOL bIsOnline = pMainDlg->IsLogin();
	if(bIsOnline)
	{
		AddJobLogin(LOR_GUI, 0, 0, TRUE); // Must logout before closing nmatrix core.
		UINT nTryCount = 100;
		while(nTryCount-- && pMainDlg->IsLogin())
			Sleep(10);
	}
	pnMatrixCore->Close(); // Close before initializing service. Prevent closing kernel socket of the service.

	BOOL bVersionOK;
	if(StartServiceMode(bVersionOK) && bVersionOK)
	{
		BOOL bConnected = FALSE;
		CMessagePipe *pPipe = new CMessagePipe;
		if(pPipe)
		{
			INT nTry = 100; // Wait 10 seconds at max.
			while(nTry--)
				if(pPipe->GetPipeObject()->Connect(APP_STRING(ASI_PIPE_NAME)))
				{
					bConnected = TRUE;
					break;
				}
				else
					Sleep(100);

			if(bConnected)
			{
		//		printx("Pipe connected!\n");
				BYTE buffer[sizeof(DWORD) * 2 + sizeof(stConfigData)];
				UINT nSize = AddJobUpdateConfig(pConfigData, FALSE, TRUE, buffer, sizeof(buffer)), nClosedMsgID = pMainDlg->GetClosedMsgID();
				pPipe->WriteMsg(buffer, nSize); // Save config.

				stRegisterID RegID;
				AppLoadRegisterID(&RegID);
				nSize = AddJobDataExchange(FALSE, DET_REG_ID, &RegID, TRUE, buffer, sizeof(buffer));
				pPipe->WriteMsg(buffer, nSize); // Save register ID.
				nSize = AddJobDataExchange(FALSE, DET_SERVER_MSG_ID, &nClosedMsgID, TRUE, buffer, sizeof(buffer));
				pPipe->WriteMsg(buffer, nSize); // Save closed msg ID.

				pnMatrixCore->Init(pMainDlg->GetSafeHwnd(), pPipe);
				pPipe = 0;
				if(bIsOnline)
					AddJobLogin(0, 0, 0, 0);
				pMainDlg->SetShellNotifyData(); // Update string.
			}
		}

		SAFE_DELETE(pPipe);
	}

	if(!pnMatrixCore->IsPureGUIMode())
		pnMatrixCore->Init(pMainDlg->GetSafeHwnd());

	//Sleep(1000);
	pInfoDialog->CloseDialog(bVersionOK ? 1 : 2);

	return 0;
}

void CloseServiceMode()
{
	((CVPNClientApp*)AfxGetApp())->StopService();
	((CVPNClientApp*)AfxGetApp())->DeleteService();
}

DWORD WINAPI UpdateServiceThread(LPVOID lpParameter)
{
	CVPNClientDlg *pMainDlg = ((stThreadContext*)lpParameter)->pMainDlg;
	CnMatrixCore  *pnMatrixCore = ((stThreadContext*)lpParameter)->pCore;
	CInfoDialog   *pInfoDialog = ((stThreadContext*)lpParameter)->pInfoDialog;

	Sleep(THREAD_INIT_WAIT_TIME); // Wait info dialog first.

	BOOL bIsOnline = pMainDlg->IsLogin();
	if(bIsOnline)
	{
		AddJobLogin(LOR_GUI, 0, 0, TRUE); // Must logout before closing nmatrix core.
		UINT nTryCount = 100;
		while(nTryCount-- && pMainDlg->IsLogin())
			Sleep(10);
	}
	pnMatrixCore->Close(); // Close before initializing service. Prevent closing kernel socket of the service.

	CloseServiceMode();

	BOOL bResult = FALSE, bVersionOK;
	if(StartServiceMode(bVersionOK) && bVersionOK)
	{
		BOOL bConnected = FALSE;
		CMessagePipe *pPipe = new CMessagePipe;
		if(pPipe)
		{
			INT nTry = 100; // Wait 10 seconds at max.
			while(nTry--)
				if(pPipe->GetPipeObject()->Connect(APP_STRING(ASI_PIPE_NAME)))
				{
					bConnected = TRUE;
					break;
				}
				else
					Sleep(100);

			if(bConnected)
			{
				pnMatrixCore->Init(pMainDlg->GetSafeHwnd(), pPipe);
				pPipe = 0;
				if(bIsOnline)
					AddJobLogin(0, 0, 0, 0);
				pMainDlg->SetShellNotifyData(); // Update string.
				bResult = TRUE;
			}
		}

		SAFE_DELETE(pPipe);
	}

	if(!pnMatrixCore->IsPureGUIMode())
		pnMatrixCore->Init(pMainDlg->GetSafeHwnd());

	//Sleep(1000);
	pInfoDialog->CloseDialog(bResult);

	return 0;
}

BOOL UpdateService(CVPNClientDlg *pMainDlg, CnMatrixCore *pnMatrixCore)
{
	CInfoDialog InfoDialog;
	InfoDialog.SetMsg(GUILoadString(IDS_STRING1512));

	stThreadContext ctx(pMainDlg, pnMatrixCore, &InfoDialog, 0);
	HANDLE hHandle = CreateThread(0, 0, UpdateServiceThread, &ctx, 0, 0);
	CloseHandle(hHandle);
	return InfoDialog.DoModal();
}

void CDlgSetting::OnBnClickedOk()
{
	BOOL m_bNeedRestartApp = FALSE, bInitService = FALSE;

	if(!m_DlgBasic.UpdateData())
		return;
	if(!m_DlgBandwidth.UpdateData())
		return;

	if(m_pConfig->LanguageID != m_DlgBasic.m_LanListIndex)
	{
		m_pConfig->LanguageID = m_DlgBasic.m_LanListIndex;
		SetGUILanguage(m_pConfig->LanguageID);
	}
	if(!m_pConfig->bAutoStart && m_DlgBasic.m_bAutoStart)
		bInitService = TRUE;
	if(m_pConfig->bAutoStart && !m_DlgBasic.m_bAutoStart) // Remove registry item of auto-run.
		AppAutoStart(_T("nMatrix"), 0, 0, FALSE);

	memcpy(m_pConfig->LocalName, m_DlgBasic.m_LocalName, sizeof(m_pConfig->LocalName));
	ChangeByteOrder(m_DlgBasic.m_UDPAddress);
	m_pConfig->UDPTunnelAddress = m_DlgBasic.m_UDPAddress;
	m_pConfig->UDPTunnelPort = m_DlgBasic.m_UDPPort;
	m_pConfig->KeepTunnelTime = m_DlgBasic.m_KeepTunnelTime;
	m_pConfig->bAutoStart = m_DlgBasic.m_bAutoStart;
	m_pConfig->bAutoReconnect = m_DlgBasic.m_bAutoReconnect;
	m_pConfig->bAutoUpdate = m_DlgBasic.m_bAutoUpdate;
	m_pConfig->bAutoSetFirewall = m_DlgBasic.m_bAutoSetFirewall;

	m_pConfig->DataInLimit = m_DlgBandwidth.m_DataInLimit;
	m_pConfig->DataOutLimit = m_DlgBandwidth.m_DataOutLimit;

	AddJobUpdateConfig(m_pConfig, FALSE);
	if(m_pCore->IsPureGUIMode())
		((CVPNClientApp*)AfxGetApp())->SaveConfigData(m_pConfig);

	if(bInitService && !m_pCore->IsPureGUIMode()) // Init service after updating configuration. This process will close nmatrix core.
	{
		CInfoDialog InfoDialog;
		InfoDialog.SetMsg(GUILoadString(IDS_STRING1511));
		stThreadContext ctx(m_pMainDlg, m_pCore, &InfoDialog, m_pConfig);
		HANDLE hHandle = CreateThread(0, 0, InitServiceThread, &ctx, 0, 0);
		CloseHandle(hHandle);
		if(InfoDialog.DoModal() == 2) // Service file is newer.
		{
			m_pConfig->bAutoStart = false;
			AddJobUpdateConfig(m_pConfig, FALSE);
			EndDialog(3);
			return;
		}
	}

//	if(m_bNeedRestartApp)
//		AfxMessageBox(GUILoadString(IDS_STRING1510), MB_ICONINFORMATION);

	//printx("On setting changed.\n");
	OnOK();
}

void CDlgSetting::ShowPanel(DWORD dwDlg)
{
	if(dwDlg == DLG_BASIC)
	{
		m_DlgBasic.ShowWindow(SW_NORMAL);
		m_DlgBandwidth.ShowWindow(SW_HIDE);
		m_pCurrentDlg = &m_DlgBasic;
	}
	else
	{
		ASSERT(dwDlg == DLG_BANDWIDTH);

		m_DlgBasic.ShowWindow(SW_HIDE);
		m_DlgBandwidth.ShowWindow(SW_NORMAL);
		m_pCurrentDlg = &m_DlgBandwidth;
	}
}

void CDlgSetting::UpdateSelectedCategory()
{
	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_LIST_CATEGORY);
	if(m_pCurrentDlg == &m_DlgBasic)
	{
		pList->SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	}
	else if(m_pCurrentDlg == &m_DlgBandwidth)
	{
		pList->SetItemState(1, LVIS_SELECTED, LVIS_SELECTED);
	}
}

void CDlgSetting::OnLvnItemchangedListCategory(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_LIST_CATEGORY);
	POSITION pos = pList->GetFirstSelectedItemPosition();

	if(pos)
	{
		INT nSel = pList->GetNextSelectedItem(pos);
		if(nSel == 0)
		{
			ShowPanel(DLG_BASIC);
		}
		else if(nSel == 1)
		{
			ShowPanel(DLG_BANDWIDTH);
		}
	}

	*pResult = 0;
}

void CDlgSetting::OnNMClickListCategory(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if(pNMItemActivate->iItem == -1)
		UpdateSelectedCategory();

	*pResult = 0;
}

void CDlgSetting::OnNMDblclkListCategory(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if(pNMItemActivate->iItem == -1)
		UpdateSelectedCategory();

	*pResult = 0;
}

void CDlgSetting::OnNMRClickListCategory(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if(pNMItemActivate->iItem == -1)
		UpdateSelectedCategory();

	*pResult = 0;
}

void CDlgSetting::OnNMRDblclkListCategory(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if(pNMItemActivate->iItem == -1)
		UpdateSelectedCategory();

	*pResult = 0;
}

void CDlgSettingBandwidth::OnDeltaposSpinDatain(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	*pResult = 0;
}

void CDlgSettingBandwidth::OnDeltaposSpinDataout(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	*pResult = 0;
}


