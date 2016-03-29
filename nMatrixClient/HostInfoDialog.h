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


class CHostInfoManager;


class CHostInfoDialog : public CDialog
{
public:

	enum{ IDD = IDD_HOSTINFODIALOG, TIMER_ID = 1, UPDATE_TIME_INTERVAL = 1000, };


	CHostInfoDialog(CWnd* pParent = NULL);
	virtual ~CHostInfoDialog();

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);

	void SetVLanMember(stGUIVLanInfo *pVLanInfo, stGUIVLanMember *pMember) { m_pVLanInfo = pVLanInfo; m_pMember = pMember; }
	void SetManager(CHostInfoManager *pManager) { m_pManager = pManager; }

	void SafeClose();
	void UpdataData();


protected:

	friend class CHostInfoManager;

	CHostInfoManager *m_pManager;
	stGUIVLanInfo *m_pVLanInfo;
	stGUIVLanMember *m_pMember;
	CString m_csHostName;

	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void PostNcDestroy();

	DECLARE_DYNAMIC(CHostInfoDialog)
	DECLARE_MESSAGE_MAP()


};


class CHostInfoManager
{
public:

	CHostInfoManager()
	:m_pParentWnd(NULL)
	{
	}

	void ShowInfo(stGUIVLanInfo *pVLanInfo, stGUIVLanMember *pMember);
	void Close(DWORD NetID, DWORD UID);
	void Close(DWORD UID);
	void Close(CHostInfoDialog *pDlg); // For CHostInfoDialog only.
	void CloseAll();
	stTrafficTable* GetTrafficTable() { return ((CVPNClientDlg*)m_pParentWnd)->GetTrafficTable(); }
	void SetParentWnd(CWnd *pWnd) { m_pParentWnd = pWnd; }


protected:

	CWnd *m_pParentWnd;
	CList<CHostInfoDialog*> m_list;
	CCriticalSectionUTX m_cs;


};


