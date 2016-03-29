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
#include "HostInfoDialog.h"
#include "ChatDialog.h"
#include "SetupDialog.h"
#include "DlgSetting.h"
#include "DriverAPI.h"


CHostInfoManager   GHostInfoManager;
CChatManager       GChatManager;
CGUINetworkManager GNetworkManager;


void MainDlgLanEventCallback(void *pData, DWORD LanID)
{
	((CVPNClientDlg*)pData)->InitLanguageData(LanID);
}


CVPNClientDlg::CVPNClientDlg(CWnd* pParent /*=NULL*/)
:CDialog(CVPNClientDlg::IDD, pParent), m_PopupMenuMode(PMM_NULL), m_pTreeToolTip(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconOnline = AfxGetApp()->LoadIcon(IDI_ICON4L);
	m_hIconOffline = AfxGetApp()->LoadIcon(IDI_ICON1L);
	m_bHookInitState = 0;
	m_flag = 0;
	m_ServerCtrlFlag = SCF_ALL_FLAG;
	ZeroMemory(&m_tnd, sizeof m_tnd);

	m_ServerMsgID = m_ClosedMsgID = 0;
	m_NotifyWnd = 0;
}

//void CVPNClientDlg::DoDataExchange(CDataExchange* pDX)
//{
//	CDialog::DoDataExchange(pDX);
//}


BEGIN_MESSAGE_MAP(CVPNClientDlg, CDialog)
	ON_BN_CLICKED(IDC_LOGIN, &CVPNClientDlg::OnBnClickedLogin)

	ON_MESSAGE(WM_GUI_EVENT, &CVPNClientDlg::OnGUIEvent)
	ON_MESSAGE(WM_SHELL_ICON, &CVPNClientDlg::OnShellNotify)

	ON_NOTIFY(NM_RCLICK, IDC_NET_TREE, &CVPNClientDlg::OnNMRClickNetTree)
	ON_NOTIFY(NM_DBLCLK, IDC_NET_TREE, &CVPNClientDlg::OnNMDblclkNetTree)
	ON_NOTIFY(TVN_GETINFOTIP, IDC_NET_TREE, &CVPNClientDlg::OnGetInfoTip)

	ON_WM_INITMENUPOPUP()
	ON_WM_MOVING()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_SYSCOMMAND()
	ON_WM_TIMER()
	ON_WM_SIZING()

	// Menu command.
	ON_COMMAND(COMMAND_SET_HOST, &CVPNClientDlg::OnSetHost)
	ON_COMMAND(COMMAND_SET_RELAY, &CVPNClientDlg::OnSetRelay)
	ON_COMMAND(COMMAND_NET_MANAGE, &CVPNClientDlg::OnNetworkManage)
	ON_COMMAND(COMMAND_EXIT_NET, &CVPNClientDlg::OnExitVirtualNetwork)
	ON_COMMAND(COMMAND_DELETE_NET, &CVPNClientDlg::OnDeleteVirtualNetwork)
	ON_COMMAND(COMMAND_REMOVE_USER, &CVPNClientDlg::OnRemoveUser)
	ON_COMMAND(COMMAND_COPY_IP, &CVPNClientDlg::OnCopyIPAddress)
	ON_COMMAND(COMMAND_MSTSC, &CVPNClientDlg::OnMstsc)
	ON_COMMAND(COMMAND_OFFLINE_NET, &CVPNClientDlg::OnOfflineSubnet)
	ON_COMMAND(COMMAND_EXPLORE, &CVPNClientDlg::OnExploreHost)
	ON_COMMAND(COMMAND_TUNNEL_PATH, &CVPNClientDlg::OnTunnelPath)
	ON_COMMAND(COMMAND_PING_HOST, &CVPNClientDlg::OnPingHost)
	ON_COMMAND(COMMAND_SUBGROUP_OFFLINE, &CVPNClientDlg::OnSubgroupOffline)
	ON_COMMAND(COMMAND_CREATE_GROUP, &CVPNClientDlg::OnSubgroupCreate)
	ON_COMMAND(COMMAND_DELETE_GROUP, &CVPNClientDlg::OnSubgroupDelete)
	ON_COMMAND(COMMAND_SET_DEFAULT, &CVPNClientDlg::OnSubgroupSetDefault)
	ON_COMMAND(COMMAND_MANAGE_GROUP, &CVPNClientDlg::OnSubgroupManage)
	ON_COMMAND(COMMAND_HOST_INFO, &CVPNClientDlg::OnHostInformation)
	ON_COMMAND(COMMAND_TEXT_CHAT, &CVPNClientDlg::OnTextChat)
	ON_COMMAND(COMMAND_GROUP_CHAT, &CVPNClientDlg::OnGroupChat)
	ON_COMMAND_RANGE(COMMAND_MOVE_MEMBER_BASE, COMMAND_MOVE_MEMBER_END, &CVPNClientDlg::OnSubroupMoveMember)

	ON_COMMAND(ID_SYSTEM_SETLOGINACCOUNT, &CVPNClientDlg::OnSystemSetloginaccount)
	ON_UPDATE_COMMAND_UI(ID_SYSTEM_SETLOGINACCOUNT, &CVPNClientDlg::OnUpdateSystemSetloginaccount)
	ON_COMMAND(ID_SYSTEM_EXIT, &CVPNClientDlg::OnSystemExit)
	ON_COMMAND(ID_NETWORK_CREATENETWORK, &CVPNClientDlg::OnNetworkCreatenetwork)
	ON_COMMAND(ID_NETWORK_JOINNETWORK, &CVPNClientDlg::OnNetworkJoinnetwork)
	ON_COMMAND(ID_SWITCH_NETWORK_LOCATION, &CVPNClientDlg::OnSwitchNetworkLocation)
	ON_COMMAND(ID_HELP_ABOUT, &CVPNClientDlg::OnHelpAbout)
	ON_COMMAND(ID_HELP_CHECKUPDATE, &CVPNClientDlg::OnHelpCheckupdate)
	ON_UPDATE_COMMAND_UI(ID_NETWORK_CREATENETWORK, &CVPNClientDlg::OnUpdateNetworkCreatenetwork)
	ON_UPDATE_COMMAND_UI(ID_NETWORK_JOINNETWORK, &CVPNClientDlg::OnUpdateNetworkJoinnetwork)
	ON_COMMAND(ID_SYSTEM_SETTINGS, &CVPNClientDlg::OnSystemSettings)
	ON_COMMAND(ID_HELP_WEBSITE, &CVPNClientDlg::OnHelpWebsite)
	ON_COMMAND(ID_HELP_UNINSTALLADAPTERDRIVER, &CVPNClientDlg::OnHelpUninstalladapterdriver)
	ON_UPDATE_COMMAND_UI(ID_HELP_UNINSTALLADAPTERDRIVER, &CVPNClientDlg::OnUpdateHelpUninstalladapterdriver)
	ON_WM_WINDOWPOSCHANGED()
	ON_UPDATE_COMMAND_UI(ID_HELP_CHECKUPDATE, &CVPNClientDlg::OnUpdateHelpCheckupdate)
	ON_COMMAND(ID_HELP_SERVERNEWS, &CVPNClientDlg::OnHelpServernews)
	ON_UPDATE_COMMAND_UI(ID_HELP_SERVERNEWS, &CVPNClientDlg::OnUpdateHelpServernews)
	ON_WM_POWERBROADCAST()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_DROPFILES()

END_MESSAGE_MAP()


void CVPNClientDlg::InitSubmenuForGroup(stGUIVLanInfo *pVLanInfo, stGUIVLanMember *pMember, BOOL bAppend)
{
	CMenu MyMenu;
	CString cs;
	MyMenu.CreatePopupMenu();
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof( MENUITEMINFO );
	mii.fMask = MIIM_ID | MIIM_STRING;

	BYTE GroupIndex = pMember ? pMember->GroupIndex : pVLanInfo->GroupIndex;

	if(GroupIndex && !pVLanInfo->CanHideDefaultGroup())
	{
		mii.wID = COMMAND_MOVE_MEMBER_BASE;
		uCharToCString(pVLanInfo->m_GroupArray[0].VNetGroup.GroupName, -1, cs);
		mii.dwTypeData = cs.GetBuffer();
		MyMenu.InsertMenuItem( -1, &mii, TRUE );
	}

	for(UINT i = 1; i < pVLanInfo->m_nTotalGroup; ++i)
	{
		if(i == GroupIndex)
			continue;
		uCharToCString(pVLanInfo->m_GroupArray[i].VNetGroup.GroupName, -1, cs);
		mii.wID = COMMAND_MOVE_MEMBER_BASE + i;
		mii.dwTypeData = cs.GetBuffer();
		MyMenu.InsertMenuItem(-1, &mii, TRUE);
	}

	cs = GUILoadString(IDS_MOVE_TO);
	MENUITEMINFO miiNew;
	miiNew.cbSize = sizeof( MENUITEMINFO );
	miiNew.fMask = MIIM_SUBMENU | MIIM_STRING;
	miiNew.hSubMenu = MyMenu.Detach();   // Detach() to keep the pop-up menu alive when MyMenu goes out of scope.
	miiNew.dwTypeData = cs.GetBuffer();

	m_PopupMenu.InsertMenuItem( bAppend ? m_PopupMenu.GetMenuItemCount() : 0, &miiNew, TRUE );
}

void CVPNClientDlg::SetPopupMenuMode(DWORD mode, stGUIVLanInfo *pVLanInfo, stGUIObjectHeader *pObject)
{
	if(m_PopupMenuMode == mode && mode < PMM_STATIC_MENU)
		return;

	stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
	stGUIVLanMember *pMember = (stGUIVLanMember*)pObject;

	CString string1, string2 = GUILoadString(IDS_SET_SPOKE), string3 = GUILoadString(IDS_SET_HUB);
	CString string4 = GUILoadString(IDS_SET_RELAY), string5 = GUILoadString(IDS_CANCEL_RELAY);
	CString string6 = GUILoadString(IDS_PING_HOST);

	UINT i, MenuItemCount = m_PopupMenu.GetMenuItemCount();
	for(i = 0; i < MenuItemCount; ++i)
		m_PopupMenu.DeleteMenu(0, MF_BYPOSITION); // Clean content of the menu.

	switch(mode)
	{
		case PMM_VIRTUAL_LAN:
			if(pVLanInfo->IsNetOnline())
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_OFFLINE_NET, GUILoadString(IDS_GO_OFFLINE));
			else
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_OFFLINE_NET, GUILoadString(IDS_GO_ONLINE));
			m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_EXIT_NET, GUILoadString(IDS_LEAVE_NETWORK));

			if(pVLanInfo->IsOwner())
			{
				m_PopupMenu.AppendMenu(MF_SEPARATOR);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_CREATE_GROUP, GUILoadString(IDS_CREATE_SUBGROUP));

				if(pVLanInfo->CanShowGroupMoveMenu())
					InitSubmenuForGroup(pVLanInfo, 0, TRUE);

				m_PopupMenu.AppendMenu(MF_SEPARATOR);

				if(pVLanInfo->NetworkType == VNT_HUB_AND_SPOKE)
				{
					if(pVLanInfo->Flag & VF_HUB)
						m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string2);
					else
						m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string3);
				}

				if(pVLanInfo->Flag & VF_RELAY_HOST)
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string5);
				else
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string4);

				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_DELETE_NET, GUILoadString(IDS_DELETE_NETWORK));
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_NET_MANAGE, GUILoadString(IDS_MANAGE));

			}
			else
			{
				m_PopupMenu.AppendMenu(MF_SEPARATOR);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_NET_MANAGE, GUILoadString(IDS_MANAGE));
			}
			break;

		case PMM_VLAN_GROUP:

			if(1)
			{
				UINT nIndex = pVLanInfo->GetGroupIndex((stGUISubgroup*)pObject);
				if(pVLanInfo->IsSubgroupOnline(nIndex))
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SUBGROUP_OFFLINE, GUILoadString(IDS_GO_OFFLINE));
				else
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SUBGROUP_OFFLINE, GUILoadString(IDS_GO_ONLINE));
			}
			if(pVLanInfo->IsOwner())
			{
				if(m_PopupMenu.GetMenuItemCount())
					m_PopupMenu.AppendMenu(MF_SEPARATOR);

				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_DELETE_GROUP, GUILoadString(IDS_DELETE_SUBGROUP));

				if(pVLanInfo->FindDefaultGroupIndex() != pVLanInfo->GetGroupIndex((stGUISubgroup*)pObject))
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_DEFAULT, GUILoadString(IDS_SET_DEFAULT_SUBGROUP));

				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_MANAGE_GROUP, GUILoadString(IDS_MANAGE));
			}
			break;

		case PMM_VLAN_MEMBER:
		{
			if(pMember->eip.IsZeroAddress())
			{
				if(pVLanInfo->IsOwner())
				{
					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_REMOVE_USER, GUILoadString(IDS_EVICT));
					if(pVLanInfo->NetworkType == VNT_HUB_AND_SPOKE)
						if(pMember->Flag & VF_HUB)
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string2);
						else
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string3);

					if(pMember->Flag & VF_RELAY_HOST)
						m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string5);
					else
					{
						if(pMember->Flag & VF_ALLOW_RELAY)
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string4);
					}

					if(pVLanInfo->CanShowGroupMoveMenu())
						InitSubmenuForGroup(pVLanInfo, pMember, FALSE);
				}
			}
			else
			{
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_TEXT_CHAT, GUILoadString(IDS_SEND_MESSAGE));
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_GROUP_CHAT, GUILoadString(IDS_INVITE_GROUPCHAT));
				m_PopupMenu.AppendMenu(MF_SEPARATOR);

				if(pVLanInfo->IsOwner())
				{
					if(pVLanInfo->CanShowGroupMoveMenu())
						InitSubmenuForGroup(pVLanInfo, pMember, TRUE);

					m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_REMOVE_USER, GUILoadString(IDS_EVICT));

					if(pVLanInfo->NetworkType == VNT_HUB_AND_SPOKE)
						if(pMember->Flag & VF_HUB)
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string2);
						else
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_HOST, string3);

					if(pMember->Flag & VF_RELAY_HOST)
						m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string5);
					else
					{
						if(pMember->Flag & VF_ALLOW_RELAY)
							m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_SET_RELAY, string4);
					}

					m_PopupMenu.AppendMenu(MF_SEPARATOR);
				}

				IPV4 ip = GetKeyState(VK_F2) < 0 ? pMember->eip.v4 : pMember->vip;
				m_ip.Format(_T("%d.%d.%d.%d"), ip.b1, ip.b2, ip.b3, ip.b4);
				string1.Format(_T("%s - %s"), GUILoadString(IDS_COPY_ADDRESS), m_ip);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_COPY_IP, string1);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_MSTSC, GUILoadString(IDS_WIN_REMOTE_DESKTOP));
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_EXPLORE, GUILoadString(IDS_EXPLORE_HOST));
				m_PopupMenu.AppendMenu(MF_SEPARATOR);
				string1 = GUILoadString(IDS_TUNNEL_PATH);
				string2 = GUILoadString(IDS_HOST_INFO);
				if(GetUILanguage() == DEFAULT_LANGUAGE_ID)
				{
					string1.Insert(7, '&');
					string2.Insert(0, '&');
				}
				else
				{
					string1 += _T("(&P)");
					string2 += _T("(&H)");
				}
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_TUNNEL_PATH, string1);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_PING_HOST, string6);
				m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_HOST_INFO, string2);
			}

			break;
		}

		case PMM_COPY_ADDRESS:
			GetDlgItemText(IDC_VIP, m_ip);
			m_PopupMenu.AppendMenu(MF_STRING|MF_ENABLED, COMMAND_COPY_IP, GUILoadString(IDS_COPY_ADDRESS));
			break;
	}

	m_PopupMenuMode = mode;
}

void CVPNClientDlg::GUISetHostName()
{
	const TCHAR *pString;
	CString cs;
	TCHAR HostName[256];

	if(m_flag & DF_LOGIN)
	{
		if(m_ConfigData.LocalName[0])
			pString = m_ConfigData.LocalName;
		else
		{
			DWORD size = sizeof(HostName);
			GetComputerNameEx(ComputerNameDnsHostname, HostName, &size);
			pString = HostName;
		}
	}
	else if(m_flag & DF_TRY_LOGIN)
	{
		cs = GUILoadString(IDS_CONNECTING) + _T("...");
		pString = cs.GetBuffer();
	}
	else
	{
		cs = GUILoadString(IDS_OFFLINE);
		pString = cs.GetBuffer();
	}

	SetDlgItemText(IDC_HOSTNAME, pString);
	m_HostName = pString;
}

void CVPNClientDlg::GUIUpdateVirtualIP(IPV4 vip)
{
	CString info;
	info.Format(_T("%d.%d.%d.%d"), vip.b1, vip.b2, vip.b3, vip.b4);
	SetDlgItemText(IDC_VIP, info);
	m_Vip = vip;
//	printx("Update virtual ip: 0x%08x\n", vip);
}

void CVPNClientDlg::UpdateDlgPosition()
{
	CRect rect, rectDlg, rectDesktop;
	GetWindowRect(&rectDlg);
	SystemParametersInfo(SPI_GETWORKAREA, NULL, &rectDesktop, NULL);
	rect = rectDlg;

	if(rect.Width() > rectDesktop.Width())
		rect.right = rect.left + rectDesktop.Width();
	if(rect.Height() > rectDesktop.Width())
		rect.bottom = rect.top + rectDesktop.Height();

	if(rect.top < rectDesktop.top)
	{
		rect.bottom = rectDesktop.top + rect.Height();
		rect.top = rectDesktop.top;
	}
	if(rect.bottom > rectDesktop.bottom)
	{
		rect.top = rectDesktop.bottom - rect.Height();
		rect.bottom = rectDesktop.bottom;
	}
	if(rect.left < rectDesktop.left)
	{
		rect.right = rectDesktop.left + rect.Width();
		rect.left = rectDesktop.left;
	}
	if(rect.right > rectDesktop.right)
	{
		rect.left = rectDesktop.right - rect.Width();
		rect.right = rectDesktop.right;
	}

	if(rect != rectDlg)
		MoveWindow(rect.left, rect.top, rect.Width(), rect.Height(), FALSE);
}

void CVPNClientDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default
	TCHAR wcStr[MAX_PATH];
	UINT nFileNum = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	DragQueryFile(hDropInfo, nFileNum - 1, wcStr, MAX_PATH);

	AfxMessageBox(wcStr);
	DragFinish(hDropInfo);

	CDlgHostPicker dlg;
	dlg.SetID(&GNetworkManager, 0);

	if(dlg.DoModal() == IDOK)
	{
		AfxMessageBox(_T("Success!"));
	
	}

	CDialog::OnDropFiles(hDropInfo);
}

void CVPNClientDlg::OnInitMenuPopup(CMenu *pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	ASSERT(pPopupMenu != NULL);
	// Check the enabled state of various menu items.
	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);
	// Determine if menu is popup in top-level menu and set m_pOther to it if so (m_pParentMenu == NULL indicates that it is secondary popup).

	HMENU hParentMenu;
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
		state.m_pParentMenu = pPopupMenu; // Parent == child for tracking popup.
	else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
	{
		CWnd* pParent = this;
		// Child windows don't have menus--need to go to the top!
		if (pParent != NULL && (hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
		{
			int nIndexMax = ::GetMenuItemCount(hParentMenu);
			for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
			{
				if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
				{
					// When popup is found, m_pParentMenu is containing menu.
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
					break;
				}
			}
		}
	}
	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax; state.m_nIndex++)
	{
		state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
		if (state.m_nID == 0)
			continue; // Menu separator or invalid cmd - ignore it.
		ASSERT(state.m_pOther == NULL);
		ASSERT(state.m_pMenu != NULL);
		if (state.m_nID == (UINT)-1)
		{
			// Possibly a popup menu, route to first item of that popup.
			state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
			if (state.m_pSubMenu == NULL ||
			(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
			state.m_nID == (UINT)-1)
			{
				continue; // First item of popup can't be routed to.
			}
			state.DoUpdate(this, TRUE); // Popups are never auto disabled.
		}
		else
		{
			// Normal menu item.
			// Auto enable/disable if frame window has m_bAutoMenuEnable
			// set and command is _not_ a system command.
			state.m_pSubMenu = NULL;
			state.DoUpdate(this, FALSE);
		}
		// Adjust for menu deletions and additions.
		UINT nCount = pPopupMenu->GetMenuItemCount();
		if (nCount < state.m_nIndexMax)
		{
			state.m_nIndex -= (state.m_nIndexMax - nCount);
			while (state.m_nIndex < nCount && pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
			{
				state.m_nIndex++;
			}
		}
		state.m_nIndexMax = nCount;
	}
}

typedef BOOL (WINAPI *TChangeWindowMessageFilter)(_In_ UINT message, _In_ DWORD dwFlag);

void ResetWindowsMessageFilter(CWnd *pWnd, DWORD dwOsVersion)
{
	DWORD dwMajorVersion, dwMinorVersion;
	TChangeWindowMessageFilter FnChangeWindowMessageFilter;

	//pWnd->DragAcceptFiles(TRUE);

	dwOsVersion = GetVersion();
	dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwOsVersion)));
	dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwOsVersion)));

	if(dwMajorVersion < 6)
		return;

	HMODULE hLib = LoadLibrary(_T("User32.dll"));
	FnChangeWindowMessageFilter = (TChangeWindowMessageFilter)GetProcAddress(hLib, "ChangeWindowMessageFilter");

	// Reference: How to Enable Drag and Drop for an Elevated MFC Application on Vista/Windows 7.
	if(FnChangeWindowMessageFilter)
	{
		FnChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
		FnChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
		FnChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
	}

	FreeLibrary(hLib);
}

BOOL CVPNClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_PopupMenu.CreatePopupMenu();
	m_PopupShellIconMenu.CreatePopupMenu();
	m_ToolTipCtrl.Create(this, WS_POPUP | WS_EX_TOOLWINDOW); // Create tool tip control.
	m_ToolTipCtrl.AddTool(GetDlgItem(IDC_LOGIN), _T(""));

	if(!m_nMatrixCore.Init(GetSafeHwnd(), 0, m_bHookInitState))
	{
		m_nMatrixCore.Close();
		EndDialog(0);
		return TRUE;
	}
	theApp.ReadConfigData(&m_ConfigData);

	SetGUILanguage(m_ConfigData.LanguageID);
	DWORD LanID = GetUILanguage();
	InitLanguageData(LanID);
	RegLanEventCB(MainDlgLanEventCallback, this);

	// Add "About..." menu item to system menu. IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	//CString strAboutMenu = GUILoadString(IDS_STRING461), strExit = GUILoadString(IDS_STRING452);
	//CMenu* pSysMenu = GetSystemMenu(FALSE);
	//if (pSysMenu != NULL)
	//{
	//	if (!strAboutMenu.IsEmpty())
	//	{
	//		pSysMenu->AppendMenu(MF_SEPARATOR);
	//		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	//	}
	//}

	GBoldFont.CreateFont(
		14,                        // nHeight
		0,                         // nWidth
		0,                         // nEscapement
		0,                         // nOrientation
		FW_BOLD,                   // nWeight
		FALSE,                     // bItalic
		FALSE,                     // bUnderline
		0,                         // cStrikeOut
		ANSI_CHARSET,              // nCharSet
		OUT_DEFAULT_PRECIS,        // nOutPrecision
		CLIP_DEFAULT_PRECIS,       // nClipPrecision
		DEFAULT_QUALITY,    // nQuality DEFAULT_QUALITY NONANTIALIASED_QUALITY PROOF_QUALITY
		DEFAULT_PITCH | FF_SWISS,  // nPitchAndFamily
		0 /*_T("Segoe UI")*/);

	((CEdit*)GetDlgItem(IDC_HOSTNAME))->SetFont(&GBoldFont);
	((CEdit*)GetDlgItem(IDC_VIP))->SetFont(&GBoldFont);

	// Set the icon for this dialog. The framework does this automatically when the application's main window is not a dialog.
	SetIcon(m_hIcon, TRUE);	  // Set big icon
	SetIcon(m_hIcon, FALSE);  // Set small icon

	// TODO: Add extra initialization here.
	SetTimer(READ_TRAFFIC_TIMER_ID, READ_TRAFFIC_PERIOD, 0);

	m_vividTree.Init(&m_TrafficTable);
	m_vividTree.SubclassDlgItem(IDC_NET_TREE, this);
	m_vividTree.SetItemHeight(VT_TREE_ITEMHEIGHT);

	GNetworkManager.SetGUIObject(&m_vividTree);
	GHostInfoManager.SetParentWnd(this);
	GChatManager.SetParentWnd(this);

	SetWindowText(WINDOW_TITLE);
	m_ServerMsgDlg.SetPointer(&m_ClosedMsgID);

	BYTE vmac[6];
	GetAdapterRegInfo(vmac, &m_Vip.ip, 0);
	GUIUpdateVirtualIP(m_Vip);

	if(!m_bHookInitState)
		FlagAdd(DF_VISIBLE);
	if(HasWindowsRemoteDesktop(0))
		FlagAdd(DF_MSTSC);

	// Restore window position.
	CRect rect;
	WINDOWPLACEMENT wp = { 0 };
	wp.length = sizeof(wp);
	wp.rcNormalPosition.top = theApp.GetProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_top), 0);
	wp.rcNormalPosition.bottom = theApp.GetProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_bottom), 0);
	wp.rcNormalPosition.left = theApp.GetProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_left), 0);
	wp.rcNormalPosition.right = theApp.GetProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_right), 0);
	rect = wp.rcNormalPosition;

	if(rect.Width() && rect.Height())
	{
		SetWindowPlacement(&wp);
	//	MoveWindow(rect.left, rect.top, rect.Width(), rect.Height(), FALSE);
	}
	else
	{
		GetWindowRect(&rect);
		MoveWindow(rect.left, rect.top, rect.Width() + 1, rect.Height(), FALSE); // Change size so we can do layout at least once.
	}
	UpdateDlgPosition();

	ResetWindowsMessageFilter(this, 0);

	CToolTipCtrl *pCtrl = m_vividTree.GetToolTips();
	LONG style = GetWindowLong(pCtrl->GetSafeHwnd(), GWL_STYLE);
	style |= TTS_ALWAYSTIP;
	SetWindowLong(pCtrl->GetSafeHwnd(), GWL_STYLE, style);

	//INT time;
	//time = pCtrl->GetDelayTime(TTDT_RESHOW);  // 100ms.
	//time = pCtrl->GetDelayTime(TTDT_AUTOPOP); // 5000ms.
	//time = pCtrl->GetDelayTime(TTDT_INITIAL); // 500ms.
	//pCtrl->SetDelayTime(TTDT_RESHOW, 5000);
	//pCtrl->SetDelayTime(TTDT_AUTOPOP, 5000);
	//pCtrl->SetDelayTime(TTDT_INITIAL, 1000);

	ASSERT(!m_pTreeToolTip);
	m_pTreeToolTip = pCtrl;

	return TRUE;
}

void CVPNClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg DlgAbout;
		DlgAbout.DoModal();
	}

	switch(nID)
	{
		case SC_MINIMIZE:
			FlagClean(DF_VISIBLE);
			m_TrafficTable.ZeroCacheTable();
			break;

		case SC_CLOSE:
	//		FlagClean(DF_VISIBLE); // Let OnWindowPosChanged clean flag.
			GHostInfoManager.CloseAll();
	//		ShowWindow(SW_HIDE);
			AnimateWindow(WND_ANI_TIME, AW_HIDE | AW_BLEND);
			return;

		case SC_RESTORE:
			FlagAdd(DF_VISIBLE);
			break;
	}

	CDialog::OnSysCommand(nID, lParam);
}


// If you add a minimize button to your dialog, you will need the code below to draw the icon.
// For MFC applications using the document/view model, this is automatically done for you by the framework.
void CVPNClientDlg::OnPaint()
{
	CPaintDC dc(this);
//	dc.GradientFill(CRect( m_h_offset, m_v_offset, m_h_size + m_h_offset, m_v_size + m_v_offset ),
//		m_gradient_bkgd_from, m_gradient_bkgd_to, !m_gradient_horz );
	CRect rect;
	GetWindowRect(&rect);
	::FillRect(dc.GetSafeHdc(), &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

	CDialog::OnPaint();
}

HCURSOR CVPNClientDlg::OnQueryDragIcon()
{
	// The system calls this function to obtain the cursor to display while the user drags the minimized window.
	return static_cast<HCURSOR>(m_hIcon);
}

void CVPNClientDlg::OnBnClickedLogin()
{
	if(!IsLogin() && !(m_flag & DF_TRY_LOGIN))
		AddJobLogin(0, 0, 0, FALSE);
	else
		AddJobLogin(LOR_GUI, 0, 0, TRUE);
}

void CVPNClientDlg::GUILoadNetInfo()
{
	CString info, text, state;
	HTREEITEM hItem;
	INT pos, iStart, iTextLen, nBit;
	DWORD dw;

	info = theApp.GetProfileString(_T("ClientInfo"), _T("NetState"), 0);

	for(hItem = m_vividTree.GetRootItem(); hItem; hItem = m_vividTree.GetNextSiblingItem(hItem))
	{
		stGUIVLanInfo *pVLan = m_vividTree.GetVLanInfo(hItem);

		text = pVLan->NetName;
		iTextLen = text.GetLength();
		iStart = 0;

SEARCH_TEXT:

		if((pos = info.Find(text, iStart)) == -1)
			continue;
		iStart += iTextLen;

		if((pos && info[pos - 1] != ';') || (info[pos + iTextLen] != ' ' ) || info[pos + iTextLen + 1 + 8] != ';')
			goto SEARCH_TEXT;

		state = info.Mid(pos + iTextLen + 1, 8);
		dw = _tcstoul(state, 0, 16);

		if(dw & 0x80000000)
			m_vividTree.ToggleButton(hItem);

		nBit = 0;
		if(pVLan->m_nTotalGroup > 1)
			for(HTREEITEM hSub = m_vividTree.GetNextItem(hItem, TVGN_CHILD); hSub; hSub = m_vividTree.GetNextSiblingItem(hSub), ++nBit)
				if(dw & (0x00000001 << nBit))
					m_vividTree.ToggleButton(hSub);
	}
}

void CVPNClientDlg::GUISaveNetInfo()
{
	UINT TreeState, nBit;
	CString info, text;
	HTREEITEM hItem = m_vividTree.GetRootItem(), hSub;

	if(!hItem) // Not login or no item.
		return;

	for(; hItem; hItem = m_vividTree.GetNextSiblingItem(hItem))
	{
		TreeState = 0;

		if(m_vividTree.GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED)
			TreeState |= 0x80000000;

		for(nBit = 0, hSub = m_vividTree.GetNextItem(hItem, TVGN_CHILD); hSub; ++nBit, hSub = m_vividTree.GetNextSiblingItem(hSub))
		{
			stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hSub);
			if(m_vividTree.IsVLanMember(pObject))
				break;

			if(m_vividTree.GetItemState(hSub, TVIS_EXPANDED) & TVIS_EXPANDED)
				TreeState |= (0x00000001 << nBit);
		}

		text.Format(_T("%s %08x;"), m_vividTree.GetVLanName(hItem), TreeState);
		info += text;
	}

	if(!theApp.WriteProfileString(_T("ClientInfo"), _T("NetState"), info))
		printx("GUISaveNetInfo() error!\n");
}

void CVPNClientDlg::SetLoginButtonState()
{
	CString csOnline = GUILoadString(IDS_START), csOffline = GUILoadString(IDS_TURNOFF);
	CWnd *pWnd = GetDlgItem(IDC_LOGIN);

	if((m_flag & DF_LOGIN) || (m_flag & DF_TRY_LOGIN))
	{
		pWnd->SetWindowText(csOffline);
		m_ToolTipCtrl.UpdateTipText(csOffline, pWnd);
	}
	else
	{
		pWnd->SetWindowText(csOnline);
		m_ToolTipCtrl.UpdateTipText(csOnline, pWnd);

		pWnd->EnableWindow(FindAdapter() ? TRUE : FALSE);
	}
}

void CVPNClientDlg::SetShellNotifyData()
{
	DWORD dwMsg = NIM_MODIFY;

	if(!m_tnd.uID)
	{
		m_tnd.cbSize = sizeof(NOTIFYICONDATA);
		m_tnd.hWnd = this->m_hWnd;
		m_tnd.uID = IDR_MAINFRAME;
		m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		m_tnd.uCallbackMessage = WM_SHELL_ICON;
		m_tnd.hIcon = m_hIconOffline;

		dwMsg = NIM_ADD;
	}

	CString csTooltip(_T("nMatrix - "));
	if(m_flag & DF_LOGIN)
	{
		csTooltip += GUILoadString(IDS_ONLINE);
		m_tnd.hIcon = m_hIconOnline;
	}
	else if(m_flag & DF_TRY_LOGIN)
	{
		csTooltip += GUILoadString(IDS_CONNECTING);
		m_tnd.hIcon = m_hIconOffline;
	}
	else
	{
		csTooltip += GUILoadString(IDS_OFFLINE);
		m_tnd.hIcon = m_hIconOffline;
	}

	if(m_nMatrixCore.IsPureGUIMode())
		csTooltip += (_T(" (") + GUILoadString(IDS_SERVICEMODE) + _T(")"));
	else
		csTooltip += (_T(" (") + GUILoadString(IDS_APPMODE) + _T(")"));

	UINT nLen = csTooltip.GetLength();
	if(nLen && nLen < sizeof(m_tnd.szTip) / sizeof(TCHAR))
	{
		memcpy(m_tnd.szTip, csTooltip.GetBuffer(), (nLen + 1) * sizeof(TCHAR));
	}
	else
	{
		ASSERT(0);
	}

	Shell_NotifyIcon(dwMsg, &m_tnd);
}

void CVPNClientDlg::AddUser(stGUIEventMsg *pMsg)
{
	stGUIVLanInfo *pVLanInfo;
	stGUIVLanMember *pVLanMember;

	pVLanInfo = GNetworkManager.FindVNet(pMsg->DWORD_1);
	if(!pVLanInfo)
		return;

	if(pMsg->member.LinkState == LS_TRYING_TPT)
		GNetworkManager.SetMemberLinkState(0, pMsg->member.dwUserID, pMsg->member.LinkState, &pMsg->member.DriverMapIndex, 0, 0);
	if(pMsg->member.LinkState != LS_OFFLINE)
		pVLanInfo->nOnline++;

	pVLanMember = pVLanInfo->AddVLanMember(&pMsg->member);

	HTREEITEM hParent = (HTREEITEM)((pVLanInfo->m_nTotalGroup > 1) ? pVLanInfo->m_GroupArray[pVLanMember->GroupIndex].GUIHandle : pVLanInfo->GUIHandle);
	m_vividTree.InsertHost(pVLanMember, hParent);

	pVLanInfo->UpdataToolTipString();

	m_vividTree.RedrawWindow();
	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::HostOnline(stGUIEventMsg *pData)
{
	if(pData->dwResult)
	{
		USHORT &DriverMapIndex = pData->member.DriverMapIndex;
		if(DriverMapIndex != INVALID_DM_INDEX)
			m_TrafficTable.SetZero(DriverMapIndex);
		printx(_T("User: %s (UID: 0x%08x Driver Map Index: %d) is online. %d\n"), pData->member.HostName, pData->member.dwUserID, DriverMapIndex, pData->member.LinkState);
	}
	else
	{
		ASSERT(pData->member.DriverMapIndex == INVALID_DM_INDEX);
		GHostInfoManager.Close(pData->member.dwUserID);

		time_t t = time(0);
		GNetworkManager.UpdateHostData(0, pData->member.dwUserID, UDF_ONLINE_TIME, &t, sizeof(t));
	//	GChatManager.OnHostOffline(pData->member.dwUserID);

		printx(_T("User: %s (UID: 0x%08x) is offline.\n"), pData->member.HostName, pData->member.dwUserID);
	}

	if(GNetworkManager.UpdateHost(pData->member.dwUserID, &pData->member))
		m_vividTree.RedrawWindow();

	ReleaseGUIEventMsg(pData);
}

void CVPNClientDlg::RemoveUser(stGUIEventMsg *pMsg)
{
//	printx("---> CVPNClientDlg::RemoveUser().\n");
	GHostInfoManager.Close(pMsg->DWORD_1, pMsg->DWORD_2); // Close info dialog first.

	stGUIVLanInfo *pVLanInfo = GNetworkManager.RemoveUser(pMsg->DWORD_1, pMsg->DWORD_2);

	if(pVLanInfo && pVLanInfo->CanHideDefaultGroup())
		m_vividTree.HideDefaultGroup(pVLanInfo);

	//if(pMsg->DWORD_3)
	//{
	//	ASSERT(pMsg->DWORD_3 == LS_NO_CONNECTION);
	//	USHORT DriverMapIndex = INVALID_DM_INDEX;
	//	GNetworkManager.SetMemberLinkState(0, pMsg->DWORD_2, LS_NO_CONNECTION, &DriverMapIndex, 0, 0);
	//}

	if(m_NotifyWnd)
		::SendMessage(m_NotifyWnd, WM_MEMBER_REMOVED_EVENT, pMsg->DWORD_2, 0);

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::ExitNetResult(stGUIEventMsg *pMsg)
{
	if(pMsg->dwResult == CSRC_SUCCESS || pMsg->dwResult == CSRC_NOTIFY)
	{
		GHostInfoManager.Close(pMsg->DWORD_2, 0);
		if(GNetworkManager.RemoveVLan(pMsg->DWORD_2))
		{
			CStreamBuffer sb;
			sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
			GNetworkManager.UpdateLinkStateFromStream(sb);
		}
		else
			printx(_T("Failed to exit(del) from net %s.\n"), pMsg->string);
	}
	else
	{
		printx(_T("Exit(Del) net %s was rejected by server!\n"), pMsg->string);
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnOfflineNetResult(stGUIEventMsg *pMsg)
{
	DWORD dwResult = pMsg->DWORD_1, NetID = pMsg->DWORD_2, bOnline = pMsg->DWORD_3;

	if(dwResult == CSRC_SUCCESS)
	{
		GNetworkManager.OfflineSubnet(bOnline, NetID);

		CStreamBuffer sb;
		sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
		GNetworkManager.UpdateLinkStateFromStream(sb);
		sb.DetachBuffer();
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnOfflineNetPeer(stGUIEventMsg *pData)
{
//	printx("---> OnOfflineNetPeer.\n");

	GNetworkManager.OfflineNetMember(pData->DWORD_2, pData->DWORD_3, pData->DWORD_1);

	ReleaseGUIEventMsg(pData);
}

void CVPNClientDlg::UpdateNetList(stGUIEventMsg *pData)
{
	CStreamBuffer sb;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pVLanMember;
	USHORT i, j, NetCount, HostCount;

	sb.AttachBuffer((BYTE*)pData->GetHeapData(), pData->GetHeapDataSize());
	for(sb >> NetCount, i = 0; i < NetCount; i++)
	{
		sb >> HostCount;

		pVLanInfo = GNetworkManager.CreateVNet();
		pVLanInfo->ReadFromStream(sb);
		m_vividTree.InsertVirtualLan(pVLanInfo);

		for(j = 0; j < HostCount; j++)
		{
			pVLanMember = pVLanInfo->AddVLanMember(0);
			pVLanMember->ReadFromStream(sb);

			if(pVLanMember->LinkState != LS_OFFLINE)
				pVLanInfo->nOnline++;
			m_vividTree.InsertHost(pVLanMember, (HTREEITEM)pVLanInfo->GetMemberGUIParentHandle(pVLanMember));
		}

		if(pVLanInfo->CanHideDefaultGroup())
			m_vividTree.HideDefaultGroup(pVLanInfo); // Don't show default group.

		pVLanInfo->UpdataToolTipString();
	}
	ASSERT(sb.GetDataSize() == pData->GetHeapDataSize());
	sb.DetachBuffer();

	ReleaseGUIEventMsg(pData);

	if(IsFirstTimeUpdate()) // Update GUI and state flag.
	{
		GUILoadNetInfo();
		FlagAdd(DF_LOGIN);
		FlagClean(DF_INIT_UI | DF_TRY_LOGIN);
		SetLoginButtonState();
		GUISetHostName();
		SetShellNotifyData();
	}
	else if(NetCount)
	{
		ASSERT(NetCount == 1 && pVLanInfo);
		GNetworkManager.UpdateAllMemberFromVNet(pVLanInfo);
	}

	m_vividTree.RedrawWindow();
}

void CVPNClientDlg::UpdateConnectState(stGUIEventMsg *pData)
{
	if(pData->dwResult)
	{
		stGUIVLanMember &member = pData->member;
		GNetworkManager.SetMemberLinkState(0, member.dwUserID, LS_CONNECTED, &member.DriverMapIndex, &member.eip, &member.eip.m_port);
	}
	else
	{
		// Failed to connect.
		stGUIVLanMember &member = pData->member;
		GNetworkManager.SetMemberLinkState(0, member.dwUserID, LS_NO_TUNNEL, &member.DriverMapIndex, 0, 0);
	}

	ReleaseGUIEventMsg(pData);
}

void CVPNClientDlg::OnLoginResult(DWORD dwResult)
{
	printx("---> OnLoginResult: %d.\n", dwResult);

	if(dwResult < LRC_LOGIN_CODE)
	{
		switch(dwResult)
		{
			case LRC_LOGIN_SUCCESS:
				if(m_NotifyWnd)
					::PostMessage(m_NotifyWnd, WM_SERVER_RESPONSE_EVENT, dwResult, 0);
				m_TrafficTable.ZeroCacheTable();
				AddJobDataExchange(TRUE, DET_LOGIN_INFO, 0);
				break;

			case LRC_NAME_NOT_FOUND:
			case LRC_PASSWORD_ERROR:
				break;

			case LRC_REG_ID_NOT_FOUND:
				if(AfxMessageBox(GUILoadString(IDS_STRING1506), MB_YESNO) == IDYES)
				{
					stRegisterID RegID;
					AddJobDataExchange(FALSE, DET_REG_ID, &RegID);
				}
				break;

			case LRC_UNSUPPORTED_VERSION:
				AfxMessageBox(GUILoadString(IDS_STRING1500));
				break;

			case LRC_SERVER_REJECTED:
				break;

			case LRC_CONNECTING:
				FlagAdd(DF_INIT_UI | DF_TRY_LOGIN);
				SetLoginButtonState();
				GUISetHostName();
				SetShellNotifyData();
				break;

			case LRC_ADAPTER_ERROR:
				AfxMessageBox(_T("Virtual adapter error!"));
				break;

			case LRC_SOCKET_ERROR:
				AfxMessageBox(_T("Failed to create socket!"));
				break;

			case LRC_UDP_PORT_ERROR:
				AfxMessageBox(_T("Failed to open tunnel UDP port!"));
				break;

			case LRC_CONNECT_FAILED: // No use.
				break;

			case LRC_THREAD_FAILED:
				break;
		}

		if(dwResult != LRC_LOGIN_SUCCESS && dwResult != LRC_CONNECTING) // Failed to login.
		{
			if(m_NotifyWnd)
				::PostMessage(m_NotifyWnd, WM_SERVER_RESPONSE_EVENT, dwResult, 0);

			FlagClean(DF_LOGIN | DF_TRY_LOGIN);
			SetLoginButtonState();
			GUISetHostName();
			SetShellNotifyData();
		}
	}
	else
	{
		// Close GUI first.
		GChatManager.OnAppLogout();
		GHostInfoManager.CloseAll();
		GUISaveNetInfo();
		m_vividTree.Release();
		GNetworkManager.Release();  // Release at last.

		FlagClean(DF_LOGIN | DF_TRY_LOGIN); // Must clean two flags here.
		SetLoginButtonState();
		GUISetHostName();
		SetShellNotifyData();

		switch(dwResult)
		{
			case LRC_LOGOUT_SUCCESS:
				break;
			case LRC_LOGOUT_DISCONNECT:
				break;
			case LRC_LOGOUT_NOT_LOGIN:
				break;
			case LRC_QUERY_ADDRESS_FAILED:
				AfxMessageBox(GUILoadString(IDS_STRING1505));
				break;
			case LRC_LOGOUT_CORE_ERROR:
				break;
		}
	}
}

void CVPNClientDlg::OnRegisterEvent(stGUIEventMsg *pMsg)
{
	GUIUpdateVirtualIP(pMsg->DWORD_5);

	if(m_nMatrixCore.IsPureGUIMode())
	{
		stRegisterID regID;
		regID.d1 = pMsg->DWORD_1;
		regID.d2 = pMsg->DWORD_2;
		regID.d3 = pMsg->DWORD_3;
		regID.d4 = pMsg->DWORD_4;
		AppSaveRegisterID(&regID);
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnUpdateMemberState(stGUIEventMsg *pMsg)
{
	CStreamBuffer sb;

	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	GNetworkManager.UpdateLinkStateFromStream(sb);
	sb.DetachBuffer();

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::ReadTrafficInfo(stGUIEventMsg *pMsg)
{
	USHORT count;
	CStreamBuffer sb;
	sb.AttachBuffer((BYTE*)pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	sb >> count;
	sb.Read(m_TrafficTable.DataIn, sizeof(DWORD) * count);
	sb.Read(m_TrafficTable.DataOut, sizeof(DWORD) * count);
	ASSERT(sb.GetDataSize() == pMsg->GetHeapDataSize());
	sb.DetachBuffer();

	UINT bChangeCount = 0;
	for(USHORT i = 0; i < count; ++i)
	{
		if(m_TrafficTable.DataIn[i] != m_TrafficTable.DataInCache[i])
		{
			m_TrafficTable.DataInCache[i] = m_TrafficTable.DataIn[i];
			if(!m_TrafficTable.bDownload[i])
			{
				m_TrafficTable.bDownload[i] = true;
				bChangeCount++;
			}
		}
		else
		{
			if(m_TrafficTable.bDownload[i])
			{
				m_TrafficTable.bDownload[i] = false;
				bChangeCount++;
			}
		}

		if(m_TrafficTable.DataOut[i] != m_TrafficTable.DataOutCache[i])
		{
			m_TrafficTable.DataOutCache[i] = m_TrafficTable.DataOut[i];
			if(!m_TrafficTable.bUpload[i])
			{
				m_TrafficTable.bUpload[i] = true;
				bChangeCount++;
			}
		}
		else
		{
			if(m_TrafficTable.bUpload[i])
			{
				m_TrafficTable.bUpload[i] = false;
				bChangeCount++;
			}
		}
	}

	if(m_TrafficTable.bFirstTimeUpdateTrafficTable)
	{
		ZeroMemory(m_TrafficTable.bUpload, sizeof(m_TrafficTable.bUpload));
		ZeroMemory(m_TrafficTable.bDownload, sizeof(m_TrafficTable.bDownload));
		m_TrafficTable.bFirstTimeUpdateTrafficTable = FALSE;
	}
	else if(bChangeCount)
		m_vividTree.RedrawWindow();

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnRelayEvent(stGUIEventMsg *pMsg)
{
	printx("---> OnRelayEvent\n");

	if(pMsg->DWORD_3 == MASTER_SERVER_RELAY_ID)
	{
		if(pMsg->DWORD_1)
			GNetworkManager.SetMemberLinkState(0, pMsg->DWORD_2, LS_SERVER_RELAYED, 0, 0, 0, &pMsg->DWORD_3);
		else
			GNetworkManager.SetMemberLinkState(0, pMsg->DWORD_2, (USHORT)pMsg->DWORD_4, 0, 0, 0, &pMsg->DWORD_3);
	}
	else
		GNetworkManager.SetMemberLinkState(0, pMsg->DWORD_2, LS_RELAYED_TUNNEL, 0, 0, 0, &pMsg->DWORD_3);

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::InitLanguageData(DWORD LanID)
{
	CMenu menu, *pMenu = GetMenu(), *pSubMenu;
	BOOL bFirstTimeInit = FALSE;

	if(!pMenu)
	{
		bFirstTimeInit = TRUE;
		menu.LoadMenu(IDR_MAINMENU);
		pMenu = &menu;
	}

//	if(LanID != DEFAULT_LANGUAGE_ID)
	{
		pMenu->ModifyMenu(0, MF_STRING | MF_BYPOSITION, 0, GUILoadString(IDS_STRING450)); // System.
		pMenu->ModifyMenu(1, MF_STRING | MF_BYPOSITION, 0, GUILoadString(IDS_STRING453)); // Network.
		pMenu->ModifyMenu(2, MF_STRING | MF_BYPOSITION, 0, GUILoadString(IDS_STRING456)); // Help.

		pSubMenu = pMenu->GetSubMenu(0);
		pSubMenu->ModifyMenu(0, MF_STRING | MF_BYPOSITION, ID_SYSTEM_SETTINGS, GUILoadString(IDS_STRING451));
		pSubMenu->ModifyMenu(3, MF_STRING | MF_BYPOSITION, ID_SYSTEM_EXIT, GUILoadString(IDS_STRING452));
		pSubMenu = pMenu->GetSubMenu(1);
		pSubMenu->ModifyMenu(0, MF_STRING | MF_BYPOSITION, ID_NETWORK_CREATENETWORK, GUILoadString(IDS_STRING454));
		pSubMenu->ModifyMenu(1, MF_STRING | MF_BYPOSITION, ID_NETWORK_JOINNETWORK, GUILoadString(IDS_STRING455));
		pSubMenu = pMenu->GetSubMenu(2);
		pSubMenu->ModifyMenu(0, MF_STRING | MF_BYPOSITION, ID_HELP_CHECKUPDATE, GUILoadString(IDS_STRING457));
		pSubMenu->ModifyMenu(2, MF_STRING | MF_BYPOSITION, ID_HELP_SERVERNEWS, GUILoadString(IDS_STRING458));
		pSubMenu->ModifyMenu(3, MF_STRING | MF_BYPOSITION, ID_HELP_WEBSITE, GUILoadString(IDS_STRING459));
		pSubMenu->ModifyMenu(5, MF_STRING | MF_BYPOSITION, ID_HELP_UNINSTALLADAPTERDRIVER, GUILoadString(IDS_STRING460));
		pSubMenu->ModifyMenu(7, MF_STRING | MF_BYPOSITION, ID_HELP_ABOUT, GUILoadString(IDS_STRING461));
	}

	if(bFirstTimeInit)
	{
		SetMenu(pMenu);
		pMenu->Detach();
	}

	m_PopupShellIconMenu.RemoveMenu(0, MF_BYPOSITION);
	m_PopupShellIconMenu.RemoveMenu(0, MF_BYPOSITION);
	m_PopupShellIconMenu.AppendMenu(MF_STRING|MF_ENABLED, ID_HELP_ABOUT, GUILoadString(IDS_STRING461));
	m_PopupShellIconMenu.AppendMenu(MF_STRING|MF_ENABLED, ID_SYSTEM_EXIT, GUILoadString(IDS_STRING452));

	m_PopupMenuMode = PMM_NULL;
	csGColon = GUILoadString(IDS_COLON);

	SetLoginButtonState();
	GUISetHostName();
	SetShellNotifyData();

	m_vividTree.CacheLanguageData();
}

void CVPNClientDlg::ReadLoginData(stGUIEventMsg *pMsg)
{
	DWORD vip = 0;
	CStreamBuffer sb;
	BYTE byLen;
	TCHAR buffer[1024];

	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	sb >> m_HostUID >> vip >> m_ServerCtrlFlag;
	sb.ReadString(byLen, _countof(buffer) - 1, buffer, sizeof(buffer));
	m_LoginName = buffer;
	sb.ReadString(byLen, _countof(buffer) - 1, buffer, sizeof(buffer));
	m_LoginPassword = buffer;
	sb.Read(&m_RegID, sizeof(m_RegID));
//	printx("Server ctrl flag: %08x\n", m_ServerCtrlFlag);

	GUIUpdateVirtualIP(vip);
	GChatManager.SetHostUID(m_HostUID);
}

void CVPNClientDlg::OnDataExchange(stGUIEventMsg *pMsg)
{
	switch(pMsg->dwResult)
	{
		case DET_LOGIN_INFO:
			ReadLoginData(pMsg);
			break;

		case DET_REG_ID:
			break;

		case DET_CONFIG_DATA:
			break;
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnSettingResponse(stGUIEventMsg *pMsg)
{
//	printx("---> OnSettingResponse().\n");

	BOOL bSet = pMsg->DWORD_1 & JST_SET_DATA;
	DWORD dwType = pMsg->DWORD_1 & ~JST_SET_DATA;

	switch(dwType)
	{
		case JST_NET_PASSWORD:
	//		printx("GUI event: Reset network password.\n");
			break;

		case JST_NET_FLAG:
	//		printx("GUI event: Reset network flag.\n");
			GNetworkManager.UpdateVLanFlag(pMsg->DWORD_3, pMsg->DWORD_4);
			break;

		case JST_HOST_ROLE_HUB:
		case JST_HOST_ROLE_RELAY:
			if(GNetworkManager.UpdateMemberRole(pMsg->DWORD_2, pMsg->DWORD_3, pMsg->DWORD_4, pMsg->DWORD_5))
				printx("GUI update host role OK.\n");

			if(dwType == JST_HOST_ROLE_RELAY && m_NotifyWnd != NULL)
				::SendMessage(m_NotifyWnd, WM_RELAY_HOST_CHANGED, (WPARAM)pMsg->DWORD_3, 0);

			break;
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnClientQuery(stGUIEventMsg *pMsg)
{
//	printx("---> GUI:OnClientQuery. rcode:%d\n", pMsg->DWORD_2);

	switch(pMsg->DWORD_1)
	{
		case CQT_SERVER_NEWS:
			if(pMsg->GetHeapData())
			{
				m_ServerMsgID = pMsg->DWORD_3;
				m_ClosedMsgID = pMsg->DWORD_4;
				m_ServerNews = (TCHAR*)pMsg->GetHeapData();
				printx("Server Msg ID: %u. Closed ID: %u.\n", pMsg->DWORD_3, pMsg->DWORD_4);
				if(pMsg->DWORD_3 > pMsg->DWORD_4)
					OnHelpServernews();
			}
			break;

		case CQT_HOST_LAST_ONLINE_TIME:
			{
				time_t t = (((INT64)pMsg->DWORD_3) << 32) | pMsg->DWORD_4;
				GNetworkManager.UpdateHostData(0, pMsg->member.dwUserID, UDF_ONLINE_TIME, &t, sizeof(t));
		//		GHostInfoManager.UpdateOnlineTime(pMsg->member.dwUserID, t);
			}
			break;

		case CQT_VNET_MEMBER_INFO:
			if(m_NotifyWnd)
				::SendMessage(m_NotifyWnd, WM_VNET_MEMBER_INFO, (WPARAM)pMsg, 0);
			break;
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnUpdateAdapterConfiguration(DWORD dw)
{
//	printx("---> OnUpdateAdapterConfiguration().\n");
	static CInfoDialog *pInfoDialog = 0;

	if(dw == 1)
	{
		CInfoDialog info;
		pInfoDialog = &info;
		info.SetMsg(GUILoadString(IDS_STRING1503));
		info.DoModal();
		pInfoDialog = 0;
	}
	else
	{
		if(pInfoDialog)
			pInfoDialog->EndDialog(0);
	}
}

void CVPNClientDlg::OnCoreServiceError(DWORD dwErrorCode)
{
//	printx("---> GUI: OnCoreServiceError: %d\n", dwErrorCode);

	switch(dwErrorCode)
	{
		case CSEC_TUNNEL_ERROR:
			AfxMessageBox(_T("Tunnel Engine Error!"));
			break;
	}
}

void CVPNClientDlg::OnRelayInfo(stGUIEventMsg *pMsg)
{
	UINT i, nCount;
	CStreamBuffer sb;
	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	sb >> nCount;

	printx("---> GUI:OnRelayInfo %d\n", nCount);

	DWORD UID, RelayNetworkID;
	for(i = 0; i < nCount; i++)
	{
		sb >> UID >> RelayNetworkID;
		GNetworkManager.SetMemberLinkState(0, UID, LS_RELAYED_TUNNEL, 0, 0, 0, &RelayNetworkID);
	}

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnServiceVersion(DWORD dwServiceVersion)
{
//	INT main, sub, buildnum;
//	AppGetVersionDetail(dwServiceVersion, main, sub, buildnum);
//	printx("Service version: %d.%d.%d\n", main, sub, buildnum);

	DWORD dwGUIVersion = AppGetVersion();
	if(dwGUIVersion > dwServiceVersion)
	{
		BOOL bResult = UpdateService(this, &m_nMatrixCore);
//		printx("Update service result: %d\n", bResult);
	}
	else if(dwGUIVersion < dwServiceVersion)
	{
		m_nMatrixCore.Close();
		AfxMessageBox(GUILoadString(IDS_STRING1504));
		EndDialog(0);
	}
	else
		AddJobServiceState();
}

void CVPNClientDlg::OnSubnetSubgroup(stGUIEventMsg *pMsg)
{
	stGUIVLanInfo *pVNet;
	UINT nIndex, bRedrawTreeCtrl = 0, nOldGroupIndex;
	CStreamBuffer sb;
	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());
	DWORD dw1, dw2, dw3, dw4, dw5, dw6;
	sb >> dw1 >> dw2 >> dw3 >> dw4 >> dw5; // Don't read more data here.

	printx("---> OnSubnetSubgroup: %d\n", dw1);

	switch(dw1)
	{
		case SSCT_CREATE:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;
			nIndex = pVNet->CreateGroup(dw4, (stVNetGroup*)sb.GetCurrentBuffer(), dw5);
			if(!nIndex)
				break;

			if(nIndex == 1)
			{
				ASSERT(pVNet->m_nTotalGroup == 2);
				m_vividTree.InitDefaultGroup(pVNet);
				pVNet->m_GroupArray[0].VNetGroup.Flag |= VGF_DEFAULT_GROUP;
				m_vividTree.InsertVNetGroup(&pVNet->m_GroupArray[0], (HTREEITEM)pVNet->GUIHandle);
				m_vividTree.MoveAllMembers((HTREEITEM)pVNet->GUIHandle, (HTREEITEM)pVNet->m_GroupArray[0].GUIHandle);
			}
			m_vividTree.InsertVNetGroup(&pVNet->m_GroupArray[nIndex], (HTREEITEM)pVNet->GUIHandle);
			pVNet->UpdataToolTipString();
			bRedrawTreeCtrl = TRUE;

			break;

		case SSCT_DELETE:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;

			HTREEITEM hItem;
			if(!pVNet->DeleteGroup(dw4, (void**)&hItem))
				break;

			m_vividTree.DeleteVNetGroup(hItem, pVNet, dw4);
			pVNet->UpdataToolTipString();
			bRedrawTreeCtrl = TRUE;

			break;

		case SSCT_MOVE:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;

			if(dw4 == m_HostUID)
			{
				nOldGroupIndex = pVNet->GroupIndex;
				pVNet->GroupIndex = (BYTE)dw5;
			}
			else
			{
				stGUIVLanMember *pMember = pVNet->UpdateMemberGroupIndex(dw4, dw5, nOldGroupIndex);
				if(!pMember)
					break;

				m_vividTree.MoveMember(pVNet, pMember, dw5);
				bRedrawTreeCtrl = TRUE;
			}

			if(nOldGroupIndex == 0 && pVNet->CanHideDefaultGroup())
				m_vividTree.HideDefaultGroup(pVNet);
			pVNet->UpdataToolTipString();

			break;

		case SSCT_OFFLINE:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;

			sb >> dw6;
			if(dw4 == m_HostUID)
			{
				pVNet->UpdateGroupBitMask(dw5, dw6);
			}
			else
			{
				stGUIVLanMember *pMember = pVNet->FindMember(dw4);
				if(pMember)
					pMember->UpdateGroupBitMask(dw5, dw6);
			}
			bRedrawTreeCtrl = TRUE;

			break;

		case SSCT_SET_FLAG:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;

			sb >> dw6;

			switch (dw5) // Check mask type.
			{
				case VGF_DEFAULT_GROUP:

					pVNet->SetDefaultGroup(dw4);
					if(pVNet->CanHideDefaultGroup())
						m_vividTree.HideDefaultGroup(pVNet);
					bRedrawTreeCtrl = TRUE;
					break;

			//	case VGF_STATE_OFFLINE:
			//		break;

				default:
					pVNet->UpdateGroupFlag(dw4, dw5, dw6);
					bRedrawTreeCtrl = TRUE;
					break;
			}

			break;

		case SSCT_RENAME:

			if(dw2 != CSRC_SUCCESS && dw2 != CSRC_NOTIFY)
				break;
			if((pVNet = GNetworkManager.FindVNet(dw3)) == 0)
				break;
			if(!pVNet->UpdateGroupName(dw4, (TCHAR*)sb.GetCurrentBuffer(), dw5))
				break;

			if(m_NotifyWnd)
				::PostMessage(m_NotifyWnd, WM_UPDATE_GROUP_NAME, dw3, dw4);
			bRedrawTreeCtrl = TRUE;
			break;
	}

	sb.DetachBuffer();
	if(bRedrawTreeCtrl)
		m_vividTree.RedrawWindow();

	ReleaseGUIEventMsg(pMsg);
}

void CVPNClientDlg::OnClientProfile(stGUIEventMsg *pMsg)
{
	printx("---> GUI::OnClientProfile\n");

	DWORD dw1, dw2, dw3, dw4;
	TCHAR HostName[MAX_HOST_NAME_LENGTH + 1];
	CStreamBuffer sb;
	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());

	sb >> dw1 >> dw2;

	switch(dw1)
	{
		case CPT_HOST_NAME:

			sb >> dw3 >> dw4;
			sb.Read(HostName, sizeof(TCHAR) * dw4);
			HostName[dw4] = 0;

			if(dw2 == 1)
				GUISetHostName();
			else if(dw2 == 2)
			{
				if(GNetworkManager.UpdateMemberProfile(&m_vividTree, dw3, dw1, HostName, sizeof(TCHAR) * (dw4 + 1)))
					m_vividTree.RedrawWindow();
			}

			break;
	}

	sb.DetachBuffer();
	ReleaseGUIEventMsg(pMsg);
}

LRESULT CVPNClientDlg::OnGUIEvent(WPARAM wParam, LPARAM lParam)
{
//	printx("---> OnGUIEvent(). Type: %d\n", wParam);
	stGUIEventMsg *pMsg = (stGUIEventMsg*)lParam;

	switch(wParam)
	{
		case GET_UPDATE_NET_LIST:
			UpdateNetList(pMsg);
			break;

		case GET_ADD_USER:
			AddUser(pMsg);
			break;

		case GET_HOST_ONLINE:
			HostOnline(pMsg);
			break;

		case GET_HOST_EXIT_NET:
			RemoveUser(pMsg);
			break;

		case GET_DELETE_NET_RESULT:
		case GET_EXIT_NET_RESULT:
			ExitNetResult(pMsg);
			break;

		case GET_OFFLINE_SUBNET:
			OnOfflineNetResult(pMsg);
			break;

		case GET_OFFLINE_SUBNET_PEER:
			OnOfflineNetPeer(pMsg);
			break;

		case GET_UPDATE_CONNECT_STATE:
			UpdateConnectState(pMsg);
			break;

		case GET_JOIN_NET_RESULT:
			if(m_NotifyWnd)
				::PostMessage(m_NotifyWnd, WM_SERVER_RESPONSE_EVENT, lParam, 0);
			break;

		case GET_CREATE_NET_RESULT:
			if(m_NotifyWnd)
				::PostMessage(m_NotifyWnd, WM_SERVER_RESPONSE_EVENT, lParam, 0);
			break;

		case GET_RELAY_EVENT:
			OnRelayEvent(pMsg);
			break;

		case GET_REGISTER_EVENT:
			OnRegisterEvent(pMsg);
			break;

		case GET_REGISTER_RESULT:
		//	if(m_NotifyWnd)
		//		::PostMessage(m_NotifyWnd, WM_SERVER_RESPONSE_EVENT, lParam, 0);
			break;

		case GET_LOGIN_EVENT:
			OnLoginResult(lParam);
			break;

		case GET_UPDATE_MEMBER_STATE:
			OnUpdateMemberState(pMsg);
			break;

		case GET_UPDATE_TRAFFIC_INFO:
			ReadTrafficInfo(pMsg);
			break;

		case GET_DATA_EXCHANGE:
			OnDataExchange(pMsg);
			break;

		case GET_ON_RECEIVE_CHAT_TEXT:
			GChatManager.OnReceiveText(pMsg->member.HostName, pMsg->dwResult, pMsg->string);
			ReleaseGUIEventMsg(pMsg);
			break;

		case GET_SETTING_RESPONSE:
			OnSettingResponse(pMsg);
			break;

		case GET_CLIENT_QUERY:
			OnClientQuery(pMsg);
			break;

		case GET_UPDATE_ADAPTER_CONFIG:
			OnUpdateAdapterConfiguration(lParam);
			break;

		case GET_CORE_ERROR:
			OnCoreServiceError(lParam);
			break;

		case GET_RELAY_INFO:
			OnRelayInfo(pMsg);
			break;

		case GET_SERVICE_VERSION:
			OnServiceVersion(lParam);
			break;

		case GET_SERVICE_STATE:
			if(lParam == SSC_DELETE_READY)
			{
				CloseServiceMode();
				break;
			}
			m_nMatrixCore.Close();
			m_nMatrixCore.Init(GetSafeHwnd());
			OnLoginResult(LRC_LOGOUT_SUCCESS);
			break;

		case GET_PING_HOST:
			if(m_DlgPingHost.GetSafeHwnd())
				m_DlgPingHost.PostMessage(WM_PING_HOST_RESPONSE, 0, lParam);
			break;

		case GET_GROUP_CHAT:
			GChatManager.OnGroupChatEvent(pMsg);
			break;

		case GET_SUBNET_SUBGROUP:
			OnSubnetSubgroup(pMsg);
			break;

		case GET_CLIENT_PROFILE:
			OnClientProfile(pMsg);
			break;

		case GET_READY:
			break;
	}

	return 0;
}

LRESULT CVPNClientDlg::OnShellNotify(WPARAM wParam, LPARAM lParam)
{
	CPoint point;
	CWnd *pPopup = GetWindow(GW_ENABLEDPOPUP);

	switch(lParam)
	{
		case WM_RBUTTONDOWN:
			if(pPopup)
				return 0;
			GetCursorPos(&point);
			SetForegroundWindow();
			m_PopupShellIconMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, 0);
			break;

		case WM_LBUTTONDBLCLK:
			FlagAdd(DF_VISIBLE);
		//	m_vividTree.RedrawWindow();
			ShowWindow(SW_RESTORE);
		//	AnimateWindow(WND_ANI_TIME, AW_BLEND);

			if(pPopup) // For windows that already show normally.
				pPopup->SetForegroundWindow();
			else
				SetForegroundWindow();
			break;
	}

	return 1;
}

void CVPNClientDlg::OnNMDblclkNetTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	TCHAR path[MAX_PATH];
	CString strDirectory, strVIP;

	*pResult = 0;

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;
	stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
	if(!pObject)
		return;

	if(m_vividTree.IsVLanMember(pObject))
	{
		stGUIVLanMember *pMember = (stGUIVLanMember*)pObject;

		if(GetKeyState(VK_MENU) < 0) // Alt key.
		{
			OnHostInformation();
		}
		else if(pMember->LinkState >= LS_RELAYED_TUNNEL)
		{
			GetSystemDirectory(path, MAX_PATH);
			DWORD vip = pMember->vip;
			strVIP.Format(_T("-t %d.%d.%d.%d"), vip & 0xff, (vip >> 8) & 0xff, (vip >> 16) & 0xff, (vip >> 24) & 0xff);
			ShellExecute(GetSafeHwnd(), _T("open"), _T("ping.exe"), strVIP, strDirectory, SW_SHOW);
		}
	}
	//else if(m_vividTree.IsVLanGroup(pObject))
	//{
	//	if(GetKeyState(VK_MENU) < 0)
	//		OnSubgroupManage();
	//}
	//else
	//{
	//	if(GetKeyState(VK_MENU) < 0)
	//		OnNetworkManage();
	//}
}

void CVPNClientDlg::OnNMRClickNetTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point, ClientPoint;
	GetCursorPos(&point);
	ClientPoint = point;
	m_vividTree.ScreenToClient(&ClientPoint);

	HTREEITEM hItem = m_vividTree.HitTest(ClientPoint, 0); // Must do hit test here.
//	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	if(m_vividTree.IsVLanItem(hItem))
	{
		SetPopupMenuMode(PMM_VIRTUAL_LAN, m_vividTree.GetVLanInfo(hItem)); // Prepare menu for network options.
		m_PopupMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, 0);
	}
	else
	{
		stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);

		if(m_vividTree.IsVLanMember(pObject))
		{
			SetPopupMenuMode(PMM_VLAN_MEMBER, m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)), pObject); // Prepare menu for host options.
			if(m_PopupMenu.GetMenuItemCount())
				m_PopupMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, 0);
		}
		else
		{
			SetPopupMenuMode(PMM_VLAN_GROUP, m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)), pObject); // Prepare menu for host options.
			if(m_PopupMenu.GetMenuItemCount())
				m_PopupMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, 0);
		}
	}

	*pResult = 0;
}

void CVPNClientDlg::OnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMTVGETINFOTIP>(pNMHDR);

//	printx("Enter CVPNClientDlg::OnTvnGetInfoTip(). cchTextMax: %d\n", pGetInfoTip->cchTextMax);
//	printx("hItem: %d, lParam: %d, pszText: %d\n", pGetInfoTip->hItem, pGetInfoTip->lParam, pGetInfoTip->pszText);

	if(pGetInfoTip->hItem)
	{
		CString info = m_vividTree.GetItemToolTip(pGetInfoTip->hItem);
		memcpy(pGetInfoTip->pszText, info.GetBuffer(), (info.GetLength() + 1) * sizeof(TCHAR));
	}

	*pResult = 0;
}

void CVPNClientDlg::OnTimer(UINT_PTR nIDEvent)
{
//	printx("---> CVPNClientDlg::OnTimer.\n");

	if(m_bHookInitState)
		m_bHookInitState--;

	if(!IsLogin() || !IsVisible())
		return;
	AddJobReadTraffic();
}

void CVPNClientDlg::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
	CDialog::OnWindowPosChanged(lpwndpos);

	if(lpwndpos->flags & SWP_HIDEWINDOW)
	{
	//	printx("hide windows!\n");
		FlagClean(DF_VISIBLE);
		m_TrafficTable.ZeroCacheTable();
	}
	else if(lpwndpos->flags & SWP_SHOWWINDOW)
	{
	//	printx("show windows!\n");
		FlagAdd(DF_VISIBLE);
	}
}

void CVPNClientDlg::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	if(m_bHookInitState)
		if(lpwndpos->flags & SWP_SHOWWINDOW)
			lpwndpos->flags &= ~SWP_SHOWWINDOW;

//	printx("---> OnWindowPosChanging: %d\n", lpwndpos->flags & SWP_SHOWWINDOW);
	CDialog::OnWindowPosChanging(lpwndpos);
}

void CVPNClientDlg::OnSize(UINT nType, INT cx, INT cy)
{
	CDialog::OnSize(nType, cx, cy);

	if(nType == SIZE_RESTORED && !m_bHookInitState) // Must add this or causing incorrect state when the user executes app twice.
		FlagAdd(DF_VISIBLE);

	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_NET_TREE);
	if(!pTree)
		return;

	INT nMargin = 10, nBottomMargin = 15, nHeight;
	CRect ClientRect;

	pTree->GetWindowRect(&ClientRect);
	ScreenToClient(&ClientRect);

	nHeight = cy - ClientRect.top - nBottomMargin;
	if(nHeight < 10)
		nHeight = 10;

	pTree->MoveWindow(nMargin, ClientRect.top, cx - nMargin * 2, nHeight, TRUE);
	pTree->RedrawWindow();
}

void CVPNClientDlg::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	if(!MIN_DIALOG_SIZE)
		return;

	if(pRect->bottom - pRect->top < MIN_HEIGHT)
	{
		if(fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT)
			pRect->top = pRect->bottom - MIN_HEIGHT;
		else
			pRect->bottom = pRect->top + MIN_HEIGHT;
	}
	if(pRect->right - pRect->left < MIN_WIDTH)
	{
		if(fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_LEFT || fwSide == WMSZ_TOPLEFT)
			pRect->left = pRect->right - MIN_WIDTH;
		else
			pRect->right = pRect->left + MIN_WIDTH;
	}
}

void CVPNClientDlg::OnNcLButtonDown(UINT nHitTest, CPoint point)
{
	CRect rect;
	GetWindowRect(&rect);

	m_LBClickDown = point;
	m_offset.x = point.x - rect.left;
	m_offset.y = point.y - rect.top;

	CDialog::OnNcLButtonDown(nHitTest, point);
}

void CVPNClientDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetDlgItem(IDC_VIP)->GetWindowRect(rect);
	ClientToScreen(&point);

	if(rect.left <= point.x && point.x <= rect.right)
		if(rect.top <= point.y && point.y <= rect.bottom)
		{
			// Prepare pop up menu.
			SetPopupMenuMode(PMM_COPY_ADDRESS);
			m_PopupMenu.TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this, 0);
		}

	CDialog::OnLButtonDown(nFlags, point);
}

void CVPNClientDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
//	CDialog::OnMoving(fwSide, pRect);

	CRect rect, rectDesktop;
	GetWindowRect(&rect);
	SystemParametersInfo(SPI_GETWORKAREA, NULL, &rectDesktop, NULL);
	const INT nStickSpace = 5;

	CPoint point;
	GetCursorPos(&point);

	BOOL bForceMoveX = (Abs(point.x - m_LBClickDown.x) > nStickSpace) ? TRUE : FALSE;
	BOOL bForceMoveY = (Abs(point.y - m_LBClickDown.y) > nStickSpace) ? TRUE : FALSE;

	if(bForceMoveX || bForceMoveY)
	{
		if(Abs(pRect->left - rectDesktop.left) < nStickSpace && !bForceMoveX)
		{
			pRect->left = rectDesktop.left;
			pRect->right = rectDesktop.left + rect.Width();
		}
		else if(Abs(pRect->right - rectDesktop.right) < nStickSpace && !bForceMoveX)
		{
			pRect->left = rectDesktop.right - rect.Width();
			pRect->right = rectDesktop.right;
		}
		else
		{
			pRect->left = point.x - m_offset.x;
			pRect->right = pRect->left + rect.Width();
			m_LBClickDown.x = point.x;
		}

		if(Abs(pRect->top - rectDesktop.top) < nStickSpace && !bForceMoveY)
		{
			pRect->top = rectDesktop.top;
			pRect->bottom = rectDesktop.top + rect.Height();
		}
		else if(Abs(pRect->bottom - rectDesktop.bottom) < nStickSpace && !bForceMoveY)
		{
			pRect->top = rectDesktop.bottom - rect.Height();
			pRect->bottom = rectDesktop.bottom;
		}
		else
		{
			pRect->top = point.y - m_offset.y;
			pRect->bottom = pRect->top + rect.Height();
			m_LBClickDown.y = point.y;
		}

		return;
	}

	if(Abs(pRect->top - rectDesktop.top) < nStickSpace)
	{
		pRect->top = rectDesktop.top;
		pRect->bottom = rectDesktop.top + rect.Height();
	}
	if(Abs(pRect->bottom - rectDesktop.bottom) < nStickSpace)
	{
		pRect->top = rectDesktop.bottom - rect.Height();
		pRect->bottom = rectDesktop.bottom;
	}
	if(Abs(pRect->left - rectDesktop.left) < nStickSpace)
	{
		pRect->left = rectDesktop.left;
		pRect->right = rectDesktop.left + rect.Width();
	}
	if(Abs(pRect->right - rectDesktop.right) < nStickSpace)
	{
		pRect->left = rectDesktop.right - rect.Width();
		pRect->right = rectDesktop.right;
	}
}

UINT CVPNClientDlg::OnPowerBroadcast(UINT nPowerEvent, LPARAM nEventData)
{
	if(!m_nMatrixCore.IsPureGUIMode()) // App mode.
	{
		switch(nPowerEvent)
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

	return CDialog::OnPowerBroadcast(nPowerEvent, nEventData);
}

void CVPNClientDlg::OnSystemSettings()
{
	CDlgSetting	DlgSetting(this, &m_nMatrixCore, &m_ConfigData);

	INT i = DlgSetting.DoModal();
	if(i == IDOK)
	{
		if(m_nMatrixCore.IsPureGUIMode())
		{
		}
//		if(m_HostName != m_ConfigData.LocalName && IsLogin())
//			GUISetHostName();
	}
	else if(i == 3)
		AfxMessageBox(GUILoadString(IDS_STRING1504));
}

void CVPNClientDlg::OnSystemSetloginaccount()
{
	ASSERT(!m_NotifyWnd);
	CIDLoginDlg dlg;
	dlg.SetMode(&m_NotifyWnd, IsLogin());
	dlg.SetString(m_LoginName, m_LoginPassword, &m_ServerCtrlFlag);
	if(dlg.DoModal() == 1)
	{
		m_LoginName = dlg.GetLgName();
		m_LoginPassword = dlg.GetLgPassword();
	}
}

void CVPNClientDlg::OnSystemExit()
{
	if(m_nMatrixCore.IsPureGUIMode())
		GChatManager.OnAppGuiExit();

	m_nMatrixCore.Close();
	EndDialog(0);
}

void CVPNClientDlg::OnNetworkCreatenetwork()
{
	ASSERT(!m_NotifyWnd);
	CDlgCreateSubnet DlgCreateSubnet;
	DlgCreateSubnet.SetPointer(m_HostUID, &m_NotifyWnd);
	DlgCreateSubnet.DoModal();
}

void CVPNClientDlg::OnNetworkJoinnetwork()
{
	ASSERT(!m_NotifyWnd);
	CDlgJoinNet	DlgJoinSubnet;
	DlgJoinSubnet.SetPointer(m_HostUID, &m_NotifyWnd);
	DlgJoinSubnet.DoModal();
}

void CVPNClientDlg::OnSwitchNetworkLocation()
{
//	AfxMessageBox(_T("No function"));

	SwitchNT6NetworkLocation();
}

void CVPNClientDlg::OnHelpCheckupdate()
{
	AfxMessageBox(_T("No function"));
}

void ShowHtmlNews(CString &param)
{

// ERROR_SXS_CANT_GEN_ACTCTX SE_ERR_ACCESSDENIED

	//INT nError = (INT)ShellExecute(0, _T("open"), _T("c:\\test.exe"), 0, 0, SW_NORMAL);
	//if(nError <= 32)
	//{
	//	CString info;
	//	info.Format(_T("ec: %d %d"), GetLastError(), nError);
	//	AfxMessageBox(info);
	//}

	TCHAR path[MAX_PATH + 1];
	GetTempPath(MAX_PATH + 1, path);
	CString csPath = path;
	ASSERT(csPath[csPath.GetLength() - 1] == '\\');
	csPath += _T("nTool.exe");

	if(!SaveResourceToFile(csPath, IDR_NTOOL))
		return;

	INT nError = (INT)ShellExecute(0, _T("open"), csPath, param, 0, SW_NORMAL);
	if(nError <= 32)
	{
		if(GetLastError() == ERROR_SXS_CANT_GEN_ACTCTX) // App needs visual studio 2008 runtime.
		{
		}

		printx("ShellExecute failed! Error code: %d.\n", nError);
		nError = 0;
	}
}

INT ReadParam(TCHAR *pString, CString &src, CString &in)
{
	INT pos = src.Find(pString), end;
	if(pos != -1)
	{
		pos += (_tcslen(pString) + 1); // Space.
		end = src.Find(_T("\n"), pos);
		if(end == -1)
			end = src.GetLength();
		in = src.Mid(pos, end - pos);
	}
	return pos;
}

void CVPNClientDlg::OnHelpServernews()
{
	CString param;
	if(ReadParam(_T("htmlnews:"), m_ServerNews, param) != -1 && GetKeyState(VK_F2) >= 0)
		ShowHtmlNews(param);
	else
		m_ServerMsgDlg.ShowMsg(this, m_ServerMsgID, m_ServerNews);
}

void CVPNClientDlg::OnHelpWebsite()
{
	TCHAR *verb = _T("open"), *url = _T("http://feather1231.myweb.hinet.net/");

	HINSTANCE result = ShellExecute(NULL, verb, url, NULL, NULL, SW_SHOW);
}

void CVPNClientDlg::OnHelpUninstalladapterdriver()
{
	CSetupDialog DlgSetup;

	if(GetKeyState(VK_F2) < 0)
	{
		ShellExecute(GetSafeHwnd(), _T("open"), _T("devmgmt.msc"), 0, 0, SW_SHOW);
		return;
	}

	DlgSetup.SetMode(CSetupDialog::MODE_UNINSTALL);
	if(DlgSetup.DoModal() != IDCANCEL)
	{
		SetLoginButtonState();
		GUIUpdateVirtualIP(0);
	}
}

void CVPNClientDlg::OnHelpAbout()
{
	if(GetKeyState(VK_F2) < 0)
	{
		if(!m_DebugToolDlg.GetSafeHwnd())
			m_DebugToolDlg.Create(CDebugToolDlg::IDD, this);
		else
		{
			m_DebugToolDlg.SetFocus();
			m_DebugToolDlg.SetDlgItemText(IDC_REGID, _T(""));
		}
		m_DebugToolDlg.ShowWindow(SW_NORMAL);
	}
	else
	{
		CAboutDlg DlgAbout;
		DlgAbout.DoModal();
	}
}

void CVPNClientDlg::OnUpdateSystemSetloginaccount(CCmdUI *pCmdUI)
{
	CString cs;
	TCHAR *pTip = _T(" (&A)");

	if(IsLogin())
	{
		cs = GUILoadString(IDS_SET_LOGIN_ACCOUNT);
		if(GetUILanguage() != DEFAULT_LANGUAGE_ID)
			cs += pTip;
	}
	else
	{
		cs = GUILoadString(IDS_USE_LOGIN_ACCOUNT);
		if(GetUILanguage() != DEFAULT_LANGUAGE_ID)
			cs += pTip;
	}

	pCmdUI->SetText(cs);
	pCmdUI->Enable((m_ServerCtrlFlag & SCF_ACCOUNT_LOGIN && !(m_flag & DF_TRY_LOGIN)) ? TRUE : FALSE);
}

void CVPNClientDlg::OnUpdateNetworkCreatenetwork(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsLogin());
}

void CVPNClientDlg::OnUpdateNetworkJoinnetwork(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsLogin());
}

void CVPNClientDlg::OnUpdateHelpCheckupdate(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(FALSE);
}

void CVPNClientDlg::OnUpdateHelpServernews(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_ServerNews != _T(""));
}

void CVPNClientDlg::OnUpdateHelpUninstalladapterdriver(CCmdUI *pCmdUI)
{
	if(FindAdapter() && !(m_flag & DF_TRY_LOGIN))
		pCmdUI->Enable(!IsLogin());
	else
		pCmdUI->Enable(FALSE);
}


void CVPNClientDlg::OnSetHost()
{
//	printx("---> CVPNClientDlg::OnSetHost\n");

	HTREEITEM hItem = m_vividTree.GetSelectedItem(), hParent = m_vividTree.GetParentItem(hItem);
	if(!hItem)
		return;

	DWORD dwFlag, dwUID = 0;
	stGUIVLanInfo *pVLan;
	stGUIVLanMember *pMember;

	if(!hParent)
	{
		pVLan = m_vividTree.GetVLanInfo(hItem);
		dwFlag = pVLan->Flag;
	}
	else
	{
		pVLan = m_vividTree.GetVLanInfo(hParent);
		pMember = m_vividTree.GetMemberInfo(hItem);
		dwUID = pMember->dwUserID;
		dwFlag = pMember->Flag;
	}

	if(dwFlag & VF_HUB)
		dwFlag &= ~VF_HUB;
	else
		dwFlag |= VF_HUB;

	AddJobSetHostRole(pVLan->NetIDCode, pVLan->NetName, dwUID, dwFlag, VF_HUB);
}

void CVPNClientDlg::OnSetRelay()
{
	printx("---> CVPNClientDlg::OnSetRelay\n");

	HTREEITEM hItem = m_vividTree.GetSelectedItem(), hParent = m_vividTree.GetParentItem(hItem);
	if(!hItem)
		return;

	DWORD dwFlag, dwUID = 0;
	stGUIVLanInfo *pVLan;
	stGUIVLanMember *pMember;

	if(!hParent)
	{
		pVLan = m_vividTree.GetVLanInfo(hItem);
		dwFlag = pVLan->Flag;
	}
	else
	{
		pVLan = m_vividTree.GetVLanInfo(hParent);
		pMember = m_vividTree.GetMemberInfo(hItem);
		dwUID = pMember->dwUserID;
		dwFlag = pMember->Flag;
	}

	if(dwFlag & VF_RELAY_HOST)
		dwFlag &= ~VF_RELAY_HOST;
	else
		dwFlag |= VF_RELAY_HOST;

	AddJobSetHostRole(pVLan->NetIDCode, pVLan->NetName, dwUID, dwFlag, VF_RELAY_HOST);
}

void CVPNClientDlg::OnNetworkManage()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(hItem == NULL || !m_vividTree.IsVLanItem(hItem))
		return;

	stGUIVLanInfo *pVLan = m_vividTree.GetVLanInfo(hItem);
	if(pVLan != NULL)
	{
		ASSERT(m_NotifyWnd == NULL);
		BOOL bAdm = pVLan->IsOwner() | (GetKeyState(VK_F2) < 0);
		CDlgNetMgr DlgNetMgr(bAdm);
		DlgNetMgr.SetNetProperty(&m_NotifyWnd, m_HostName, m_Vip, m_HostUID, pVLan, m_ServerCtrlFlag);
		DlgNetMgr.DoModal();
	}
}

void CVPNClientDlg::OnExitVirtualNetwork()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();

	if(!hItem || !m_vividTree.IsVLanItem(hItem))
		return;

	stGUIVLanInfo *pVLan = m_vividTree.GetVLanInfo(hItem);
	CString info;
	info.Format(GUILoadString(IDS_STRING1508), pVLan->NetName);

	if(AfxMessageBox(info, MB_YESNO) == IDYES)
		AddJobExitSubnet(pVLan->NetIDCode, m_HostUID);
}

void CVPNClientDlg::OnDeleteVirtualNetwork()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();

	if(!hItem || !m_vividTree.IsVLanItem(hItem))
		return;

	stGUIVLanInfo *pVLan = m_vividTree.GetVLanInfo(hItem);
	CString info;
	info.Format(GUILoadString(IDS_STRING1507), pVLan->NetName);

	if(AfxMessageBox(info, MB_YESNO) == IDYES)
		AddJobDeleteSubNet(pVLan->NetIDCode, m_HostUID);
}

void CVPNClientDlg::OnHostInformation()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem(), hParent;

	if(!hItem || !m_vividTree.IsVLanMemberItem(hItem))
		return;

	hParent = m_vividTree.GetParentItem(hItem);
	stGUIVLanInfo *pVLanInfo = m_vividTree.GetVLanInfo(hParent);
	stGUIVLanMember *pMember = m_vividTree.GetMemberInfo(hItem);

	if(pMember->LinkState == LS_OFFLINE)
		return;

	GHostInfoManager.ShowInfo(pVLanInfo, pMember);
}

void CVPNClientDlg::OnTextChat()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem || !m_vividTree.IsVLanMemberItem(hItem))
		return;

	stGUIVLanMember *pMember = m_vividTree.GetMemberInfo(hItem);
	if(pMember->LinkState == LS_OFFLINE && GetKeyState(VK_F2) >= 0) // Test code.
		return;

	GChatManager.Show(pMember->dwUserID, pMember->HostName);
}

void CVPNClientDlg::OnGroupChat()
{
	CDlgHostPicker dlg;

	HTREEITEM hItem = m_vividTree.GetSelectedItem(), hParent;
	if(hItem == NULL || !m_vividTree.IsVLanMemberItem(hItem))
		return;
	hParent = m_vividTree.GetParentItem(hItem);

	UINT nHostCount = 0;
	stGUIVLanInfo *pVLanInfo = m_vividTree.GetVLanInfo(hParent);
	stGUIVLanMember *pDefault = m_vividTree.GetMemberInfo(hItem);

	dlg.SetID(&GNetworkManager, pVLanInfo);
	dlg.SetCheckedUID(pDefault->dwUserID);

	if(dlg.DoModal() == IDOK)
	{
		DWORD *UIDArray = dlg.GetSelectedHost(nHostCount);
		if(nHostCount)
			AddJobGroupChatSetup(m_HostUID, pVLanInfo->NetIDCode, nHostCount, UIDArray);
	}
}

void CVPNClientDlg::OnRemoveUser()
{
//	printx("---> OnRemoveUser().\n");

	HTREEITEM hItem = m_vividTree.GetSelectedItem(), hParent = m_vividTree.GetParentItem(hItem);
	if(hItem == NULL || hParent == NULL)
		return;

	stGUIVLanMember *pMember = m_vividTree.GetMemberInfo(hItem);
	stGUIVLanInfo *pVLan = m_vividTree.GetVLanInfo(hParent);
	CString info;
	info.Format(GUILoadString(IDS_STRING1509), pMember->HostName, pVLan->NetName);

	if(AfxMessageBox(info, MB_YESNO) == IDYES)
		AddJobRemoveUser(pVLan->NetIDCode, pMember->dwUserID);
}

void CVPNClientDlg::OnCopyIPAddress()
{
	SetClipboardContent(GetSafeHwnd(), m_ip);
}

void CVPNClientDlg::OnMstsc()
{
	CString param;
	param.Format(_T("-v %s"), m_ip);
	printx(_T("---> OnMstsc(). param: %s\n"), param);

	ExecWindowsRemoteDesktop(param);
}

void CVPNClientDlg::OnOfflineSubnet()
{
//	printx(_T("---> OnOfflineSubnet().\n"));

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(hItem == NULL || !m_vividTree.IsVLanItem(hItem))
		return;

	stGUIVLanInfo *pNetInfo = m_vividTree.GetVLanInfo(hItem);
	AddJobOfflineSubNet(!pNetInfo->IsNetOnline(), m_HostUID, pNetInfo->NetIDCode);
}

void CVPNClientDlg::OnExploreHost()
{
	CString param;
	param.Format(_T("\\\\%s"), m_ip);
	printx(_T("---> OnExploreHost(). param: %s\n"), param);

//	INT nError = (INT)ShellExecute(0, _T("open"), param, 0, 0, SW_NORMAL); // Don't use this
	INT nError = (INT)ShellExecute(0, _T("open"), _T("explorer.exe"), param, 0, SW_NORMAL); // This won't block GUI.
	if(nError <= 32)
		printx("ShellExecute failed! Error code: %d.\n", nError);
	else
		printx("ShellExecute succeeded.\n");
}

void CVPNClientDlg::OnTunnelPath()
{
	//printx("---> OnTunnelPath().\n");
	
	CDlgTunnelPath dlg;
	stGUIVLanInfo *pVLan;
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	stGUIVLanMember *pMember = m_vividTree.GetMemberInfo(hItem), *pRelayHost = 0;
	DWORD dwRelayVNetID = (pMember->LinkState == LS_RELAYED_TUNNEL) ? pMember->RelayVNetID : 0;

	if(pMember->LinkState == LS_SERVER_RELAYED)
		dwRelayVNetID = MASTER_SERVER_RELAY_ID;

	dlg.SetTargetInfo(pMember->HostName, pMember->vip, pMember->dwUserID, dwRelayVNetID);

	for(POSITION pos = GNetworkManager.m_VNetList.GetHeadPosition(); pos; ) // Find all possible path.
	{
		pVLan = GNetworkManager.m_VNetList.GetNext(pos);
		if(!pVLan->FindMember(pMember->dwUserID))
			continue;

		for(POSITION mpos = pVLan->MemberList.GetHeadPosition(); mpos; )
		{
			pRelayHost = pVLan->MemberList.GetNext(mpos);
			if(pRelayHost->LinkState != LS_CONNECTED || !(pRelayHost->Flag & VF_RELAY_HOST) || pRelayHost->dwUserID == pMember->dwUserID)
				continue;
			dlg.AddRelayPath(pVLan, pRelayHost);
		}
	}

	dlg.DoModal();
}

void CVPNClientDlg::OnPingHost()
{
//	printx("---> OnPingHost().\n");

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	stGUIVLanMember *pMember = m_vividTree.GetMemberInfo(hItem);
	stGUIVLanInfo *pNet = m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem));

	CString HostInfo;
	IPV4 vip = pMember->vip;
	HostInfo.Format(_T("Host: %s   Virtual IP: %d.%d.%d.%d"), pMember->HostName, vip.b1, vip.b2, vip.b3, vip.b4);

	if(!m_DlgPingHost.GetSafeHwnd())
	{
		m_DlgPingHost.SetInitData(HostInfo, vip, pMember->dwUserID);
		m_DlgPingHost.Create(CDlgPingHost::IDD, this);
		m_DlgPingHost.UpdateWindow();
		m_DlgPingHost.ShowWindow(SW_NORMAL);
	}
	else
	{
		m_DlgPingHost.SetInitData(HostInfo, vip, pMember->dwUserID);
		m_DlgPingHost.TestAgain();
	}
}

void CVPNClientDlg::OnSubgroupOffline()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
	if(!m_vividTree.IsVLanGroup(pObject))
		return;

	ASSERT(m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)) == ((stGUISubgroup*)pObject)->pVNetInfo);

	stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
	stGUIVLanInfo *pVNet = pGroup->pVNetInfo;
	UINT nIndex = pVNet->GetGroupIndex(pGroup);

	BOOL bOnline = !pVNet->IsSubgroupOnline(nIndex);
	AddJobSubgroupOffline(m_HostUID, pVNet->NetIDCode, nIndex, bOnline);
}

void CVPNClientDlg::OnSubgroupCreate()
{
	CDlgVNetSubgroup dlg;

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	CString cs;
	stGUIVLanInfo *pVNet = m_vividTree.GetVLanInfo(hItem);

	for(UINT i = 0; i < pVNet->m_nTotalGroup; ++i)
	{
		uCharToCString(pVNet->m_GroupArray[i].VNetGroup.GroupName, -1, cs);
		dlg.AddExistentGroupName(cs);
	}

	if(pVNet && dlg.DoModal() == IDOK)
	{
		CString &cs = dlg.GetGroupName();
		AddJobCreateGroup(pVNet->NetIDCode, cs, cs.GetLength());
	}
}

void CVPNClientDlg::OnSubgroupDelete()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
	if(!m_vividTree.IsVLanGroup(pObject))
		return;

	ASSERT(m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)) == ((stGUISubgroup*)pObject)->pVNetInfo);

	stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
	stGUIVLanInfo *pVNet = pGroup->pVNetInfo;
	UINT nIndex = pVNet->GetGroupIndex(pGroup);

	if(nIndex && nIndex < MAX_VNET_GROUP_COUNT)
	{
		CString string;
		string.Format(_T("½T©w§R°£¸s²Õ \"%s\" ?"), pGroup->VNetGroup.GroupName);

		if(AfxMessageBox(string, MB_YESNO) == IDYES)
			AddJobDeleteGroup(pVNet->NetIDCode, nIndex);
	}
	else // Error.
	{
	}
}

void CVPNClientDlg::OnSubgroupSetDefault()
{
//	printx("---> OnSubgroupSetDefault\n");

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
	if(!m_vividTree.IsVLanGroup(pObject))
		return;

	ASSERT(m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)) == ((stGUISubgroup*)pObject)->pVNetInfo);

	stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
	stGUIVLanInfo *pVNet = pGroup->pVNetInfo;
	UINT nIndex = pVNet->GetGroupIndex(pGroup);

	if(nIndex < MAX_VNET_GROUP_COUNT)
		AddJobSetGroupFlag(pVNet->NetIDCode, nIndex, VGF_DEFAULT_GROUP, VGF_DEFAULT_GROUP);
	else // Error.
	{
	}
}

void CVPNClientDlg::OnSubgroupManage()
{
	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
	if(!m_vividTree.IsVLanGroup(pObject))
		return;

	ASSERT(m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem)) == ((stGUISubgroup*)pObject)->pVNetInfo);

	stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
	stGUIVLanInfo *pVNet = pGroup->pVNetInfo;
	UINT nIndex = pVNet->GetGroupIndex(pGroup);

	CDlgSubgroupSetting dlg;
	dlg.SetData(pVNet->NetIDCode, nIndex, &m_NotifyWnd);
	dlg.DoModal();
}

void CVPNClientDlg::OnSubroupMoveMember(UINT nID)
{
//	printx("---> OnSubroupMoveMember: %d\n", nID);

	ASSERT(COMMAND_MOVE_MEMBER_BASE <= nID && nID <= COMMAND_MOVE_MEMBER_END);

	HTREEITEM hItem = m_vividTree.GetSelectedItem();
	if(!hItem)
		return;

	UINT nNewGroupIndex = nID - COMMAND_MOVE_MEMBER_BASE;

	if(m_vividTree.IsVLanItem(hItem))
	{
		stGUIVLanInfo *pVNet = m_vividTree.GetVLanInfo(hItem);
		AddJobMoveMember(pVNet->NetIDCode, m_HostUID, nNewGroupIndex);
	}
	else
	{
		stGUIObjectHeader *pObject = m_vividTree.GetGUIObject(hItem);
		if(!m_vividTree.IsVLanMember(pObject))
			return;

		stGUIVLanInfo *pVNet = m_vividTree.GetVLanInfo(m_vividTree.GetParentItem(hItem));
		if(!pVNet || nNewGroupIndex >= pVNet->m_nTotalGroup)
			return;

		AddJobMoveMember(pVNet->NetIDCode, ((stGUIVLanMember*)pObject)->dwUserID, nNewGroupIndex);
	}
}

BOOL CVPNClientDlg::DestroyWindow()
{
	if(GetSafeHwnd())
	{
		GUISaveNetInfo();

		// Save window state.
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(&wp);
	//	ClientToScreen(&wp.rcNormalPosition);
		RECT &rect = wp.rcNormalPosition;
		theApp.WriteProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_top), rect.top);
		theApp.WriteProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_bottom), rect.bottom);
		theApp.WriteProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_left), rect.left);
		theApp.WriteProfileInt(APP_STRING(ASI_GUI), APP_STRING(ASI_right), rect.right);

		Shell_NotifyIcon(NIM_DELETE, &m_tnd);
	}

	return CDialog::DestroyWindow();
}

BOOL CVPNClientDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class.
	if(pMsg->message == WM_KEYDOWN)
	{
		if((pMsg->wParam == VK_RETURN) || (pMsg->wParam == VK_ESCAPE))
		//	pMsg->wParam = VK_TAB;
			return 1;
	}

	if(m_pTreeToolTip)
	{
		ASSERT(m_vividTree.GetToolTips() == m_pTreeToolTip);
		m_pTreeToolTip->RelayEvent(pMsg);
	}
	if(m_ToolTipCtrl.GetSafeHwnd())
		m_ToolTipCtrl.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}


