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
#include "ChatDialog.h"
#include "CnMatrixCore.h"
#include "SetupDialog.h"
#include "VPN ClientDlg.h"
#include "CommonDlg.h"


extern CGUINetworkManager GNetworkManager;
CMenu GPopupMenu;


CUserListBox::CUserListBox()
{
}

CUserListBox::~CUserListBox()
{
}


BEGIN_MESSAGE_MAP(CUserListBox, CListBox)
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()


void CUserListBox::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	SendMessage(WM_LBUTTONDOWN, 0, MAKELONG(point.x, point.y));
	SendMessage(WM_LBUTTONUP, 0, MAKELONG(point.x, point.y));

	ClientToScreen(&point);

	UINT i, MenuItemCount = GPopupMenu.GetMenuItemCount();
	for(i = 0; i < MenuItemCount; ++i)
		GPopupMenu.DeleteMenu(0, MF_BYPOSITION); // Clean content of the menu.

	GPopupMenu.AppendMenu(MF_STRING|MF_ENABLED, CChatDialog::PMC_INVITE, GUILoadString(IDS_INVITE));

	if(GetCurSel() == LB_ERR)
	{
		printx("no selection!\n");
	}
	else
	{
		GPopupMenu.AppendMenu(MF_STRING|MF_ENABLED, CChatDialog::PMC_EVICT, GUILoadString(IDS_EVICT));
	}

	GPopupMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, GetParent(), 0);

	CListBox::OnRButtonUp(nFlags, point);
}


CChatDialog::CChatDialog(CWnd* pParent /*=NULL*/)
: CDialog(CChatDialog::IDD, pParent)
{
	m_pChatManager = NULL;

	m_bGroupChatMode = 0;
	m_bMinimized = 0;
	m_SessionIndex = MAX_GROUP_CHAT_SESSION;
	m_OldSessionIndex = MAX_GROUP_CHAT_SESSION;

	m_HostUID = 0;
	m_OwnerUID = 0;
	m_PeerUID = 0;
	m_VNetID = 0;

	ZeroMemory(m_UIDArray, sizeof(m_UIDArray));
}

CChatDialog::~CChatDialog()
{
	printx("---> CChatDialog::~CChatDialog()\n");
}

void CChatDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


//IMPLEMENT_DYNAMIC(CChatDialog, CDialog)
BEGIN_MESSAGE_MAP(CChatDialog, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_SIZING()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_MESSAGE(WM_INVALID_SESSION, OnInvalidSession)
	ON_COMMAND(PMC_EVICT, &CChatDialog::OnCommandEvict)
	ON_COMMAND(PMC_INVITE, &CChatDialog::OnCommandInvite)
END_MESSAGE_MAP()


BOOL CChatDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), TRUE);
	SetIcon(AfxGetApp()->LoadIcon(IDR_MAINFRAME), FALSE);

	INT width, height;
	if(m_bGroupChatMode)
	{
		SetWindowText(GUILoadString(IDS_GROUP_MSG));
		width = MIN_GROUP_CHAT_DIALOG_WIDTH;
		height = MIN_GROUP_CHAT_DIALOG_HEIGHT;

		m_UserList.SubclassDlgItem(IDC_USER_LIST, this);
	}
	else
	{
		width = MIN_CHAT_DIALOG_WIDTH;
		height = MIN_CHAT_DIALOG_HEIGHT;
	}

	CHARFORMAT fmt;
	ZeroMemory(&fmt, sizeof(fmt));
	fmt.cbSize = sizeof(fmt);
	fmt.dwMask = CFM_SIZE;
	fmt.yHeight = 188;

	CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);
	pEdit->SetDefaultCharFormat(fmt);
	pEdit->HideSelection(FALSE, TRUE);

	SetWindowPos(0, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChatDialog::PostNcDestroy()
{
	delete this;
}

BOOL CChatDialog::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if(pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_RETURN)
		{
			OnKeyEnterDown();
//			return 1;
		}
	//	if(pMsg->wParam == VK_ESCAPE)
	//		return 0;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CChatDialog::OnCancel()
{
	// Prevent closing dialog when pressing Esc.
	//printx("Enter CChatDialog::OnCancel().\n");
	//CDialog::OnCancel();
}

void CChatDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if(nID == SC_CLOSE)
	{
		SafeClose();
	//	DestroyWindow(); // Can't use EndDialog() here.
		return;
	}

	CDialog::OnSysCommand(nID, lParam);
}

void CChatDialog::OnKeyEnterDown()
{
	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_CHAT_EDIT);

	if(GetKeyState(VK_SHIFT) < 0)
	{
		pEdit->ReplaceSel(_T("\r\n"));
		return;
	}

	CString strTalk;
	pEdit->GetWindowText(strTalk);

	if(strTalk == _T(""))
		return;

	if(m_bGroupChatMode)
	{
		if(m_SessionIndex != MAX_GROUP_CHAT_SESSION)
			AddJobGroupChatSend(m_HostUID, m_SessionIndex, strTalk, strTalk.GetLength());
		else
			printx("Session closed!\n");
	}
	else
	{
		AddJobTextChat(strTalk.GetBuffer(), m_HostUID, m_PeerUID);
		AppendChatString(strTalk);
	}

	// Delete all of the text.
	pEdit->SetSel(0, -1);
	pEdit->Clear();
}

void CChatDialog::NotifyLeaveSession()
{
	if(m_bGroupChatMode && m_SessionIndex < MAX_GROUP_CHAT_SESSION)
	{
		if(m_HostUID == m_OwnerUID && GetKeyState(VK_F2) < 0) // Test code.
			AddJobGroupChatClose(m_HostUID, m_SessionIndex);
		else
			AddJobGroupChatLeave(m_HostUID, m_SessionIndex);
	}
}

void CChatDialog::SafeClose()
{
	//printx("CChatDialog::SafeClose\n");
	NotifyLeaveSession();
	m_pChatManager->Close(this);
}

void CChatDialog::AppendChatString(const TCHAR *pString)
{
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_HISTORY);
	INT nLine = pEdit->GetLineCount(), pos;

	pos = pEdit->LineIndex(nLine - 1);
	pEdit->SetSel(pos, pos);

	CString text;
	text.Format(_T("%s %s\n"), _T("Local:"), pString);
	pEdit->ReplaceSel(text);
}

void CChatDialog::ReceiveString(const TCHAR *pString)
{
	CString csChat;
	csChat.Format(_T("%s: %s\n"), m_HostName, pString);
//	LONG lStyles = GetWindowLong(m_hWnd, GWL_STYLE);
//	if(lStyles & WS_MINIMIZE)
	if(m_bMinimized)
	{
		m_csTempBuffer += csChat;
		return;
	}

	CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);
	INT pos = pEdit->GetTextLength();
	pEdit->SetSel(pos - 1, pos - 1);
	pEdit->ReplaceSel(csChat);
}

void CChatDialog::ReceiveString(DWORD uid, const TCHAR *pString)
{
	CString csChat;

	if(uid == m_HostUID)
		csChat.Format(_T("%s %s\n"), _T("Local: "), pString);
	else
		for(UINT i = 0; i < MAX_GROUP_CHAT_HOST; ++i)
			if(uid == m_UIDArray[i])
			{
				csChat.Format(_T("%s: %s\n"), m_PeerName[i], pString);
				break;
			}

	if(m_bMinimized)
	{
		m_csTempBuffer += csChat;
		return;
	}

	if(!csChat.IsEmpty())
	{
		CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);
		INT pos = pEdit->GetTextLength();
		pEdit->SetSel(pos - 1, pos - 1);
		pEdit->ReplaceSel(csChat);
	}
}

void CChatDialog::AddString(DWORD dwColor, const TCHAR *pString, UINT nLength)
{
	CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);

	if(!nLength)
		nLength = _tcslen(pString);

	CHARFORMAT cf;
	cf.cbSize = sizeof(cf);
	cf.dwMask = /*CFM_BOLD | */CFM_COLOR;
	cf.dwEffects = 0; //CFE_BOLD | ~CFE_AUTOCOLOR;
	cf.crTextColor = dwColor; // RGB(0, 255, 0)

	UINT spos = pEdit->GetTextLength();

	pEdit->SetSel(spos - 1, spos - 1);
	pEdit->ReplaceSel(pString);

	INT line = pEdit->GetLineCount();
	spos = pEdit->LineIndex(line - 2);

	INT endpos = pEdit->LineIndex(line - 1) - 1;

	pEdit->SetSel(spos, endpos);
	pEdit->SetSelectionCharFormat(cf);

	// Remove Black selection bars.
	spos = pEdit->GetTextLength();
	pEdit->SetSel(spos - 1, spos - 1);
}

void CChatDialog::DeleteLine()
{
	CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);
	INT line = pEdit->GetLineCount() - 1 - 1; // The last line is always empty.

	INT spos = pEdit->LineIndex(line) - 1;
	INT endpos = spos + pEdit->LineLength(line) + pEdit->LineLength(line + 1);

	pEdit->SetSel(spos, endpos);
	pEdit->SetReadOnly(FALSE);
	pEdit->Clear();
	pEdit->SetReadOnly(TRUE);
}

void CChatDialog::UpdateGUIList()
{
	CString cs;
	CListBox *pList = (CListBox*)GetDlgItem(IDC_USER_LIST);
	for(UINT i = 0; i < MAX_GROUP_CHAT_HOST; ++i)
	{
		if(!m_UIDArray[i])
			break;

		cs = _T(" ") + m_PeerName[i];
		pList->InsertString(i, cs);
		pList->SetItemData(i, m_UIDArray[i]);
	}

	pList->ShowWindow(SW_NORMAL);
}

void CChatDialog::CloseSession()
{
	if(m_SessionIndex < MAX_GROUP_CHAT_SESSION)
		AddString(RGB(255, 0, 0), _T("\nSession closed!\n"), 0);

	((CListBox*)GetDlgItem(IDC_USER_LIST))->ResetContent();
	ZeroMemory(m_UIDArray, sizeof(m_UIDArray));
	m_SessionIndex = MAX_GROUP_CHAT_SESSION;
}

void CChatDialog::ResumeSession()
{
	DeleteLine();
	m_SessionIndex = m_OldSessionIndex;
}

void CChatDialog::RemoveUser(DWORD dwUID)
{
	CString cs, csHostName;
	CListBox *pList = (CListBox*)GetDlgItem(IDC_USER_LIST);

	UINT i, nTotal = pList->GetCount();
	for(i = 0; i < nTotal; ++i)
		if(pList->GetItemData(i) == dwUID)
		{
			pList->DeleteString(i);
			break;
		}

	for(i = 0; i < MAX_GROUP_CHAT_HOST; ++i)
		if(m_UIDArray[i] == dwUID)
		{
			csHostName = m_PeerName[i];
			m_PeerName[i].Empty();
			m_UIDArray[i] = 0;
			break;
		}

	if(!csHostName.IsEmpty())
	{
		cs.Format(_T(" %s left the chat room.\n"), csHostName);
		AddString(RGB(255, 0, 0), cs, cs.GetLength());
	}
}

void CChatDialog::AddNewMember(CString csHostName, DWORD HostUID, BOOL bResume)
{
	CListBox *pList = (CListBox*)GetDlgItem(IDC_USER_LIST);

	UINT index = pList->InsertString(-1, csHostName);
	if(index != LB_ERR && index != LB_ERRSPACE)
	{
		pList->SetItemData(index, HostUID);

		for(UINT i = 0; i < MAX_GROUP_CHAT_HOST; ++i)
			if(!m_UIDArray[i])
			{
				m_PeerName[i] = csHostName;
				m_UIDArray[i] = HostUID;

				if(!bResume)
				{
					CString cs;
					cs.Format(_T(" %s enters the chat room.\n"), csHostName);
					AddString(RGB(0, 128, 0), cs, cs.GetLength());
				}

				break;
			}
	}
}

void CChatDialog::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	UINT width = pRect->right - pRect->left, nMinWidth;
	UINT height = pRect->bottom - pRect->top, nMinHeight;

	if(m_bGroupChatMode)
	{
		nMinWidth = MIN_GROUP_CHAT_DIALOG_WIDTH;
		nMinHeight = MIN_GROUP_CHAT_DIALOG_HEIGHT;
	}
	else
	{
		nMinWidth = MIN_CHAT_DIALOG_WIDTH;
		nMinHeight = MIN_CHAT_DIALOG_HEIGHT;
	}

	if(width < nMinWidth)
	{
		if(fwSide == WMSZ_LEFT || fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_TOPLEFT)
			pRect->left = pRect->right - nMinWidth;
		else if(fwSide == WMSZ_RIGHT || fwSide == WMSZ_BOTTOMRIGHT || fwSide == WMSZ_TOPRIGHT)
			pRect->right = pRect->left + nMinWidth;
	}
	if(height < nMinHeight)
	{
		if(fwSide == WMSZ_BOTTOM || fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_BOTTOMRIGHT)
			pRect->bottom = pRect->top + nMinHeight;
		else if(fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT)
			pRect->top = pRect->bottom - nMinHeight;
	}
}

void CChatDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CWnd *pEdit = GetDlgItem(IDC_CHAT_EDIT), *pChatView = GetDlgItem(IDC_HISTORY);
	CListCtrl *pUserList = (CListCtrl*)GetDlgItem(IDC_USER_LIST);

	if(!pEdit || !pChatView || !pUserList)
		return;

	if(nType == SIZE_MINIMIZED)
	{
		//printx("SIZE_MINIMIZED\n");
		m_bMinimized = TRUE;
	}
	else if(nType == SIZE_RESTORED || nType == SIZE_MAXIMIZED)
	{
		//printx("SIZE_RESTORED\n");
		if(m_bMinimized)
		{
			if(!m_csTempBuffer.IsEmpty())
				SetTimer(TIMER_UPDATE_TEXT, 50, 0);
			m_bMinimized = FALSE;
		}
	}

	const UINT nMargin = 15, nEditHeight = 100, nUserListWidth = 120, nUserListHeight = 250;

	if(!m_bGroupChatMode)
	{
		pChatView->SetWindowPos(0, nMargin, nMargin, cx - nMargin * 2, cy - nEditHeight - nMargin * 3, SWP_NOZORDER);
		pEdit->SetWindowPos(0, nMargin * 2, cy - nMargin - nEditHeight, cx - nMargin * 4, nEditHeight, SWP_NOZORDER);

	//	UINT nWidth = cx - nMargin * 2;
	//	pChatView->SetWindowPos(0, nMargin, nMargin, nWidth, cy - nEditHeight - nMargin * 3, SWP_NOZORDER);
	//	pEdit->SetWindowPos(0, nMargin, cy - nMargin - nEditHeight, nWidth, nEditHeight, SWP_NOZORDER);
	}
	else
	{
		UINT nWidth = cx - nMargin * 3 - nUserListWidth;

		pChatView->SetWindowPos(0, nMargin, nMargin, nWidth, cy - nEditHeight - nMargin * 3, SWP_NOZORDER);
		pEdit->SetWindowPos(0, nMargin, cy - nMargin - nEditHeight, nWidth, nEditHeight, SWP_NOZORDER);

		pUserList->SetWindowPos(0, nMargin * 2 + nWidth, nMargin, nUserListWidth, nUserListHeight, SWP_NOZORDER);
	}
}

void CChatDialog::OnTimer(UINT_PTR nIDEvent)
{
	CRichEditCtrl *pEdit = (CRichEditCtrl*)GetDlgItem(IDC_HISTORY);
	INT pos = pEdit->GetTextLength();
	pEdit->SetSel(pos - 1, pos - 1);
	pEdit->ReplaceSel(m_csTempBuffer);

	m_csTempBuffer.Empty();
	KillTimer(TIMER_UPDATE_TEXT);
}

void CChatDialog::OnCommandEvict()
{
	INT index = m_UserList.GetCurSel();
	if(index == LB_ERR)
		return;

	DWORD DestUID = m_UserList.GetItemData(index);
	AddJobGroupChatEvict(DestUID, m_SessionIndex);
}

void CChatDialog::OnCommandInvite()
{
	if(m_SessionIndex == MAX_GROUP_CHAT_SESSION)
		return;

	stGUIVLanInfo *pVLanInfo = GNetworkManager.FindVNet(m_VNetID);
	if(!pVLanInfo)
		return;

	CDlgHostPicker dlg;
	dlg.SetID(&GNetworkManager, pVLanInfo);

	if(dlg.DoModal() == IDOK)
	{
		UINT nHostCount = 0;
		DWORD *UIDArray = dlg.GetSelectedHost(nHostCount);
		if(nHostCount)
			AddJobGroupChatInvite(m_VNetID, m_SessionIndex, nHostCount, UIDArray);
	}
}

LRESULT CChatDialog::OnInvalidSession(WPARAM wParam, LPARAM lParam)
{
//	AfxMessageBox(_T("Invalid session!\n"));
	CloseSession();
	return 0;
}


CChatManager::CChatManager()
:m_pParentWnd(0), m_HostUID(0)
{
	GPopupMenu.CreatePopupMenu();
}

CChatManager::~CChatManager()
{
}

CChatDialog* CChatManager::Show(DWORD dwUID, CString HostName)
{
	CChatDialog *pDialog = NULL;
	POSITION pos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos; pDialog = NULL)
	{
		pDialog = m_list.GetNext(pos);
		if(pDialog->m_PeerUID == dwUID)
			break;
	}

	if(pDialog != NULL)
	{
		m_cs.LeaveCriticalSection();
		pDialog->ShowWindow(SW_NORMAL);
		pDialog->SetForegroundWindow();
		return pDialog;
	}

	pDialog = new CChatDialog;
	pDialog->SetMember(m_HostUID, dwUID, HostName);
	m_list.AddTail(pDialog);
	m_cs.LeaveCriticalSection();

	ASSERT(m_pParentWnd != NULL);
	pDialog->SetManager(this);
	pDialog->Create(CChatDialog::IDD, m_pParentWnd);
	pDialog->ShowWindow(SW_SHOW);

	return pDialog;
}

void CChatManager::FlashWindow(CChatDialog *pDialog)
{
	FLASHWINFO finfo = { 0 };

	finfo.cbSize = sizeof(finfo);
	finfo.hwnd = pDialog->GetSafeHwnd();
	finfo.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;

	FlashWindowEx(&finfo);
}

void CChatManager::Close(CChatDialog *pChatDialog)
{
	CChatDialog *pDlg;
	POSITION pos, oldpos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pDlg = m_list.GetNext(pos);

		if(pDlg == pChatDialog)
		{
			m_list.RemoveAt(oldpos);
			pDlg->DestroyWindow();
			break;
		}
	}

	m_cs.LeaveCriticalSection();
}

void CChatManager::Close(DWORD UID)
{
	CChatDialog *pDlg;
	POSITION pos, oldpos;

	m_cs.EnterCriticalSection();

	for(pos = m_list.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pDlg = m_list.GetNext(pos);

		if(pDlg->m_PeerUID == UID)
		{
			m_list.RemoveAt(oldpos);
			pDlg->DestroyWindow();
			break;
		}
	}

	m_cs.LeaveCriticalSection();
}

void CChatManager::OnReceiveText(CString &HostName, DWORD SenderUID, const TCHAR *pText)
{
	CChatDialog *pDlg = NULL;

	m_cs.EnterCriticalSection();
	for(POSITION pos = m_list.GetHeadPosition(); pos; pDlg = NULL)
	{
		pDlg = m_list.GetNext(pos);
		if(pDlg->m_PeerUID == SenderUID)
			break;
	}
	m_cs.LeaveCriticalSection();

	if(pDlg == NULL)
		pDlg = Show(SenderUID, HostName); // Create chat dialog if not found.
	ASSERT(pDlg != NULL);

	pDlg->ReceiveString(pText);
	FlashWindow(pDlg);
	MessageBeep(0xFFFFFFFF);
}

CChatDialog* CChatManager::CreateGroupChatGUI(UINT nSession, DWORD VNetID)
{
	ASSERT(nSession < MAX_GROUP_CHAT_SESSION);

	CChatDialog *pDialog = new CChatDialog;
	if(pDialog == NULL)
		return NULL;

	pDialog->SetManager(this);
	pDialog->m_bGroupChatMode = TRUE;
	pDialog->m_VNetID = VNetID;
	pDialog->m_HostUID = m_HostUID;
	pDialog->m_OldSessionIndex = pDialog->m_SessionIndex = nSession;

	m_cs.EnterCriticalSection();
	m_list.AddTail(pDialog);
	m_cs.LeaveCriticalSection();

	return pDialog;
}

CChatDialog* CChatManager::FindDialogBySessionIndex(UINT index, BOOL bResume)
{
	CChatDialog *pDialog = NULL;

	m_cs.EnterCriticalSection();
	if(bResume)
		for(POSITION pos = m_list.GetHeadPosition(); pos; pDialog = NULL)
		{
			pDialog = m_list.GetNext(pos);
			if(pDialog->m_bGroupChatMode && pDialog->m_OldSessionIndex == index)
				break;
		}
	else
		for(POSITION pos = m_list.GetHeadPosition(); pos; pDialog = NULL)
		{
			pDialog = m_list.GetNext(pos);
			if(pDialog->m_bGroupChatMode && pDialog->m_SessionIndex == index)
				break;
		}
	m_cs.LeaveCriticalSection();

	return pDialog;
}

void CChatManager::OnAppGuiExit()
{
	CChatDialog *pDialog;

	m_cs.EnterCriticalSection();
	for(POSITION pos = m_list.GetHeadPosition(); pos;)
	{
		pDialog = m_list.GetNext(pos);
		if(!pDialog->m_bGroupChatMode)
			continue;
		pDialog->NotifyLeaveSession();
	}
	m_cs.LeaveCriticalSection();
}

void CChatManager::OnAppLogout()
{
	CChatDialog *pDialog;

	m_cs.EnterCriticalSection();
	for(POSITION pos = m_list.GetHeadPosition(); pos;)
	{
		pDialog = m_list.GetNext(pos);
		if(!pDialog->m_bGroupChatMode)
			continue;
		pDialog->CloseSession();
	}
	m_cs.LeaveCriticalSection();
}

void CChatManager::OnHostOffline(DWORD uid)
{
	CChatDialog *pDialog;

	m_cs.EnterCriticalSection();
	for(POSITION pos = m_list.GetHeadPosition(); pos;)
	{
		pDialog = m_list.GetNext(pos);
		if(!pDialog->m_bGroupChatMode || pDialog->m_SessionIndex == MAX_GROUP_CHAT_SESSION)
			continue;
		pDialog->RemoveUser(uid);
	}
	m_cs.LeaveCriticalSection();
}

void CChatManager::SetupDialogMembers(BOOL bAdd, DWORD *UIDArray, UINT MemberCount, DWORD VNetID, CChatDialog *pDialog)
{
	CString cs;
	stGUIVLanMember *pMember;
	stGUIVLanInfo *pVLanInfo = GNetworkManager.FindVNet(VNetID);

	for(UINT i = 0, count = 0, j = 1; i < MemberCount; ++i)
	{
		if(UIDArray[i] == m_HostUID)
			continue;
		pMember = pVLanInfo ? pVLanInfo->FindMember(UIDArray[i]) : 0;

		if(!pMember)
			cs.Format(_T("UnknowHost%03d"), j++);
		else
			cs = pMember->HostName;

		if(bAdd)
		{
			pDialog->AddNewMember(cs, UIDArray[i], (bAdd == 2) ? TRUE : FALSE);
		}
		else
		{
			pDialog->m_PeerName[count] = cs;
			pDialog->m_UIDArray[count++] = UIDArray[i];
		}
	}
}

void CChatManager::OnGroupChatEvent(stGUIEventMsg *pMsg)
{
	UINT nLength;
	DWORD type, ResultCode, SessionIndex, VNetID, ClientRequestID, MemberCount, PeerUID;
	DWORD UIDArray[MAX_GROUP_CHAT_HOST];
	TCHAR Buffer[MAX_GROUP_CHAT_LENGTH + 1];
	CChatDialog *pDialog;

	CStreamBuffer sb;
	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	sb >> type;

	switch(type)
	{
		case GCRT_REQUEST:
			sb >> ResultCode >> SessionIndex >> VNetID >> ClientRequestID >> MemberCount;
			sb.Read(UIDArray, sizeof(DWORD) * MemberCount);

		//	printx("GCRT_REQUEST %d %d %d %d %d\n", ResultCode, SessionIndex, VNetID, ClientRequestID, MemberCount);

			pDialog = CreateGroupChatGUI(SessionIndex, VNetID);
			if(!pDialog)
				break;
			pDialog->SetSessionOwner(UIDArray[0]);
			SetupDialogMembers(FALSE, UIDArray, MemberCount, VNetID, pDialog);

			pDialog->Create(CChatDialog::IDD, m_pParentWnd);
			pDialog->ShowWindow(SW_NORMAL);
			pDialog->UpdateGUIList();

			break;

		case GCRT_INVITE:
			sb >> ResultCode >> SessionIndex >> VNetID >> PeerUID >> MemberCount;
			sb.Read(UIDArray, sizeof(DWORD) * MemberCount);
			ASSERT(SessionIndex <= MAX_GROUP_CHAT_SESSION);

			if(ResultCode == GCCSC_OK)
			{
				pDialog = CreateGroupChatGUI(SessionIndex, VNetID);
				if(!pDialog)
					break;
				pDialog->SetSessionOwner(PeerUID);
				SetupDialogMembers(FALSE, UIDArray, MemberCount, VNetID, pDialog);

				pDialog->Create(CChatDialog::IDD, m_pParentWnd);
				pDialog->ShowWindow(SW_NORMAL);
				pDialog->UpdateGUIList();
			}
			else if(ResultCode == GCCSC_NOTIFY)
			{
				pDialog = FindDialogBySessionIndex(SessionIndex);
				if(!pDialog)
					break;
				SetupDialogMembers(1, UIDArray, MemberCount, VNetID, pDialog);
			}
			else if(ResultCode == GCCSC_RESUME)
			{
				printx("GCCSC_RESUME\n");
				BOOL bResetDialog = FALSE;
				pDialog = FindDialogBySessionIndex(SessionIndex, TRUE);
				if(!pDialog)
				{
					bResetDialog = TRUE;
					pDialog = CreateGroupChatGUI(SessionIndex, VNetID);
					if(!pDialog)
						break;
				}

				SetupDialogMembers(2, UIDArray, MemberCount, VNetID, pDialog);

				if(!bResetDialog)
					pDialog->ResumeSession();
				else
				{
					pDialog->Create(CChatDialog::IDD, m_pParentWnd);
					pDialog->ShowWindow(SW_NORMAL);
					pDialog->UpdateGUIList();
				}
			}

			break;

		case GCRT_LEAVE:
			sb >> ResultCode >> SessionIndex >> PeerUID;
			ASSERT(ResultCode == GCCSC_NOTIFY);

			pDialog = FindDialogBySessionIndex(SessionIndex);
			if(!pDialog)
				break;

			pDialog->RemoveUser(PeerUID);
			break;

		case GCRT_CLOSE:
			sb >> ResultCode >> SessionIndex >> PeerUID;

			pDialog = FindDialogBySessionIndex(SessionIndex);
			if(!pDialog)
				break;
			pDialog->CloseSession();
			break;

		case GCRT_EVICT:
			sb >> ResultCode >> SessionIndex >> PeerUID;

			pDialog = FindDialogBySessionIndex(SessionIndex);
			if(!pDialog)
				break;

			if(ResultCode == GCCSC_NOTIFY)
				pDialog->CloseSession();
			else if(ResultCode == GCCSC_OK)
				pDialog->RemoveUser(PeerUID);
			break;

		case GCRT_CHAT:
			sb >> ResultCode >> SessionIndex >> PeerUID >> nLength;

			pDialog = FindDialogBySessionIndex(SessionIndex);
			if(!pDialog) // Must do check.
				break;

			if(ResultCode == GCCSC_INVALID_SESSION)
			{
				pDialog->PostMessage(WM_INVALID_SESSION);
				break;
			}

			sb.Read(Buffer, sizeof(TCHAR) * nLength);
			Buffer[nLength] = 0;
			pDialog->ReceiveString(PeerUID, Buffer);

			ASSERT(nLength <= MAX_GROUP_CHAT_LENGTH);
	//		printx("chat msg: %d %d %d %d\n", ResultCode, SessionIndex, PeerUID, nLength);
			break;
	}

	sb.DetachBuffer();
	ReleaseGUIEventMsg(pMsg);
}


