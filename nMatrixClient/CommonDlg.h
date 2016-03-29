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


//#include "afxdhtml.h"
#include "CnMatrixCore.h"
#include "SetupDialog.h"


#define TIMER_WAIT_ID 2
#define WM_SERVER_RESPONSE_EVENT (WM_USER + 1)
#define WM_VNET_MEMBER_INFO      (WM_USER + 2)
#define WM_RELAY_HOST_CHANGED    (WM_USER + 3)
#define WM_MEMBER_REMOVED_EVENT  (WM_USER + 4)
#define WM_PING_HOST_RESPONSE    (WM_USER + 5)
#define WM_PING_HOST_RESET       (WM_USER + 6)
#define WM_UPDATE_GROUP_NAME     (WM_USER + 7)


#define NO_D3D_ABOUT


#ifndef NO_D3D_ABOUT
	#include <d3d9.h>
	#include <d3dx9.h>
	#pragma comment(lib, "d3d9.lib")
	#pragma comment(lib, "d3dx9.lib")
#endif


class CAboutDlg : public CDialog
{
public:

	enum { IDD = IDD_ABOUTBOX, TIMER_ID = 1, TIMER_PERIOD = 32 };

	CAboutDlg()
	:CDialog(IDD)
	{
#ifndef NO_D3D_ABOUT
		m_pD3D = NULL;
		m_pD3DDevice = NULL;
		m_pVertexBuffer = NULL;
		m_pTex = NULL;
#endif
	}
	~CAboutDlg()
	{
	}

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	BOOL InitD3d(HWND hWnd, INT width, INT height);
	void ReleaseD3d();

	struct stVertex
	{
	//	stVertex():diff(0) {}
		enum { D3D_FVF = D3DFVF_XYZ/* | D3DFVF_DIFFUSE*/ | D3DFVF_TEX1};

		FLOAT x, y, z;
	//	D3DCOLOR diff;
		FLOAT tu, tv;
	};


protected:


#ifndef NO_D3D_ABOUT
	LPDIRECT3D9 m_pD3D;
	LPDIRECT3DDEVICE9 m_pD3DDevice;
	LPDIRECT3DVERTEXBUFFER9 m_pVertexBuffer;
	LPDIRECT3DTEXTURE9 m_pTex;

	DWORD m_dwStartTime;
	stVertex m_Vertex[24];
#endif

	DECLARE_MESSAGE_MAP()

	virtual void OnCancel();


};


class CIDLoginDlg : public CDialog
{
public:

	enum
	{
		IDD = IDD_ID_LOGIN
	};

	CIDLoginDlg()
	:CDialog(CIDLoginDlg::IDD), m_mode(0), m_result(0), m_pSerCtrlFlag(0), m_pNotifyHandle(0)
	{
	}
	virtual ~CIDLoginDlg() { *m_pNotifyHandle = 0; }

	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();

	void SetMode(HWND *pHwnd, BOOL bLogin) { m_mode = bLogin ? 1 : 0; m_pNotifyHandle = pHwnd; }
	void SetString(CString &ln, CString &lp, DWORD *pSerFlag) { m_LoginName = ln; m_Password = lp; m_pSerCtrlFlag = pSerFlag; }

	CString GetLgName() { return m_LoginName; }
	CString GetLgPassword() { return m_Password; }


protected:

	DWORD m_mode, m_result, *m_pSerCtrlFlag;
	HWND  *m_pNotifyHandle;
	CString m_LoginName, m_Password;

	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnServerReply(WPARAM wParam, LPARAM lParam);


};


class CDlgCreateSubnet : public CDialog
{
public:

	enum
	{
		IDD = IDD_CREATE_SUBNET,
		TIME_OUT = 6000
	};

	CDlgCreateSubnet(CWnd* pParent = NULL);
	virtual ~CDlgCreateSubnet();

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();

	void SetPointer(DWORD LocalUID, HWND *pHwnd) { m_LocalUID = LocalUID; m_pNotifyHandle = pHwnd; }


protected:

	DWORD m_LocalUID;
	HWND  *m_pNotifyHandle;

	afx_msg LRESULT OnServerReply(WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()


};


class CDlgJoinNet : public CDialog
{
public:

	enum
	{
		IDD = IDD_JOIN_SUBNET,
		TIME_OUT = 6000
	};

	CDlgJoinNet(CWnd *pParent = NULL);
	virtual ~CDlgJoinNet();

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedJoin();

	void SetPointer(DWORD LocalUID, HWND *pHwnd) { m_LocalUID = LocalUID; m_pNotifyHandle = pHwnd; }


protected:

	DWORD m_LocalUID;
	HWND *m_pNotifyHandle;

	afx_msg LRESULT OnServerReply(WPARAM wParam, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()


};


class CDlgResetNetwork : public CDialog
{
public:

	enum { IDD = IDD_RESET_NETWORK };

	CDlgResetNetwork()
	:CDialog(CDlgResetNetwork::IDD), m_mode(0), m_result(0), m_SerCtrlFlag(0)
	{
	}
	virtual ~CDlgResetNetwork()
	{
	}


	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedOk();

	void SetString(CString nn, CString np, DWORD SerFlag) { m_NetworkName = nn; m_Password = np; m_SerCtrlFlag = SerFlag; }
	CString GetNetworkName() { return m_NetworkName; }
	CString GetNetworkPassword() { return m_Password; }


protected:

	DWORD m_mode, m_result, m_SerCtrlFlag;
	CString m_NetworkName, m_Password;
	CEdit   m_Edit;

	DECLARE_MESSAGE_MAP()


};


class CDlgSubgroupSetting : public CDialog
{
public:

	enum { IDD = IDD_GROUP_MANAGE };


	CDlgSubgroupSetting(CWnd *pParent = NULL)
	:CDialog(CDlgSubgroupSetting::IDD, pParent), m_nGroupIndex(0), m_VNetID(0), m_pNotifyHandle(0)
	{
	}
	virtual ~CDlgSubgroupSetting()
	{
		*m_pNotifyHandle = 0;
	}


	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedRename();
	afx_msg void OnEnChangeGroupName();
	afx_msg void OnBnClickedOk();

	void SetData(DWORD VNetID, UINT nGroupIndex, HWND *pNotifyHandle) { m_VNetID = VNetID; m_nGroupIndex = nGroupIndex; m_pNotifyHandle = pNotifyHandle; }
	void UpdateTitleName(const TCHAR *pGroupName);
	stGUISubgroup* GetGroupObject();


protected:

	UINT  m_nGroupIndex;
	DWORD m_VNetID;
	HWND  *m_pNotifyHandle;
	CEdit *m_pEdit;
	BOOL m_bDefaultOffline;

	DECLARE_MESSAGE_MAP()

	afx_msg LRESULT OnUpdateGroupName(WPARAM wParam, LPARAM lParam);


};


class CDlgNetMgr : public CDialog
{
public:

	enum
	{
		IDD1 = IDD_NETWORK_MANAGE,
		IDD2 = IDD_NETWORK_OPTION,

		ADM_MODE_NULL = 0,
		ADM_MODE_MEMBER_INFO = 1,
		ADM_MODE_RELAY_HOST  = 2,

		DEFAULT_WIDTH = 426,
		DEFAULT_HEIGHT = 364,

		ADM_MODE_MIN_WIDTH = 426,
		ADM_MODE_MIN_HEIGHT = DEFAULT_HEIGHT + 150,
	};


	CDlgNetMgr(BOOL bAdm, CWnd *pParent = NULL)
	:CDialog(bAdm ? CDlgNetMgr::IDD1 : CDlgNetMgr::IDD2, pParent), m_pNotifyHandle(NULL)
	{
		m_bIsADM = (bAdm != 0);
		m_AdmMode = ADM_MODE_NULL;
	}
	virtual ~CDlgNetMgr()
	{
		*m_pNotifyHandle = NULL;
	}


	virtual BOOL OnInitDialog();

	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnBnClickedResetPassword();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedQueryMembers();
	afx_msg void OnBnClickedAct();
	afx_msg void OnCbnSelchangeTypeCombo();
	afx_msg void OnLvnItemchangedInfoList(NMHDR *pNMHDR, LRESULT *pResult);

	void SetupGUIForAdmMode();
	void SetupRelayHostList(CListCtrl *pListCtrl);
	void UpdateControl(CWnd *pWnd, INT VerticalOffset, BOOL bShow = TRUE);
	void UpdateControlSize(CWnd *pWnd, INT iNewHeight, INT iNewWidth);

	void SetNetProperty(HWND *phWnd, CString LocalName, DWORD LocalVIP, DWORD LocalUID, stGUIVLanInfo *pVLan, DWORD dwSerCtrlFlag)
	{
		m_pNotifyHandle = phWnd;

		m_LocalName = LocalName;
		m_LocalVIP = LocalVIP;
		m_LocalUID = LocalUID;

		m_dwSerCtrlFlag = dwSerCtrlFlag;
		m_NetName = pVLan->NetName;
		m_VNetID = pVLan->NetIDCode;

		m_bNeedPassword = (pVLan->Flag & VF_NEED_PASSWORD) ? true : false;
		m_bDisallowUser = (pVLan->Flag & VF_DISALLOW_USER) ? true : false;
		m_bAllowRelay = (pVLan->Flag & VF_ALLOW_RELAY) ? true : false;
	}


protected:

	CRect   m_Rect;
	CString m_NetName, m_LocalName, m_csRelayOn, m_csRelayOff, m_csSetOn, m_csSetOff;
	DWORD m_LocalVIP, m_LocalUID, m_VNetID, m_dwSerCtrlFlag, m_AdmMode;
	HWND  *m_pNotifyHandle;
	CButton *m_pActButton, *m_pOkButton;
	CListCtrl *m_pListCtrl;

	bool  m_bIsADM, m_bNeedPassword, m_bDisallowUser, m_bAllowRelay;

//	afx_msg LRESULT OnServerReply(WPARAM wParam, LPARAM lParam); // Not used.
	afx_msg LRESULT OnVNetMemberInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRelayHostChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnMemberRemovedEvent(WPARAM wParam, LPARAM lParam);
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange *pDX);
	DECLARE_MESSAGE_MAP()


};


class CEditSP: public CEdit
{
public:

	void SetBkColor(COLORREF cr)
	{
		m_cr = cr;
		if((HBRUSH)m_br)
			m_br.DeleteObject();
		m_br.CreateSolidBrush(cr);
		Invalidate();
	}


protected:

	COLORREF m_cr;
	CBrush   m_br;

	DECLARE_MESSAGE_MAP()

	HBRUSH CtlColor(CDC* pDC, UINT nCtlColor)
	{
		pDC->SetBkColor(m_cr);
		return m_br;
	}


};


class CDlgServerMsg : public CDialog
{
public:

	enum{ IDD = IDD_SERVER_MSG };

	CDlgServerMsg()
	:CDialog(CDlgServerMsg::IDD), m_MsgID(0)
	{
	}
	virtual ~CDlgServerMsg()
	{
	}

	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	virtual void OnCancel();

	void UpdatePos(CWnd *pParentWnd);
	void ShowMsg(CWnd *pParentWnd, DWORD MsgID, const TCHAR *pString);
	void SetPointer(DWORD *pClosedID) { m_pClosedID = pClosedID; }


protected:

	CEditSP m_Edit;
	DWORD m_MsgID, *m_pClosedID;

	DECLARE_MESSAGE_MAP()


};


/*
class CDlgHtmlNews : public CDHtmlDialog
{
public:

	CDlgHtmlNews(CWnd *pParent = NULL);

	enum { IDD = IDD_SERVER_MSG };

	virtual BOOL OnInitDialog();
	afx_msg void OnClose();


protected:

	DECLARE_MESSAGE_MAP()
	DECLARE_DHTML_EVENT_MAP()


};
*/


class CDlgTunnelPath : public CDialog
{
public:

	enum { IDD = IDD_TUNNEL_PATH, MAX_PATH_COUNT = 20, };

	CDlgTunnelPath()
	:CDialog(CDlgTunnelPath::IDD), m_TotalPath(0), m_bHasServerRelay(0), m_bServerRelayRight(0)
	{
	}
	virtual ~CDlgTunnelPath() { /**m_pNotifyHandle = 0;*/ }

	struct stPathNode
	{
		CString RelayHostName, NetworkGroup;
		DWORD RHUID, NetID, vip;
	};


	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);

	void EnableUseButton(BOOL bEnable) { GetDlgItem(IDOK)->EnableWindow(bEnable); }
	void SetTargetInfo(CString HostName, DWORD vip, DWORD DestUID, DWORD RHNetID) { m_HostName = HostName; m_vip = vip; m_DestUID = DestUID; m_RHNetID = RHNetID; }
	BOOL AddRelayPath(stGUIVLanInfo *pVNet, stGUIVLanMember *pRH);


protected:

	UINT m_TotalPath;
	BOOL m_bHasServerRelay, m_bServerRelayRight;
	stPathNode m_PathInfo[MAX_PATH_COUNT];
	DWORD_PTR m_RHNetID, m_DestUID;
	CString m_HostName;
	IPV4 m_vip;

	DECLARE_MESSAGE_MAP()


};


class CDlgPingHost : public CDialog
{
public:

	enum
	{
		// Timer state.
		TS_START_PING,
		TS_WAIT_RESPONSE,
		TS_PING_AGAIN,
		TS_PH_OVER,

		TIMER_ID = 1,
		MAX_PH_COUNT = 10,
		PH_TIME_OUT = 5000,

		IDD = IDD_PING_HOST
	};


	CDlgPingHost()
	:CDialog(CDlgPingHost::IDD), m_SendCount(0), m_vip(0), m_uid(0), m_state(TS_START_PING)
	{
	}
	virtual ~CDlgPingHost() {}

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	void ShowEndString();
	void PingOnce();
	void TestAgain();
	void AppendText(const TCHAR *pString = 0);

	inline void SetInitData(CString HostInfo, DWORD VipIn, DWORD UidIn)
	{
		m_HostInfo = HostInfo;
		m_vip = VipIn;
		m_uid = UidIn;
		m_SendCount = 0;
		m_state = TS_START_PING;
	}


protected:

	DECLARE_MESSAGE_MAP()

	CString m_HostInfo;
	UINT    m_SendCount;
	DWORD   m_vip, m_uid, m_state;
	CEditSP m_Edit;

	virtual void OnCancel();
	afx_msg LRESULT OnPingHostResponse(WPARAM wParam, LPARAM lParam);


};


class CDlgHostPicker : public CDialog
{
public:

	enum
	{
		IDD = IDD_HOST_PICKER,
		GUI_MAX_HOST_COUNT = 512,
	};

	CDlgHostPicker()
	:CDialog(CDlgHostPicker::IDD), m_pVNet(NULL), m_HostCount(0), m_CheckedUID(0)
	{
	}
	virtual ~CDlgHostPicker()
	{
	}

	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedSelectAll();
	afx_msg void OnBnClickedRemoveAll();
	afx_msg void OnNMClickTree(NMHDR *pNMHDR, LRESULT *pResult);


	void SetID(void *pNetworkManager, stGUIVLanInfo *pVNet) { m_pNetworkManager = pNetworkManager; m_pVNet = pVNet;}
	void SetCheckedUID(DWORD UID) { m_CheckedUID = UID; }
	DWORD* GetSelectedHost(UINT &nHostCount) { nHostCount = m_HostCount; return m_UIDArray; }


protected:

	void  *m_pNetworkManager;
	stGUIVLanInfo *m_pVNet;
	DWORD m_CheckedUID;
	UINT  m_HostCount;
	DWORD m_UIDArray[GUI_MAX_HOST_COUNT];


	DECLARE_MESSAGE_MAP()


};


class CDlgVNetSubgroup : public CDialog
{
public:

	enum { IDD = IDD_VNET_GROUP };

	CDlgVNetSubgroup()
	:CDialog(CDlgVNetSubgroup::IDD), m_TotalName(0)
	{
	}
	virtual ~CDlgVNetSubgroup()
	{
	}


	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeGroupName();
	afx_msg void OnBnClickedOk();

	CString& GetGroupName() { return m_csName; }
	void AddExistentGroupName(CString csName) { m_NameArray[m_TotalName++] = csName; }


protected:

	CString  m_csName;
	BOOL     m_bButtonEnabled;
	UINT     m_TotalName;
	CString  m_NameArray[MAX_VNET_GROUP_COUNT];


	DECLARE_MESSAGE_MAP()

	virtual void DoDataExchange(CDataExchange* pDX);


};


