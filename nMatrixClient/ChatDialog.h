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


#define WM_INVALID_SESSION (WM_USER + 1)


class CChatManager;


class CUserListBox : public CListBox
{
public:

	CUserListBox();
	virtual ~CUserListBox();


	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);


protected:

	DECLARE_MESSAGE_MAP()


};


class CChatDialog : public CDialog
{
public:

	enum
	{
		IDD = IDD_CHAT,
		TIMER_UPDATE_TEXT,
		MIN_CHAT_DIALOG_WIDTH  = 400,
		MIN_CHAT_DIALOG_HEIGHT = 400,
		MIN_GROUP_CHAT_DIALOG_WIDTH  = 500,
		MIN_GROUP_CHAT_DIALOG_HEIGHT = 400,

		// Pop-up menu commands.
		PMC_EVICT = 100,
		PMC_INVITE,

	};


	CChatDialog(CWnd* pParent = NULL);
	virtual ~CChatDialog();

	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnCommandInvite();
	afx_msg void OnCommandEvict();
	afx_msg LRESULT OnInvalidSession(WPARAM wParam, LPARAM lParam);

	void NotifyLeaveSession();
	void SafeClose();
	void OnKeyEnterDown();
	void AppendChatString(const TCHAR *pString);
	void ReceiveString(const TCHAR *pString);
	void ReceiveString(DWORD uid, const TCHAR *pString);
	void AddString(DWORD dwColor, const TCHAR *pString, UINT nLength = 0);
	void DeleteLine();
	void UpdateGUIList();
	void CloseSession();
	void ResumeSession();
	void RemoveUser(DWORD dwUID);
	void AddNewMember(CString csHostName, DWORD HostUID, BOOL bResume = FALSE);
	void SetMember(DWORD HostUID, DWORD dwUID, CString &HostName) { m_HostUID = HostUID; m_PeerUID = dwUID; m_HostName = HostName; }
	void SetManager(CChatManager *pManager) { m_pChatManager = pManager; }
	void SetSessionOwner(DWORD OwnerUID) { m_OwnerUID = OwnerUID; }


protected:

	friend class CChatManager;

	CUserListBox m_UserList;
	CChatManager *m_pChatManager;

	BOOL    m_bGroupChatMode, m_bMinimized;
	UINT    m_SessionIndex, m_OldSessionIndex;

	DWORD   m_HostUID, m_OwnerUID, m_PeerUID, m_VNetID;
	CString m_HostName, m_csTempBuffer;

	DWORD   m_UIDArray[MAX_GROUP_CHAT_HOST];
	CString m_PeerName[MAX_GROUP_CHAT_HOST];


	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void PostNcDestroy();
	virtual void OnCancel();

	//DECLARE_DYNAMIC(CChatDialog)
	DECLARE_MESSAGE_MAP()


};


class CChatManager
{
public:

	CChatManager();
	~CChatManager();


	CChatDialog* Show(DWORD dwUID, CString HostName);
	void FlashWindow(CChatDialog *pDialog);
	void Close(CChatDialog *pChatDialog);
	void Close(DWORD UID);
	void SetHostUID(DWORD uid) { m_HostUID = uid; }
	void OnReceiveText(CString &HostName, DWORD SenderUID, const TCHAR *pText);
	void SetParentWnd(CWnd *pParent) { m_pParentWnd = pParent; }


	CChatDialog* CreateGroupChatGUI(UINT nSession, DWORD VNetID);
	CChatDialog* FindDialogBySessionIndex(UINT index, BOOL bResume = FALSE);
	void OnAppGuiExit();
	void OnAppLogout();
	void OnHostOffline(DWORD uid);
	void SetupDialogMembers(BOOL bAdd, DWORD *UIDArray, UINT MemberCount, DWORD VNetID, CChatDialog *pDialog);
	void OnGroupChatEvent(stGUIEventMsg *pMsg);


protected:

	CWnd *m_pParentWnd;
	DWORD m_HostUID;

	CCriticalSectionUTX m_cs;
	CList<CChatDialog*> m_list;


};


