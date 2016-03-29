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
#include "Resource.h"
#include "VPN ClientDlg.h"
#include "HostInfoDialog.h"


CHostInfoDialog::CHostInfoDialog(CWnd* pParent /*=NULL*/)
: CDialog(CHostInfoDialog::IDD, pParent)
{
	m_pManager = NULL;
	m_pVLanInfo = NULL;
	m_pMember = NULL;
}

CHostInfoDialog::~CHostInfoDialog()
{
	printx("Enter CHostInfoDialog::~CHostInfoDialog().\n");
}

void CHostInfoDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


IMPLEMENT_DYNAMIC(CHostInfoDialog, CDialog)
BEGIN_MESSAGE_MAP(CHostInfoDialog, CDialog)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CHostInfoDialog::OnBnClickedOk)
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


void CHostInfoDialog::PostNcDestroy()
{
	delete this;
}

void CHostInfoDialog::OnCancel()
{
	// Prevent closing dialog when pressing Esc.
	SafeClose();
//	CDialog::OnCancel();
}

BOOL CHostInfoDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	IPV4 ip = m_pMember->vip;
	CString string;
	string.Format(APP_STRING(ASI_IPV4_FORMAT), ip.b1, ip.b2, ip.b3, ip.b4);
	SetDlgItemText(IDC_VIP, string);

	UpdataData();
	SetTimer(TIMER_ID, UPDATE_TIME_INTERVAL, 0);

	if(m_pVLanInfo->IsOwner())
	{
	}

	SetWindowText(GUILoadString(IDS_HOST_INFO));

	SetDlgItemText(IDC_S_HOST_NAME, GUILoadString(IDS_HOST_NAME) + csGColon);
	SetDlgItemText(IDC_S_VIRTUAL_IP, GUILoadString(IDS_VIRTUAL_IP) + csGColon);
	SetDlgItemText(IDC_S_TUNNEL_ADDRESS, GUILoadString(IDS_TUNNEL_ADDRESS) + csGColon);
	SetDlgItemText(IDC_S_DATA_IN, GUILoadString(IDS_DATA_IN) + csGColon);
	SetDlgItemText(IDC_S_DATA_OUT, GUILoadString(IDS_DATA_OUT) + csGColon);
	SetDlgItemText(IDOK, GUILoadString(IDS_CLOSE));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CHostInfoDialog::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == TIMER_ID)
		UpdataData();

	CDialog::OnTimer(nIDEvent);
}

void CHostInfoDialog::OnBnClickedOk()
{
	SafeClose();
//	DestroyWindow(); // Can't use EndDialog() here.
//	OnOK();
}

void CHostInfoDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if(nID == SC_CLOSE)
	{
		SafeClose();
	//	DestroyWindow(); // Can't use EndDialog() here.
		return;
	}

	CDialog::OnSysCommand(nID, lParam);
}

void AddDot(CString &string)
{
	INT i, count = (string.GetLength() - 1) / 3;

	for(i = 0; i < count; ++i)
		string.Insert(string.GetLength() - (i + 1) * 3 - i, ',');
}

void CHostInfoDialog::SafeClose()
{
	m_pManager->Close(this);
}

void CHostInfoDialog::UpdataData()
{
	CString string;
	DWORD DataCount;
	stTrafficTable *pTrafficTable = m_pManager->GetTrafficTable();
	USHORT DriverMapIndex = m_pMember->DriverMapIndex, LinkState = m_pMember->LinkState;
	ASSERT((DriverMapIndex < MAX_NETWORK_CLIENT || DriverMapIndex == INVALID_DM_INDEX) && LinkState != LS_OFFLINE);

	CString Info = GUILoadString(IDS_NO_DATA), Info2 = GUILoadString(IDS_RELAYED_TUNNEL);

	if(m_csHostName != m_pMember->HostName)
	{
		m_csHostName = m_pMember->HostName;
		SetDlgItemText(IDC_HOSTNAME, m_csHostName);
	}

	if(DriverMapIndex == INVALID_DM_INDEX)
		ASSERT(LinkState == LS_NO_CONNECTION || LinkState == LS_TRYING_TPT);

	switch(LinkState)
	{
		case LS_OFFLINE:
			break;

		case LS_NO_CONNECTION:
			SetDlgItemText(IDC_TUNNEL_IP, GUILoadString(IDS_NO_CONNECTION));
			SetDlgItemText(IDC_DATA_IN, Info);
			SetDlgItemText(IDC_DATA_OUT, Info);
			return;

		case LS_NO_TUNNEL:
			SetDlgItemText(IDC_TUNNEL_IP, GUILoadString(IDS_NO_TUNNEL));
			SetDlgItemText(IDC_DATA_IN, Info);
			SetDlgItemText(IDC_DATA_OUT, Info);
			return;

		case LS_TRYING_TPT:
			SetDlgItemText(IDC_TUNNEL_IP, GUILoadString(IDS_TRYING_TPT));
			SetDlgItemText(IDC_DATA_IN, Info);
			SetDlgItemText(IDC_DATA_OUT, Info);
			return;

		case LS_RELAYED_TUNNEL:
			SetDlgItemText(IDC_TUNNEL_IP, Info2);
			break;

		case LS_SERVER_RELAYED:
			SetDlgItemText(IDC_TUNNEL_IP, Info2);
		//	SetDlgItemText(IDC_TUNNEL_IP, _T("Server relayed tunnel"));
			break;

		case LS_CONNECTED:
			IPV4 ip = m_pMember->eip.v4;
			string.Format(_T("%d.%d.%d.%d : %d"), ip.b1, ip.b2, ip.b3, ip.b4, m_pMember->eip.m_port);
			SetDlgItemText(IDC_TUNNEL_IP, string);
			break;
	}

	DataCount = pTrafficTable->DataIn[DriverMapIndex];
	string.Format(_T("%u"), DataCount);
	AddDot(string);
	string += _T(" Bytes");
	SetDlgItemText(IDC_DATA_IN, string);

	DataCount = pTrafficTable->DataOut[DriverMapIndex];
	string.Format(_T("%u"), DataCount);
	AddDot(string);
	string += _T(" Bytes");
	SetDlgItemText(IDC_DATA_OUT, string);
}


void CHostInfoManager::ShowInfo(stGUIVLanInfo *pVLanInfo, stGUIVLanMember *pMember)
{
	CHostInfoDialog *pDlg = NULL;
	POSITION pos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos; pDlg = NULL)
	{
		pDlg = m_list.GetNext(pos);

		if(pDlg->m_pMember->dwUserID == pMember->dwUserID)
			break;
	}

	if(pDlg != NULL)
	{
		m_cs.LeaveCriticalSection();
		pDlg->SetForegroundWindow();
		return;
	}

	pDlg = new CHostInfoDialog;
	m_list.AddTail(pDlg);
	m_cs.LeaveCriticalSection();

	ASSERT(m_pParentWnd != NULL);
	pDlg->SetManager(this);
	pDlg->SetVLanMember(pVLanInfo, pMember);
	pDlg->Create(CHostInfoDialog::IDD, m_pParentWnd);
	pDlg->ShowWindow(SW_SHOW);
}

void CHostInfoManager::Close(DWORD NetID, DWORD UID)
{
	CHostInfoDialog *pDlg = NULL;
	POSITION pos, oldpos;

	m_cs.EnterCriticalSection();
	for(pos = m_list.GetHeadPosition(); pos; pDlg = NULL)
	{
		oldpos = pos;
		pDlg = m_list.GetNext(pos);

		if(pDlg->m_pVLanInfo->NetIDCode != NetID)
			continue;

		if(UID)
		{
			if(pDlg->m_pMember->dwUserID == UID)
			{
				m_list.RemoveAt(oldpos);
				pDlg->DestroyWindow();
				break;
			}
		}
		else
		{
			m_list.RemoveAt(oldpos);
			pDlg->DestroyWindow();
		}
	}
	m_cs.LeaveCriticalSection();
}

void CHostInfoManager::Close(DWORD UID)
{
	CHostInfoDialog *pDlg = NULL;
	POSITION pos, oldpos;

	m_cs.EnterCriticalSection();
	for(pos = m_list.GetHeadPosition(); pos; pDlg = NULL)
	{
		oldpos = pos;
		pDlg = m_list.GetNext(pos);

		if(pDlg->m_pMember->dwUserID == UID)
		{
			m_list.RemoveAt(oldpos);
			break;
		}
	}
	m_cs.LeaveCriticalSection();

	if(pDlg)
		pDlg->DestroyWindow();
}

void CHostInfoManager::Close(CHostInfoDialog *pDlg)
{
	CHostInfoDialog *pDlgTemp;
	POSITION pos, oldpos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pDlgTemp = m_list.GetNext(pos);

		if(pDlgTemp == pDlg)
		{
			m_list.RemoveAt(oldpos);
			pDlgTemp->DestroyWindow();
			break;
		}
	}

	m_cs.LeaveCriticalSection();
}


void CHostInfoManager::CloseAll()
{
	POSITION pos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos;)
		m_list.GetNext(pos)->DestroyWindow();
	m_list.RemoveAll();

	m_cs.LeaveCriticalSection();
}

/*
void* CHostInfoManager::GetTrafficTable()
{
}
*/


