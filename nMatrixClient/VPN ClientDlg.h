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


#include "CnMatrixCore.h"
#include "VividTreeEx.h"
#include "Profiler.h"
#include "CommonDlg.h"
#include "DebugToolDlg.h"


#define WM_GUI_EVENT  (WM_USER + 1)
#define WM_SHELL_ICON (WM_USER + 2)


#define MIN_DIALOG_SIZE 1
#define WINDOW_TITLE _T("nMatrix")


enum POPUP_MENU_COMMAND // GUI command id for pop-up menu.
{
	COMMAND_SET_HOST = 200, // Hub & Spoke.
	COMMAND_SET_RELAY,
	COMMAND_EXIT_NET,
	COMMAND_DELETE_NET,
	COMMAND_OFFLINE_NET,
	COMMAND_REMOVE_USER,
	COMMAND_NET_MANAGE,
	COMMAND_HOST_INFO,
	COMMAND_TEXT_CHAT,
	COMMAND_GROUP_CHAT,
	COMMAND_COPY_IP,
	COMMAND_MSTSC,
	COMMAND_EXPLORE,
	COMMAND_TUNNEL_PATH,
	COMMAND_PING_HOST,

	COMMAND_SUBGROUP_OFFLINE,
	COMMAND_CREATE_GROUP,
	COMMAND_DELETE_GROUP,
	COMMAND_SET_DEFAULT,
	COMMAND_MANAGE_GROUP,

	COMMAND_MOVE_MEMBER_BASE,
	COMMAND_MOVE_MEMBER_END = COMMAND_MOVE_MEMBER_BASE + MAX_VNET_GROUP_COUNT,

};


class CLimitSingleInstance
{
public:

	CLimitSingleInstance(const TCHAR *strMutexName)
	{
		// Make sure that you use a name that is unique for this application otherwise
		// two apps may think they are the same if they are using same name for 3rd param to CreateMutex.
		m_hMutex = CreateMutex(NULL, FALSE, strMutexName);
		m_dwLastError = GetLastError();
	}
	~CLimitSingleInstance() { Release(); }

	inline BOOL IsAnotherInstanceRunning()
	{
		return (ERROR_ALREADY_EXISTS == m_dwLastError);
	}
	inline void Release()
	{
		if(m_hMutex != NULL)
		{
			CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}
	}


protected:

	DWORD  m_dwLastError;
	HANDLE m_hMutex;


};


class CVPNClientDlg : public CDialog
{
public:

	enum DLG_CONSTANT
	{
		IDD = IDD_VPNCLIENT_DIALOG,
		READ_TRAFFIC_TIMER_ID = 2,
		READ_TRAFFIC_PERIOD = 1000, // In ms.
		MIN_WIDTH = 220,
		MIN_HEIGHT = 450,
		WND_ANI_TIME = 300,
	};

	enum SHELL_NOTIFY_TOOLTIP_STRING_ID
	{
		TIP_OFFLINE,
		TIP_ONLINE,
		TIP_CONNECTING,
		TIP_DEFAULT
	};

	enum POPUP_MENU_MODE
	{
		PMM_NULL,
		PMM_COPY_ADDRESS,
		PMM_STATIC_MENU, // Content of the menu won't change before this vaule.

		PMM_VIRTUAL_LAN,
		PMM_VLAN_GROUP,
		PMM_VLAN_MEMBER,
	};

	enum DLG_FLAG
	{
		DF_TRY_LOGIN = 0x01,
		DF_LOGIN     = 0x01 << 1,
		DF_VISIBLE   = 0x01 << 2,
		DF_INIT_UI   = 0x01 << 3,
		DF_MSTSC     = 0x01 << 4
	};


	CVPNClientDlg(CWnd* pParent = NULL);

	// Tree control method.
	void AddUser(stGUIEventMsg *pMsg);
	void HostOnline(stGUIEventMsg *pData);
	void RemoveUser(stGUIEventMsg *pData);
	void ExitNetResult(stGUIEventMsg *pMsg);
	void OnOfflineNetResult(stGUIEventMsg *pMsg);
	void OnOfflineNetPeer(stGUIEventMsg *pData);
	void UpdateNetList(stGUIEventMsg *pData);
	void UpdateConnectState(stGUIEventMsg *pData);

	void OnLoginResult(DWORD dwResult);
	void OnRegisterEvent(stGUIEventMsg *pMsg);
	void OnUpdateMemberState(stGUIEventMsg *pMsg);
	void ReadTrafficInfo(stGUIEventMsg *pMsg);
	void OnRelayEvent(stGUIEventMsg *pMsg);
	void OnDataExchange(stGUIEventMsg *pMsg);
	void OnSettingResponse(stGUIEventMsg *pMsg);
	void OnClientQuery(stGUIEventMsg *pMsg);
	void OnUpdateAdapterConfiguration(DWORD dw);
	void OnCoreServiceError(DWORD dwErrorCode);
	void OnRelayInfo(stGUIEventMsg *pMsg);
	void OnServiceVersion(DWORD dwServiceVersion);
	void OnSubnetSubgroup(stGUIEventMsg *pMsg);
	void OnClientProfile(stGUIEventMsg *pMsg);

	void GUISaveNetInfo();
	void GUILoadNetInfo();
	void SetLoginButtonState();
	void SetShellNotifyData();
	void InitSubmenuForGroup(stGUIVLanInfo *pVLanInfo, stGUIVLanMember *pMember, BOOL bAppend);
	void SetPopupMenuMode(DWORD mode, stGUIVLanInfo *pVLanInfo = 0, stGUIObjectHeader *pObject = 0);
	void GUISetHostName();
	void GUIUpdateVirtualIP(IPV4 vip);
	void UpdateDlgPosition(); // Make sure dialog is visible to user.

	afx_msg void OnBnClickedLogin();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, INT cx, INT cy);
	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg UINT OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu);

	afx_msg LRESULT OnGUIEvent(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShellNotify(WPARAM wParam, LPARAM lParam);

	// Tree control notify.
	afx_msg void OnNMDblclkNetTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickNetTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);

	// Menu command.
	afx_msg void OnSystemSettings();
	afx_msg void OnSystemSetloginaccount();
	afx_msg void OnSystemExit();
	afx_msg void OnNetworkCreatenetwork();
	afx_msg void OnNetworkJoinnetwork();
	afx_msg void OnSwitchNetworkLocation();
	afx_msg void OnHelpCheckupdate();
	afx_msg void OnHelpServernews();
	afx_msg void OnHelpWebsite();
	afx_msg void OnHelpUninstalladapterdriver();
	afx_msg void OnHelpAbout();
	afx_msg void OnUpdateSystemSetloginaccount(CCmdUI *pCmdUI);
	afx_msg void OnUpdateNetworkCreatenetwork(CCmdUI *pCmdUI);
	afx_msg void OnUpdateNetworkJoinnetwork(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHelpCheckupdate(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHelpServernews(CCmdUI *pCmdUI);
	afx_msg void OnUpdateHelpUninstalladapterdriver(CCmdUI *pCmdUI);

	// Pop-up menu handler.
	afx_msg void OnSetHost();
	afx_msg void OnSetRelay();
	afx_msg void OnNetworkManage();
	afx_msg void OnExitVirtualNetwork();
	afx_msg void OnDeleteVirtualNetwork();
	afx_msg void OnHostInformation();
	afx_msg void OnTextChat();
	afx_msg void OnGroupChat();
	afx_msg void OnRemoveUser();
	afx_msg void OnCopyIPAddress();
	afx_msg void OnMstsc();
	afx_msg void OnOfflineSubnet();
	afx_msg void OnExploreHost();
	afx_msg void OnTunnelPath();
	afx_msg void OnPingHost();
	afx_msg void OnSubgroupOffline();
	afx_msg void OnSubgroupCreate();
	afx_msg void OnSubgroupDelete();
	afx_msg void OnSubgroupSetDefault();
	afx_msg void OnSubgroupManage();
	afx_msg void OnSubroupMoveMember(UINT nID);

	void FlagAdd(DWORD f) { m_flag |= f; }
	void FlagClean(DWORD f) { m_flag &= ~f; }
	BOOL IsLogin() { return m_flag & DF_LOGIN; }
	BOOL IsVisible() { return m_flag & DF_VISIBLE; }
	BOOL IsFirstTimeUpdate() { return m_flag & DF_INIT_UI; }
	stTrafficTable* GetTrafficTable() { return &m_TrafficTable; }
	CVividTreeEx* GetTreeObject() { return &m_vividTree; }
	UINT GetClosedMsgID() { return m_ClosedMsgID; }
	void EnableInitWindowHook() { m_bHookInitState = 2; }

	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);


protected:

	HICON   m_hIcon, m_hIconOnline, m_hIconOffline;
	HWND    m_NotifyWnd;
	CPoint  m_LBClickDown, m_offset;
	BOOL    m_bHookInitState;

	DWORD   m_flag, m_ServerCtrlFlag, m_PopupMenuMode, m_HostUID;
	CMenu   m_PopupMenu, m_PopupShellIconMenu;
	CString m_HostName, m_ip, m_LoginName, m_LoginPassword; // m_ip is for clipboard operation.
	IPV4    m_Vip;

	DWORD m_ServerMsgID, m_ClosedMsgID;
	CString m_ServerNews;

	CnMatrixCore   m_nMatrixCore;
	stConfigData   m_ConfigData;
	stRegisterID   m_RegID;
	stTrafficTable m_TrafficTable;
	NOTIFYICONDATA m_tnd;
	CVividTreeEx   m_vividTree;
	CDlgServerMsg  m_ServerMsgDlg;
//	CDlgHtmlNews   m_HtmlNewsDlg;
	CDlgPingHost   m_DlgPingHost;
	CDebugToolDlg  m_DebugToolDlg;
	CToolTipCtrl   m_ToolTipCtrl, *m_pTreeToolTip;


	friend void MainDlgLanEventCallback(void *pData, DWORD LanID);

	void InitLanguageData(DWORD LanID);
	void ReadLoginData(stGUIEventMsg *pMsg); // Data exchange method.

	virtual BOOL OnInitDialog();
	//virtual void DoDataExchange(CDataExchange *pDX);

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()


};


