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
#include "CommonDlg.h"
#include "VividTreeEx.h"


extern CGUINetworkManager GNetworkManager;


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(GUILoadString(IDS_ABOUT));
	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));

	INT ver, sub, buildnum;
	CString version, format = GUILoadString(IDS_VERSION_FORMAT);

	AppGetVersionDetail(AppGetVersion(), ver, sub, buildnum);
	version.Format(format, ver, sub, buildnum);
	if (AppGetVersionFlag() & stVersionInfo::VF_BETA)
		version += _T(" (Beta)");
	SetDlgItemText(IDC_VERSION, version);

	CRect rect;
	CWnd *pWnd = GetDlgItem(IDC_D3DVIEW);
	pWnd->GetWindowRect(rect);

#ifndef NO_D3D_ABOUT
	InitD3d(pWnd->GetSafeHwnd(), rect.Width(), rect.Height());
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnCancel()
{
	KillTimer(TIMER_ID);
	ReleaseD3d();
	CDialog::OnCancel();
}

void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
#ifndef NO_D3D_ABOUT

	if (m_pD3DDevice == NULL)
		return;

	DWORD ctime = GetTickCount();
	FLOAT tick = (FLOAT)(ctime - m_dwStartTime) / 1000;

	D3DXMATRIX matR;
	D3DXMatrixRotationY(&matR, 3.1415926f / 10 * tick);
//	D3DXMatrixMultiply((D3DXMATRIX*)&m_mat, (D3DXMATRIX*)&m_mat, (D3DXMATRIX*)&matR);
	m_pD3DDevice->SetTransform(D3DTS_WORLD, &matR);

	m_pD3DDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB(0, 255, 255, 255), 1.0f, 0);

	m_pD3DDevice->BeginScene();

	if (m_pVertexBuffer == NULL)
		m_pD3DDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 0, 2);
	else
		m_pD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_Vertex, sizeof(stVertex));

	m_pD3DDevice->EndScene();

	m_pD3DDevice->Present(0, 0, 0, 0);

#endif

//	CDialog::OnTimer(nIDEvent);
}

BOOL CAboutDlg::InitD3d(HWND hWnd, INT width, INT height)
{
#ifndef NO_D3D_ABOUT

	if((m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return FALSE;

	D3DDISPLAYMODE d3ddm;
	m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.Windowed = TRUE;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_4_SAMPLES;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferWidth = width; // d3ddm.Width;
	d3dpp.BackBufferHeight = height; // d3ddm.Height;
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.Flags = 0;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // D3DPRESENT_DONOTWAIT;

	if(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice) != D3D_OK)
	{
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
		if(m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &m_pD3DDevice) != D3D_OK)
		{
			SAFE_RELEASE(m_pD3D);
			return FALSE;
		}
	}

	// Init vertex.
	const INT iHalfSize = 256;

	m_Vertex[0].x = -iHalfSize;
	m_Vertex[0].y = iHalfSize;
	m_Vertex[0].z = -iHalfSize;
	m_Vertex[0].tu = 0.0f;
	m_Vertex[0].tv = 0.0f;

	m_Vertex[1].x = iHalfSize;
	m_Vertex[1].y = iHalfSize;
	m_Vertex[1].z = -iHalfSize;
	m_Vertex[1].tu = 1.0f;
	m_Vertex[1].tv = 0.0f;

	m_Vertex[2].x = iHalfSize;
	m_Vertex[2].y = -iHalfSize;
	m_Vertex[2].z = -iHalfSize;
	m_Vertex[2].tu = 1.0f;
	m_Vertex[2].tv = 1.0f;

	m_Vertex[3].x = -iHalfSize;
	m_Vertex[3].y = -iHalfSize;
	m_Vertex[3].z = -iHalfSize;
	m_Vertex[3].tu = 0.0f;
	m_Vertex[3].tv = 1.0f;

	D3DXMATRIX mat;
	D3DXMatrixLookAtLH(&mat, &D3DXVECTOR3(0, 0, -550.0f), &D3DXVECTOR3(0, 0, 0), &D3DXVECTOR3(0, 1.0f, 0));
	m_pD3DDevice->SetTransform(D3DTS_VIEW, &mat);

	D3DXMatrixPerspectiveFovLH(&mat, 3.1415926f / 2, (FLOAT)width / height, 10.0f, 1000.0f);
	m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &mat);

	//if(m_pD3DDevice->CreateVertexBuffer(sizeof(m_Vertex), D3DUSAGE_WRITEONLY, stVertex::D3D_FVF, D3DPOOL_DEFAULT, &m_pVertexBuffer, 0) == D3D_OK)
	//{
	//	stVertex *pData;
	//	if(m_pVertexBuffer->Lock(0, sizeof(m_Vertex), (void**)&pData, D3DLOCK_DISCARD) == D3D_OK)
	//	{
	//		memcpy(pData, m_Vertex, sizeof(m_Vertex));
	//		m_pVertexBuffer->Unlock();
	//		m_pD3DDevice->SetStreamSource(0, m_pVertexBuffer, 0, sizeof(stVertex));
	//	}
	//}
	//else
	//	printx("Failed to create vertex buffer!\n");
	if(D3DXCreateTextureFromResource(m_pD3DDevice, 0, MAKEINTRESOURCE(IDB_EVA), &m_pTex) != D3D_OK)
	{
	//	printx("Failed to create texture!\n");
	}
//	D3DXCreateTextureFromFile(m_pD3DDevice, _T("E:\\VPN Project\\VPN Client\\VPN Client\\res\\eva.bmp"), &m_pTex);

	m_pD3DDevice->SetFVF(stVertex::	D3D_FVF);
	m_pD3DDevice->SetTexture(0, m_pTex);

	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);

	m_pD3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pD3DDevice->SetRenderState(D3DRS_LIGHTING, 0);

	SetTimer(TIMER_ID, TIMER_PERIOD, 0);
	m_dwStartTime = GetTickCount();

#endif

	return TRUE;
}

void CAboutDlg::ReleaseD3d()
{
#ifndef NO_D3D_ABOUT
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pTex);
	SAFE_RELEASE(m_pD3DDevice);
	SAFE_RELEASE(m_pD3D);
#endif
}


BEGIN_MESSAGE_MAP(CIDLoginDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CIDLoginDlg::OnBnClickedOk)
	ON_MESSAGE(WM_SERVER_RESPONSE_EVENT, &CIDLoginDlg::OnServerReply)
END_MESSAGE_MAP()


BOOL CIDLoginDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString cs;

	if(m_mode == 1)
	{
		cs = GUILoadString(IDS_SET_LOGIN_ACCOUNT);
		cs.Remove('&');

		if(m_LoginName != _T(""))
		{
			CWnd *pTemp;
			pTemp = GetDlgItem(IDC_LOGIN_NAME);
			pTemp->SetWindowText(m_LoginName);
			pTemp->EnableWindow(FALSE);
			pTemp = GetDlgItem(IDC_LOGIN_PASSWORD);
			pTemp->SetWindowText(m_Password);
			pTemp->EnableWindow(FALSE);
			GetDlgItem(IDOK)->EnableWindow(FALSE);
		}
		GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));
	}
	else
	{
		GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_LOGIN));

		cs = GUILoadString(IDS_USE_LOGIN_ACCOUNT);
		cs.Remove('&');
	}
	SetWindowText(cs);

	cs = GUILoadString(IDS_ACCOUNT) + csGColon;
	GetDlgItem(IDC_ACCOUNT)->SetWindowText(cs);
	cs = GUILoadString(IDS_PASSWORD) + csGColon;
	GetDlgItem(IDC_PASSWORD)->SetWindowText(cs);

	cs = GUILoadString(IDS_CANCEL);
	GetDlgItem(IDCANCEL)->SetWindowText(cs);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CIDLoginDlg::OnBnClickedOk()
{
	UINT length;
	CString LoginName, Password, Msg;

	GetDlgItemText(IDC_LOGIN_NAME, LoginName);
	length = LoginName.GetLength();
	if(!length || length > LOGIN_ID_LEN)
	{
		if(!length)
			Msg.Format(_T("Name can't be null."));
		else
			Msg.Format(_T("Max name length is %d."), LOGIN_ID_LEN);
		AfxMessageBox(Msg, MB_ICONINFORMATION);
		GetDlgItem(IDC_LOGIN_NAME)->SetFocus();
		return;
	}

	GetDlgItemText(IDC_LOGIN_PASSWORD, Password);
	length = Password.GetLength();
	if(!length || length > LOGIN_PASSWORD_LEN)
	{
		if(!length)
			Msg.Format(GUILoadString(IDS_INPUT_PASSWORD));
		else
			Msg.Format(GUILoadString(IDS_MAX_PASSWORD_LEN), LOGIN_PASSWORD_LEN);
		AfxMessageBox(Msg, MB_ICONINFORMATION);
		GetDlgItem(IDC_LOGIN_PASSWORD)->SetFocus();
		return;
	}

	*m_pNotifyHandle = GetSafeHwnd();

	if(m_mode == 1)
	{
		m_LoginName = LoginName;
		m_Password = Password;
		AddJobRegister(LoginName.GetBuffer(), Password.GetBuffer());
	}
	else
	{
		AddJobLogin(0, LoginName.GetBuffer(), Password.GetBuffer(), FALSE);
	}

	GetDlgItem(IDOK)->EnableWindow(FALSE);
}

LRESULT CIDLoginDlg::OnServerReply(WPARAM wParam, LPARAM lParam)
{
	TCHAR *pError1 = _T("Name or password is incorrect.\n");
	TCHAR *pError2 = _T("Not enabled by the server!");

	if(m_mode == 1)
	{
		if(wParam == 1)
			AfxMessageBox(_T("Login account created successfully."));
		else
			AfxMessageBox(_T("Failed to create login account!"));
	}
	else
	{
		switch(wParam)
		{
			case LRC_LOGIN_SUCCESS:
				break;

			case LRC_NAME_NOT_FOUND:
				printx("Login name not found!\n");
				AfxMessageBox(pError1);
				break;

			case LRC_PASSWORD_ERROR:
				printx("Login password error!\n");
				AfxMessageBox(pError1);
				break;

			case LRC_SERVER_REJECTED:
				*m_pSerCtrlFlag &= ~SCF_ACCOUNT_LOGIN;
				AfxMessageBox(pError2);
				break;

			case LRC_CONNECT_FAILED:
			default:
	//			AfxMessageBox(_T("Login failed!"));
				;
		}
	}

	EndDialog(wParam);

	return 0;
}


CDlgCreateSubnet::CDlgCreateSubnet(CWnd* pParent /*=NULL*/)
:CDialog(CDlgCreateSubnet::IDD, pParent), m_pNotifyHandle(0)
{
}

CDlgCreateSubnet::~CDlgCreateSubnet()
{
	*m_pNotifyHandle = 0;
}

void CDlgCreateSubnet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgCreateSubnet, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgCreateSubnet::OnBnClickedOk)
	ON_MESSAGE(WM_SERVER_RESPONSE_EVENT, &CDlgCreateSubnet::OnServerReply)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CDlgCreateSubnet::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(GUILoadString(IDS_CREATE_NETWORK));

	GetDlgItem(IDC_NETWORK_ID)->SetWindowText(GUILoadString(IDS_NETWORK_ID) + csGColon);
	GetDlgItem(IDC_NETWORK_PASSWORD)->SetWindowText(GUILoadString(IDS_NETWORK_PASSWORD) + csGColon);
	GetDlgItem(IDC_TYPE)->SetWindowText(GUILoadString(IDS_TYPE) + csGColon);

	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_CREATE));
	GetDlgItem(IDCANCEL)->SetWindowText(GUILoadString(IDS_CANCEL));

	CComboBox *pComboBox = (CComboBox*)GetDlgItem(IDC_NETWORK_TYPE);
	if(pComboBox)
	{
		pComboBox->AddString(_T("Mesh"));
		pComboBox->AddString(_T("Hub & Spoke"));
	//	pComboBox->AddString(_T("Gateway"));

		pComboBox->SetCurSel(0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CDlgCreateSubnet::OnServerReply(WPARAM wParam, LPARAM lParam)
{
	KillTimer(TIMER_WAIT_ID);

	// CSRC_SUCCESS: Success. CSRC_OBJECT_ALREADY_EXIST: NetName has existed. CSRC_INVALID_PARAM: Invalid string.
	switch(wParam)
	{
		case CSRC_SUCCESS:
			EndDialog(0);
		//	AfxMessageBox(_T("Create successfully."));
			break;

		case CSRC_OBJECT_ALREADY_EXIST:
			AfxMessageBox(GUILoadString(IDS_VNET_ID_EXISTED));
			break;

		case CSRC_INVALID_PARAM:
			AfxMessageBox(GUILoadString(IDS_STRING_LEN_ERROR));
			break;

		default:
			AfxMessageBox(GUILoadString(IDS_UNKNOW_ERROR));
	}

	GetDlgItem(IDOK)->EnableWindow(TRUE);

	return 0;
}

void CDlgCreateSubnet::OnBnClickedOk()
{
	CString NetName, Password;
	GetDlgItemText(IDC_NETNAME, NetName);
	GetDlgItemText(IDC_PASSWORD, Password);

	INT iLen;
	CString info;
	if(NetName == _T(""))
	{
		AfxMessageBox(GUILoadString(IDS_INPUT_VNET_ID));
		return;
	}
	iLen = CheckUTF8Length(NetName, NetName.GetLength());
	if(iLen > MAX_NET_NAME_LENGTH)
	{
		info.Format(GUILoadString(IDS_MAX_VNET_ID_LEN), MAX_NET_NAME_LENGTH);
		AfxMessageBox(info);
		return;
	}

	if(Password == _T(""))
	{
		AfxMessageBox(GUILoadString(IDS_INPUT_PASSWORD));
		return;
	}
	iLen = CheckUTF8Length(Password, Password.GetLength());
	if(iLen > MAX_NET_PASSWORD_LEN)
	{
		info.Format(GUILoadString(IDS_MAX_PASSWORD_LEN), MAX_NET_PASSWORD_LEN);
		AfxMessageBox(info);
		return;
	}

	CComboBox *pComboBox = (CComboBox*)GetDlgItem(IDC_NETWORK_TYPE);
	INT iIndex = pComboBox->GetCurSel();
	USHORT NetworkType = VNT_MESH;
	switch(iIndex)
	{
		case 0:
			NetworkType = VNT_MESH;
			break;

		case 1:
			NetworkType = VNT_HUB_AND_SPOKE;
			break;
	}

	*m_pNotifyHandle = GetSafeHwnd();

	AddJobCreateJoinSubnet(NetName.GetBuffer(), Password.GetBuffer(), TRUE, NetworkType);

	GetDlgItem(IDOK)->EnableWindow(FALSE);
	SetTimer(TIMER_WAIT_ID, TIME_OUT, 0);
//	OnOK();
}

void CDlgCreateSubnet::OnTimer(UINT_PTR nIDEvent)
{
	ASSERT(nIDEvent == TIMER_WAIT_ID);

	//printx("Enter CDlgCreateSubnet::OnTimer()\n");

	KillTimer(TIMER_WAIT_ID);
	AfxMessageBox(GUILoadString(IDS_SERVER_NO_RESPONSE));

	EndDialog(0);
//	CDialog::OnTimer(nIDEvent);
}


CDlgJoinNet::CDlgJoinNet(CWnd *pParent /*=NULL*/)
:CDialog(CDlgJoinNet::IDD, pParent), m_pNotifyHandle(0)
{
}

CDlgJoinNet::~CDlgJoinNet()
{
	*m_pNotifyHandle = 0;
}

void CDlgJoinNet::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgJoinNet, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgJoinNet::OnBnClickedJoin)
	ON_MESSAGE(WM_SERVER_RESPONSE_EVENT, &CDlgJoinNet::OnServerReply)
	ON_WM_TIMER()
END_MESSAGE_MAP()


BOOL CDlgJoinNet::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(GUILoadString(IDS_JOIN_NETWORK));

	GetDlgItem(IDC_NETWORK_ID)->SetWindowText(GUILoadString(IDS_NETWORK_ID) + csGColon);
	GetDlgItem(IDC_NETWORK_PASSWORD)->SetWindowText(GUILoadString(IDS_NETWORK_PASSWORD) + csGColon);

	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_JOIN));
	GetDlgItem(IDCANCEL)->SetWindowText(GUILoadString(IDS_CANCEL));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

LRESULT CDlgJoinNet::OnServerReply(WPARAM wParam, LPARAM lParam)
{
	KillTimer(TIMER_WAIT_ID);

	switch(wParam)
	{
		case CSRC_SUCCESS:
			EndDialog(0);
			break;
		case CSRC_OBJECT_NOT_FOUND:
			AfxMessageBox(GUILoadString(IDS_NO_VNET));
			break;
		case CSRC_PASSWORD_ERROR:
			AfxMessageBox(GUILoadString(IDS_PASSWORD_ERROR));
			break;
		case CSRC_NETWORK_LOCKED:
			AfxMessageBox(GUILoadString(IDS_LOCKED_VNET));
			break;
		case CSRC_ALREADY_JOINED:
			AfxMessageBox(GUILoadString(IDS_HAS_JOINED));
			break;

		default:
			AfxMessageBox(GUILoadString(IDS_UNKNOW_ERROR));
	}

	GetDlgItem(IDOK)->EnableWindow(TRUE);

	return 0;
}

void CDlgJoinNet::OnBnClickedJoin()
{
	INT iLen;
	CString NetName, Password, info;
	GetDlgItemText(IDC_NETNAME, NetName);
	GetDlgItemText(IDC_PASSWORD, Password);

	if(NetName == _T(""))
	{
		AfxMessageBox(GUILoadString(IDS_INPUT_VNET_ID));
		return;
	}
	iLen = CheckUTF8Length(NetName, NetName.GetLength());
	if(iLen > MAX_NET_NAME_LENGTH)
	{
		info.Format(GUILoadString(IDS_MAX_VNET_ID_LEN), MAX_NET_NAME_LENGTH);
		AfxMessageBox(info);
		return;
	}

	//if(Password == _T(""))
	//{
	//	AfxMessageBox(GUILoadString(IDS_INPUT_PASSWORD));
	//	return;
	//}
	iLen = CheckUTF8Length(Password, Password.GetLength());
	if(iLen > MAX_NET_PASSWORD_LEN)
	{
		info.Format(GUILoadString(IDS_MAX_PASSWORD_LEN), MAX_NET_PASSWORD_LEN);
		AfxMessageBox(info);
		return;
	}

	*m_pNotifyHandle = GetSafeHwnd();

	AddJobCreateJoinSubnet(NetName, Password, FALSE, 0);

	GetDlgItem(IDOK)->EnableWindow(FALSE);
	SetTimer(TIMER_WAIT_ID, TIME_OUT, 0);
//	OnOK();
}

void CDlgJoinNet::OnTimer(UINT_PTR nIDEvent)
{
	ASSERT(nIDEvent == TIMER_WAIT_ID);

//	printx("Enter CDlgJoinNet::OnTimer()\n");

	KillTimer(TIMER_WAIT_ID);
	AfxMessageBox(GUILoadString(IDS_SERVER_NO_RESPONSE));

	EndDialog(0);
//	CDialog::OnTimer(nIDEvent);
}


BEGIN_MESSAGE_MAP(CDlgResetNetwork, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgResetNetwork::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CDlgResetNetwork::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString cs = GUILoadString(IDS_RESET_PASSWORD);
	if(GetUILanguage() == DEFAULT_LANGUAGE_ID)
		cs.Remove('&');
	SetWindowText(cs);

	GetDlgItem(IDC_S_NETWORK_ID)->SetWindowText(GUILoadString(IDS_NETWORK_ID) + csGColon);
	GetDlgItem(IDC_S_NETWORK_PASSWORD)->SetWindowText(GUILoadString(IDS_NETWORK_PASSWORD) + csGColon);

	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));
	GetDlgItem(IDCANCEL)->SetWindowText(GUILoadString(IDS_CANCEL));

	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_NETWORK_ID);
	CRect rect;
	pEdit->GetWindowRect(rect);
	ScreenToClient(rect);

//	m_SerCtrlFlag |= SCF_RENAME_NETWORK; // Test code.

	DWORD dwFlag = ES_LEFT | ES_AUTOHSCROLL;
	if(!(m_SerCtrlFlag & SCF_RENAME_NETWORK))
		dwFlag |= ES_READONLY;

	m_Edit.CreateEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, _T("Edit"), 0, WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | dwFlag, rect, this, IDC_NETNAME);
	m_Edit.SetFont(GetFont());
	m_Edit.SetWindowText(m_NetworkName);

	if(m_SerCtrlFlag & SCF_RENAME_NETWORK)
	{
		SetWindowText(GUILoadString(IDS_RESET_ID_PASSWORD));
		m_Edit.SetSel(MAKEWORD(-1, 0));
		m_Edit.SetFocus();
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgResetNetwork::OnBnClickedOk()
{
	GetDlgItemText(IDC_NETWORK_PASSWORD, m_Password);

	if(m_Password == _T(""))
		AfxMessageBox(GUILoadString(IDS_INPUT_PASSWORD));
	else
		OnOK();
}


BEGIN_MESSAGE_MAP(CDlgSubgroupSetting, CDialog)
	ON_MESSAGE(WM_UPDATE_GROUP_NAME, &CDlgSubgroupSetting::OnUpdateGroupName)
	ON_BN_CLICKED(IDC_RENAME, &CDlgSubgroupSetting::OnBnClickedRename)
	ON_EN_CHANGE(IDC_GROUP_NAME, &CDlgSubgroupSetting::OnEnChangeGroupName)
	ON_BN_CLICKED(IDOK, &CDlgSubgroupSetting::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CDlgSubgroupSetting::OnInitDialog()
{
	CDialog::OnInitDialog();

	stGUISubgroup *pGroup = GetGroupObject();
	if(!pGroup)
	{
		EndDialog(0);
		return FALSE;
	}

	CString cs;
	uCharToCString(pGroup->VNetGroup.GroupName, -1, cs);
	UpdateTitleName(cs);
	GetDlgItem(IDC_RENAME)->SetWindowText(GUILoadString(IDS_RENAME));

	CStatic *pStaticGroupName = (CStatic*)GetDlgItem(IDC_S_GROUP_NAME);
	CStatic *pStaticOption = (CStatic*)GetDlgItem(IDC_S_OPTION);
	pStaticGroupName->SetWindowText(GUILoadString(IDS_SUBGROUP_NAME));
	pStaticOption->SetWindowText(GUILoadString(IDS_SETTINGS));

	CButton *pCheckButton = (CButton*)GetDlgItem(IDC_DEF_OFFLINE);

	cs = _T(" ") + GUILoadString(IDS_DISABLE_SUBGROUP);
	pCheckButton->SetWindowText(cs);
	if(pGroup->VNetGroup.Flag & VGF_STATE_OFFLINE)
		pCheckButton->SetCheck(BST_CHECKED);

	*m_pNotifyHandle = GetSafeHwnd();
	m_pEdit = (CEdit*)GetDlgItem(IDC_GROUP_NAME);
	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));

	if(!m_nGroupIndex)
	{
		GetDlgItem(IDC_RENAME)->EnableWindow(FALSE);
		m_pEdit->EnableWindow(FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgSubgroupSetting::UpdateTitleName(const TCHAR *pGroupName)
{
	CString TitleName;
	TitleName.Format(_T("%s - %s"), GUILoadString(IDS_SUBGROUP_SETTING), pGroupName);
	SetWindowText(TitleName);
}

stGUISubgroup* CDlgSubgroupSetting::GetGroupObject()
{
	stGUIVLanInfo *pInfo = GNetworkManager.FindVNet(m_VNetID);
	if(pInfo && pInfo->m_nTotalGroup > m_nGroupIndex)
		return &pInfo->m_GroupArray[m_nGroupIndex];

	return 0;
}

LRESULT CDlgSubgroupSetting::OnUpdateGroupName(WPARAM wParam, LPARAM lParam)
{
	printx("---> OnUpdateGroupName\n");

	if(m_nGroupIndex != 0 && wParam == m_VNetID && lParam == m_nGroupIndex)
	{
		stGUISubgroup *pGroup = GetGroupObject();
		if(!pGroup)
			EndDialog(0);
		else
		{
			CString cs;
			uCharToCString(pGroup->VNetGroup.GroupName, -1, cs);
			UpdateTitleName(cs);
			GetDlgItem(IDC_RENAME)->EnableWindow();
		}
	}

	return 0;
}

void CDlgSubgroupSetting::OnBnClickedRename()
{
	CString cs;
	m_pEdit->GetWindowText(cs);

	UINT nLength = cs.GetLength();
	if(nLength && nLength <= MAX_GROUP_NAME_LEN)
	{
		AddJobRenameGroup(m_VNetID, m_nGroupIndex, cs, nLength);
		GetDlgItem(IDC_RENAME)->EnableWindow(FALSE);
		m_pEdit->SetWindowText(_T(""));
	}
	else
	{
	}
}

void CDlgSubgroupSetting::OnEnChangeGroupName()
{
	// TODO:  If this is a RICHEDIT control, the control will not send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask() with the ENM_CHANGE flag ORed into the mask.
}

void CDlgSubgroupSetting::OnBnClickedOk()
{
	stGUISubgroup *pGroup = GetGroupObject();
	if(!pGroup)
		goto End;

	DWORD dwNewFlag = 0;
	CButton *pCheckButton = (CButton*)GetDlgItem(IDC_DEF_OFFLINE);

	if(pCheckButton->GetCheck() == BST_CHECKED)
		dwNewFlag |= VGF_STATE_OFFLINE;

	if((dwNewFlag & VGF_STATE_OFFLINE) != (pGroup->VNetGroup.Flag & VGF_STATE_OFFLINE))
		AddJobSetGroupFlag(m_VNetID, m_nGroupIndex, VGF_STATE_OFFLINE, dwNewFlag);

End:
	OnOK();
}


BEGIN_MESSAGE_MAP(CDlgNetMgr, CDialog)

//	ON_MESSAGE(WM_SERVER_RESPONSE_EVENT, &CDlgNetMgr::OnServerReply)
	ON_MESSAGE(WM_VNET_MEMBER_INFO, &CDlgNetMgr::OnVNetMemberInfo)
	ON_MESSAGE(WM_RELAY_HOST_CHANGED, &CDlgNetMgr::OnRelayHostChanged)
	ON_MESSAGE(WM_MEMBER_REMOVED_EVENT, &CDlgNetMgr::OnMemberRemovedEvent)

	ON_BN_CLICKED(IDC_RESET_PASSWORD, &CDlgNetMgr::OnBnClickedResetPassword)
	ON_BN_CLICKED(IDOK, &CDlgNetMgr::OnBnClickedOk)
	ON_BN_CLICKED(IDC_QUERY_MEMBERS, &CDlgNetMgr::OnBnClickedQueryMembers)
	ON_BN_CLICKED(IDC_ACT, &CDlgNetMgr::OnBnClickedAct)
	ON_CBN_SELCHANGE(IDC_TYPE_COMBO, &CDlgNetMgr::OnCbnSelchangeTypeCombo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INFO_LIST, &CDlgNetMgr::OnLvnItemchangedInfoList)

	ON_WM_SIZING()

END_MESSAGE_MAP()


void CDlgNetMgr::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CDlgNetMgr::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString str = GUILoadString(IDS_NETWORK_MANAGEMENT);
	str += (_T(" - ") + m_NetName);
	SetWindowText(str);

	m_pOkButton = (CButton*)GetDlgItem(IDOK);
	m_pOkButton->SetWindowText(GUILoadString(IDS_OK));

	if(!m_bIsADM)
	{
		GetDlgItem(IDC_GROUP_RH)->SetWindowText(GUILoadString(IDS_RELAY_HOST));
		GetDlgItem(IDC_ALLOW_RELAY)->SetWindowText(GUILoadString(IDS_ALLOW_RELAY));

		if(m_bAllowRelay)
			((CButton*)GetDlgItem(IDC_ALLOW_RELAY))->SetCheck(BST_CHECKED);
	}
	else
	{
		GetDlgItem(IDC_GLOBAL_LOCK)->SetWindowText(GUILoadString(IDS_GLOBAL_LOCK));
		GetDlgItem(IDC_DISALLOW_USER)->SetWindowText(GUILoadString(IDS_DISALLOW_NEW_USER));
		GetDlgItem(IDC_DISALLOW_DESC)->SetWindowText(GUILoadString(IDS_DISALLOW_DESC));

		GetDlgItem(IDC_NETWORK_PASSWORD)->SetWindowText(GUILoadString(IDS_NETWORK_PASSWORD));
		GetDlgItem(IDC_NEED_PASSWORD)->SetWindowText(GUILoadString(IDS_VALIDATE_PASSWORD));

		str = GUILoadString(IDS_RESET_PASSWORD);
		if(GetUILanguage() != DEFAULT_LANGUAGE_ID)
			str += _T(" (&P)");
		GetDlgItem(IDC_RESET_PASSWORD)->SetWindowText(str);

		m_csRelayOn = GUILoadString(IDS_ACTIVE);
		m_csRelayOff = GUILoadString(IDS_DEACTIVE);
		m_csSetOn = GUILoadString(IDS_ACTIVATE);
		m_csSetOff = GUILoadString(IDS_DEACTIVATE);

		m_pListCtrl = (CListCtrl*)GetDlgItem(IDC_INFO_LIST);
		m_pActButton = (CButton*)GetDlgItem(IDC_ACT);

		if(m_bDisallowUser)
			((CButton*)GetDlgItem(IDC_DISALLOW_USER))->SetCheck(BST_CHECKED);
		if(m_bNeedPassword)
			((CButton*)GetDlgItem(IDC_NEED_PASSWORD))->SetCheck(BST_CHECKED);

		*m_pNotifyHandle = GetSafeHwnd();

		CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_TYPE_COMBO);
		CButton *pButtonQuery = (CButton*)GetDlgItem(IDC_QUERY_MEMBERS);
		CButton *pButtonAct = (CButton*)GetDlgItem(IDC_ACT);
		CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_INFO_LIST);

		pList->SetExtendedStyle(pList->GetExtendedStyle() | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

		pCombo->InsertString(0, GUILoadString(IDS_MEMBER_INFO));
		pCombo->InsertString(1, GUILoadString(IDS_RELAY_HOST));
		pCombo->SetCurSel(0);

		m_AdmMode = ADM_MODE_MEMBER_INFO;
		SetupGUIForAdmMode();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgNetMgr::OnSizing(UINT fwSide, LPRECT pRect)
{
	CDialog::OnSizing(fwSide, pRect);

	if(pRect->bottom - pRect->top < ADM_MODE_MIN_HEIGHT)
	{
		if(fwSide == WMSZ_TOP || fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT)
			pRect->top = pRect->bottom - ADM_MODE_MIN_HEIGHT;
		else
			pRect->bottom = pRect->top + ADM_MODE_MIN_HEIGHT;
	}
	if(pRect->right - pRect->left < ADM_MODE_MIN_WIDTH)
	{
		if(fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_LEFT || fwSide == WMSZ_TOPLEFT)
			pRect->left = pRect->right - ADM_MODE_MIN_WIDTH;
		else
			pRect->right = pRect->left + ADM_MODE_MIN_WIDTH;
	}

	CRect rect = pRect;
	GetWindowRect(m_Rect);

	INT nYOffset = m_Rect.Height() - rect.Height();
	if(m_bIsADM && nYOffset)
	{
		if(fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_BOTTOMRIGHT || fwSide == WMSZ_BOTTOM)
		{
			UpdateControl(m_pActButton, -nYOffset);
			UpdateControl(m_pOkButton, -nYOffset);

			UpdateControlSize(m_pListCtrl, -nYOffset, 0);
		}
		else if(fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT || fwSide == WMSZ_TOP)
		{
			UpdateControl(m_pActButton, -nYOffset);
			UpdateControl(m_pOkButton, -nYOffset);

			UpdateControlSize(m_pListCtrl, -nYOffset, 0);
		}
	}
}

/*
LRESULT CDlgNetMgr::OnServerReply(WPARAM wParam, LPARAM lParam)
{
//	printx("---> OnServerReply\n");
	KillTimer(TIMER_WAIT_ID);

	switch(wParam)
	{
		case 1:
			EndDialog(0);
			break;
	}

	return 0;
}
*/

LRESULT CDlgNetMgr::OnVNetMemberInfo(WPARAM wParam, LPARAM lParam)
{
	if(m_AdmMode != ADM_MODE_MEMBER_INFO)
		return 0;

	IPV4 ip;
	CString cs, csNoData = GUILoadString(IDS_NO_DATA);
	struct tm *p = 0;
	stVNetMemberInfo info;
	DWORD dwType, dwRCode, dwVNetID, dwPacketIndex, dwInfoCount;
	stGUIEventMsg *pMsg = (stGUIEventMsg*)wParam;
	CStreamBuffer sb;
	sb.AttachBuffer(pMsg->GetHeapData(), pMsg->GetHeapDataSize());

	sb >> dwType >> dwRCode >> dwVNetID >> dwPacketIndex >> dwInfoCount;

	do
	{
		if(dwVNetID != m_VNetID)
			break;
		stGUIVLanInfo *pVLan = GNetworkManager.FindVNet(m_VNetID);
		if(!pVLan)
			break;
		if(!dwPacketIndex)
			m_pListCtrl->DeleteAllItems();

		for(UINT i = 0, index = m_pListCtrl->GetItemCount(); i < dwInfoCount; ++i)
		{
			sb.Read(&info, sizeof(info));
			if(info.uid == m_LocalUID)
				continue;

			stGUIVLanMember *pMember = pVLan->FindMember(info.uid);
			if(!pMember)
				continue;

			INT iItem = m_pListCtrl->InsertItem(index++, pMember->HostName);
			m_pListCtrl->SetItemData(iItem, pMember->dwUserID);
			ip = info.IP;
			cs.Format(_T("%d.%d.%d.%d"), ip.b1, ip.b2, ip.b3, ip.b4);
			m_pListCtrl->SetItemText(iItem, 1, cs);

			p = info.Time ? localtime(&info.Time) : 0;
			if(p)
			{
				cs.Format(_T("%04d-%02d-%02d %02d:%02d"), p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min);
				m_pListCtrl->SetItemText(iItem, 2, cs);
			}
			else
				m_pListCtrl->SetItemText(iItem, 2, csNoData);
		}
	}
	while(0);

	sb.DetachBuffer();

	return 0;
}

LRESULT CDlgNetMgr::OnRelayHostChanged(WPARAM wParam, LPARAM lParam)
{
	if(m_AdmMode != ADM_MODE_RELAY_HOST)
		return 0;

	DWORD UID = (DWORD)wParam;
	stGUIVLanInfo *pVLan = GNetworkManager.FindVNet(m_VNetID);
	if(!pVLan)
		return 0;

	INT i, nCount, iSelected = m_pListCtrl->GetNextItem(-1, LVNI_SELECTED);

	if(!UID)
	{
		m_pListCtrl->SetItemText(0, 2, (pVLan->Flag & VF_RELAY_HOST) ? m_csRelayOn : m_csRelayOff);
		if(iSelected == 0)
			m_pActButton->SetWindowText((pVLan->Flag & VF_RELAY_HOST) ? m_csSetOff : m_csSetOn);
		return 0;
	}

	for(i = 0, nCount = m_pListCtrl->GetItemCount(); i < nCount; ++i)
		if(m_pListCtrl->GetItemData(i) == UID)
		{
			stGUIVLanMember *pMember = pVLan->FindMember(UID);
			m_pListCtrl->SetItemText(i, 2, (pMember->Flag & VF_RELAY_HOST) ? m_csRelayOn : m_csRelayOff);

			if(iSelected == i)
				m_pActButton->SetWindowText((pMember->Flag & VF_RELAY_HOST) ? m_csSetOff : m_csSetOn);
			break;
		}

	return 0;
}

LRESULT CDlgNetMgr::OnMemberRemovedEvent(WPARAM wParam, LPARAM lParam)
{
	if(m_AdmMode != ADM_MODE_MEMBER_INFO)
		return 0;

	DWORD dwUID = wParam;
	for(INT i = 0, count = m_pListCtrl->GetItemCount(); i < count; ++i)
		if(m_pListCtrl->GetItemData(i) == dwUID)
		{
			m_pListCtrl->DeleteItem(i);
			break;
		}

	return 0;
}

void CDlgNetMgr::OnBnClickedResetPassword()
{
	CDlgResetNetwork Dlg;
	Dlg.SetString(m_NetName, _T(""), m_dwSerCtrlFlag);
	if(Dlg.DoModal() == IDOK)
	{
		CString password = Dlg.GetNetworkPassword(); // No length check currently.

		if(password != _T(""))
		{
			AddJobResetNetworkPassword(m_VNetID, m_NetName, password);
		}
	//	printx("New password: %S\n", Dlg.GetNetworkPassword());
	}
}

void CDlgNetMgr::OnBnClickedOk()
{
	DWORD dwFlag = 0;

	if(m_bIsADM)
	{
		bool bDisallowUser, bNeedPassword;
		bDisallowUser = (((CButton*)GetDlgItem(IDC_DISALLOW_USER))->GetCheck() == BST_CHECKED);
		bNeedPassword = (((CButton*)GetDlgItem(IDC_NEED_PASSWORD))->GetCheck() == BST_CHECKED);
		if(m_bDisallowUser != bDisallowUser || m_bNeedPassword != bNeedPassword)
		{
			dwFlag |= (bDisallowUser ? VF_DISALLOW_USER : 0);
			dwFlag |= (bNeedPassword ? VF_NEED_PASSWORD : 0);
			AddJobResetNetworkFlag(m_VNetID, m_NetName, dwFlag, VF_DISALLOW_USER | VF_NEED_PASSWORD);
		}
	}
	else
	{
		bool bAllowRelay = (((CButton*)GetDlgItem(IDC_ALLOW_RELAY))->GetCheck() == BST_CHECKED);
		if(m_bAllowRelay != bAllowRelay)
		{
			dwFlag |= (bAllowRelay ? VF_ALLOW_RELAY : 0);
			AddJobSetHostRole(m_VNetID, m_NetName, 0, dwFlag, VF_ALLOW_RELAY);
		}
	}

	OnOK();
}

void CDlgNetMgr::OnCancel()
{
//	printx("---> OnCancel\n");
	CDialog::OnCancel();
}

void CDlgNetMgr::OnBnClickedQueryMembers()
{
//	printx("---> CDlgNetMgr::OnBnClickedQueryMembers\n");
	AddJobQueryVNetMemberInfo(m_VNetID);
}

void CDlgNetMgr::OnBnClickedAct()
{
	switch(m_AdmMode)
	{
		case ADM_MODE_NULL:
			break;

		case ADM_MODE_MEMBER_INFO:
			{
				INT iItem = m_pListCtrl->GetNextItem(-1, LVNI_SELECTED);
				if(iItem == -1)
					break;

				AddJobRemoveUser(m_VNetID, m_pListCtrl->GetItemData(iItem));
			}
			break;

		case ADM_MODE_RELAY_HOST:
			{
				INT iItem = m_pListCtrl->GetNextItem(-1, LVNI_SELECTED);
				if(iItem == -1)
					break;

				DWORD dwFlag, dwUID;
				stGUIVLanInfo *pVLan = GNetworkManager.FindVNet(m_VNetID);
				if(!pVLan)
					break;

				if(!iItem) // Local host.
				{
					dwUID = m_LocalUID;
					dwFlag = pVLan->Flag;
				}
				else
				{
					stGUIVLanMember *pMember = pVLan->FindMember(m_pListCtrl->GetItemData(iItem));
					if(!pMember)
						break;

					dwUID = pMember->dwUserID;
					dwFlag = pMember->Flag;
				}

				if(dwFlag & VF_RELAY_HOST)
					dwFlag &= ~VF_RELAY_HOST;
				else
					dwFlag |= VF_RELAY_HOST;

				AddJobSetHostRole(pVLan->NetIDCode, pVLan->NetName, dwUID, dwFlag, VF_RELAY_HOST);
			}
			break;
	}
}

void CDlgNetMgr::SetupGUIForAdmMode()
{
	CButton *pQueryButton = (CButton*)GetDlgItem(IDC_QUERY_MEMBERS);

	INT nColumnCount = m_pListCtrl->GetHeaderCtrl()->GetItemCount(); // Delete all of the columns.
	for (INT i = 0; i < nColumnCount; ++i)
		m_pListCtrl->DeleteColumn(0);
	m_pListCtrl->DeleteAllItems();

	switch(m_AdmMode)
	{
		case ADM_MODE_NULL:
			break;

		case ADM_MODE_MEMBER_INFO:
			{
				INT width[] = {150, 100, 105};
				CString string[3] = { GUILoadString(IDS_HOST_NAME), GUILoadString(IDS_LAST_ONLINE_IP), GUILoadString(IDS_LAST_ONLINE_TIME) };

				LV_COLUMN lvc;
				lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvc.fmt = LVCFMT_LEFT;

				for(INT i = 0; i < _countof(width); ++i)
				{
					lvc.pszText = string[i].GetBuffer();
					lvc.cx = width[i];
					lvc.iSubItem = i;

					m_pListCtrl->InsertColumn(i, &lvc);
				}

				pQueryButton->ShowWindow(SW_NORMAL);
				pQueryButton->SetWindowText(GUILoadString(IDS_QUERY));
				m_pActButton->SetWindowText(GUILoadString(IDS_EVICT));
			}
			break;

		case ADM_MODE_RELAY_HOST:
			{
				INT width[] = { 175, 95, 85 };
				CString string[] = { GUILoadString(IDS_HOST_NAME),  GUILoadString(IDS_VIRTUAL_IP), GUILoadString(IDS_STATUS) };

				LV_COLUMN lvc;
				lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvc.fmt = LVCFMT_LEFT;

				for(INT i = 0; i < _countof(width); ++i)
				{
					lvc.pszText = string[i].GetBuffer();
					lvc.cx = width[i];
					lvc.iSubItem = i;

					m_pListCtrl->InsertColumn(i, &lvc);
				}

				SetupRelayHostList(m_pListCtrl);
				pQueryButton->ShowWindow(SW_HIDE);

				m_pActButton->SetWindowText(m_csSetOn);
			}
			break;
	}
}

void CDlgNetMgr::SetupRelayHostList(CListCtrl *pListCtrl)
{
	stGUIVLanInfo *pVLan = GNetworkManager.FindVNet(m_VNetID);
	if(!pVLan)
		return;

	INT iItem;
	IPV4 vip;
	TCHAR *pVIPFormat = _T("%d.%d.%d.%d");
	CString csVIP;

	vip = m_LocalVIP;
	csVIP.Format(pVIPFormat, vip.b1, vip.b2, vip.b3, vip.b4);

	iItem = pListCtrl->InsertItem(0, m_LocalName);
	pListCtrl->SetItemData(iItem, 0);
	pListCtrl->SetItemText(iItem, 1, csVIP);
	pListCtrl->SetItemText(iItem, 2, (pVLan->Flag & VF_RELAY_HOST) ? m_csRelayOn : m_csRelayOff);

	POSITION pos = pVLan->MemberList.GetHeadPosition();
	for(UINT i = 1; pos; ++i)
	{
		stGUIVLanMember *pMember = pVLan->MemberList.GetNext(pos);

		if(!(pMember->Flag & VF_ALLOW_RELAY))
			continue;

		vip = pMember->vip;
		csVIP.Format(pVIPFormat, vip.b1, vip.b2, vip.b3, vip.b4);

		iItem = pListCtrl->InsertItem(i, pMember->HostName);
		pListCtrl->SetItemData(iItem, pMember->dwUserID);
		pListCtrl->SetItemText(iItem, 1, csVIP);
		pListCtrl->SetItemText(iItem, 2, (pMember->Flag & VF_RELAY_HOST) ? m_csRelayOn : m_csRelayOff);
	}
}

void CDlgNetMgr::UpdateControl(CWnd *pWnd, INT VerticalOffset, BOOL bShow)
{
	CPoint point;
	CRect rect;
	pWnd->GetWindowRect(&rect);
	point.x = rect.left;
	point.y = rect.top;
	ScreenToClient(&point);
	UINT nFlag = (bShow) ? SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW : SWP_NOSIZE | SWP_NOZORDER;
	pWnd->SetWindowPos(0, point.x, point.y + VerticalOffset, 0, 0, nFlag);
}

void CDlgNetMgr::UpdateControlSize(CWnd *pWnd, INT iNewHeight, INT iNewWidth)
{
	CRect rect;
	pWnd->GetWindowRect(rect);
	pWnd->SetWindowPos(0, 0, 0, rect.Width() + iNewWidth, rect.Height() + iNewHeight, SWP_NOMOVE | SWP_NOZORDER);
}

void CDlgNetMgr::OnCbnSelchangeTypeCombo()
{
	CComboBox *pCombo = (CComboBox*)GetDlgItem(IDC_TYPE_COMBO);
	UINT nIndex = pCombo->GetCurSel();

	if(!nIndex)
	{
		if(m_AdmMode != ADM_MODE_MEMBER_INFO)
		{
			m_AdmMode = ADM_MODE_MEMBER_INFO;
			SetupGUIForAdmMode();
		}
	}
	else if(nIndex == 1)
	{
		if(m_AdmMode != ADM_MODE_RELAY_HOST)
		{
			m_AdmMode = ADM_MODE_RELAY_HOST;
			SetupGUIForAdmMode();
		}
	}
}

void CDlgNetMgr::OnLvnItemchangedInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if(m_AdmMode == ADM_MODE_MEMBER_INFO)
	{
	}
	else if(m_AdmMode == ADM_MODE_RELAY_HOST)
	{
		do
		{
			INT iItem = m_pListCtrl->GetNextItem(-1, LVNI_SELECTED);
			if(iItem == -1)
				break;
			if(!(pNMLV->uNewState & LVIS_SELECTED))
				break;

			m_pActButton->SetWindowText((m_pListCtrl->GetItemText(iItem, 2) == m_csRelayOn) ? m_csSetOff : m_csSetOn);
		}
		while(0);
	}

	*pResult = 0;
}


BEGIN_MESSAGE_MAP(CEditSP, CEdit)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


BEGIN_MESSAGE_MAP(CDlgServerMsg, CDialog)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CDlgServerMsg::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CDlgServerMsg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_Edit.SubclassDlgItem(IDC_MSG, this);
	m_Edit.SetBkColor(RGB(255, 255, 220));

	if(m_MsgID)
	{
		SetWindowText(GUILoadString(IDS_SERVER_NEWS));
		GetDlgItem(IDC_CLOSE_MSG)->ShowWindow(SW_NORMAL);

		if(*m_pClosedID && m_MsgID == *m_pClosedID)
			((CButton*)GetDlgItem(IDC_CLOSE_MSG))->SetCheck(BST_CHECKED);
	}
	else
	{	
	}

	GetDlgItem(IDOK)->SetWindowText(GUILoadString(IDS_OK));
	GetDlgItem(IDC_CLOSE_MSG)->SetWindowText(GUILoadString(IDS_DONT_SHOW));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgServerMsg::OnClose()
{
//	printx("OnClose!\n");
	DestroyWindow();
	m_MsgID = 0;
}

void CDlgServerMsg::OnBnClickedOk()
{
	CButton *pButton = (CButton*)GetDlgItem(IDC_CLOSE_MSG);

	if(pButton->IsWindowVisible())
	{
		if(pButton->GetCheck() & BST_CHECKED)
		{
			AddJobCloseMsg(m_MsgID);
			*m_pClosedID = m_MsgID;
		}
		else
		{
			AddJobCloseMsg(0);
			*m_pClosedID = 0;
		}

		if(AppGetnMatrixCore()->IsPureGUIMode())
			theApp.SaveClosedMsgID(*m_pClosedID);
	}
	OnClose();
}

void CDlgServerMsg::OnCancel()
{
	OnClose();
}

void CDlgServerMsg::UpdatePos(CWnd *pParentWnd)
{
	CRect rect, rectClient/*, rectDesktop*/;
	pParentWnd->GetWindowRect(&rect);

	GetWindowRect(&rectClient);
//	SystemParametersInfo(SPI_GETWORKAREA, NULL, &rectDesktop, NULL);

	INT x, y;
	x = rect.left - rectClient.Width();
	y = rect.top;

	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	SetWindowPos(0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
}

void CDlgServerMsg::ShowMsg(CWnd *pParentWnd, DWORD MsgID, const TCHAR *pString)
{
	m_MsgID = MsgID; // Assign ID first.
	if(!GetSafeHwnd())
	{
		Create(IDD, pParentWnd);
		UpdatePos(pParentWnd);
		//DoModal();
	}

	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_MSG);
	pEdit->SetSel(0, -1);
	pEdit->Clear();

//	printx(_T("%s"), pString);
	pEdit->ReplaceSel(pString);
}


//CDlgHtmlNews::CDlgHtmlNews(CWnd* pParent /*=NULL*/)
//: CDHtmlDialog(CDlgHtmlNews::IDD, 0, pParent)
//{
//}
//
//
//BEGIN_MESSAGE_MAP(CDlgHtmlNews, CDHtmlDialog)
//	ON_WM_CLOSE()
//END_MESSAGE_MAP()
//
//BEGIN_DHTML_EVENT_MAP(CDlgHtmlNews)
//END_DHTML_EVENT_MAP()
//
//BOOL CDlgHtmlNews::OnInitDialog()
//{
//	CDHtmlDialog::OnInitDialog();
//
////	SetIcon(m_hIcon, TRUE);	 // Set big icon.
////	SetIcon(m_hIcon, FALSE); // Set small icon.
//
//	Navigate(_T("www.google.com"));
//
//	return TRUE;  // return TRUE  unless you set the focus to a control
//}
//
//void CDlgHtmlNews::OnClose()
//{
////	DestroyModeless();
//
//	CDHtmlDialog::OnClose();
//}


BEGIN_MESSAGE_MAP(CDlgTunnelPath, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgTunnelPath::OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PATH_LIST, OnItemChanged)
END_MESSAGE_MAP()


BOOL CDlgTunnelPath::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString csTitle, csTemp;
	csTitle.Format(_T("%s - %s (%d.%d.%d.%d)"), GUILoadString(IDS_TUNNEL_PATH), m_HostName, m_vip.b1, m_vip.b2, m_vip.b3, m_vip.b4);
	SetWindowText(csTitle);

	SetDlgItemText(IDOK, GUILoadString(IDS_USE));
	SetDlgItemText(IDCANCEL, GUILoadString(IDS_CLOSE));

	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_PATH_LIST);
	pList->SetExtendedStyle(pList->GetExtendedStyle() | /*LVS_EX_GRIDLINES | */LVS_EX_FULLROWSELECT);

	if(GetKeyState(VK_F2) < 0)
		m_bServerRelayRight = TRUE;

	TCHAR *Asterisk = _T("*"), *Dash = _T("-");
	CString string[4];
//	string[0] = GUILoadString();
	string[1] = GUILoadString(IDS_TYPE);
	string[2] = GUILoadString(IDS_NODE);
	string[3] = GUILoadString(IDS_NETWORK_GROUP);

	INT width[] = {20, 60, 160, 100};

	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;

	for(INT i = 0; i < _countof(width); ++i)
	{
		lvc.pszText = string[i].GetBuffer();
		lvc.cx = width[i];
		lvc.iSubItem = i;

		pList->InsertColumn(i, &lvc);
	}

	pList->InsertItem(0, m_RHNetID ? _T("") : Asterisk);
	pList->SetItemText(0, 1, GUILoadString(IDS_P2P));
	pList->SetItemText(0, 2, Dash);
	pList->SetItemText(0, 3, Dash);

	INT iItem = 1;
	string[0] = GUILoadString(IDS_RELAY);
	if(m_RHNetID == MASTER_SERVER_RELAY_ID || m_bServerRelayRight)
	{
		pList->InsertItem(iItem, (m_RHNetID == MASTER_SERVER_RELAY_ID) ? Asterisk : _T(""));
		pList->SetItemText(iItem, 1, string[0]);
		pList->SetItemText(iItem, 2, GUILoadString(IDS_NMATRIX_SERVER));
		pList->SetItemText(iItem, 3, Dash);
		m_bHasServerRelay = TRUE;
		iItem++;
	}

	IPV4 vip;
	for(UINT i = 0; i < m_TotalPath; ++i, ++iItem)
	{
		pList->InsertItem(iItem, (m_RHNetID == m_PathInfo[i].NetID) ? Asterisk : _T(""));
		pList->SetItemText(iItem, 1, string[0]);

		vip = m_PathInfo[i].vip;
		csTemp.Format(_T("%s (%d.%d.%d.%d)"), m_PathInfo[i].RelayHostName, vip.b1, vip.b2, vip.b3, vip.b4);

		pList->SetItemText(iItem, 2, csTemp);
		pList->SetItemText(iItem, 3, m_PathInfo[i].NetworkGroup);
	}

	EnableUseButton(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgTunnelPath::OnBnClickedOk()
{
//	printx("CDlgTunnelPath::OnBnClickedOk\n");
	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_PATH_LIST);

	INT iItem = pList->GetNextItem(-1, LVNI_SELECTED);
	if(iItem == -1)
		return;

	if(!iItem) // P2P.
	{
		if(m_RHNetID == MASTER_SERVER_RELAY_ID)
		{
			AddJobRequestRelay(RRT_REQUEST_SERVER, 0, m_DestUID, 0);
		}
		else
			for(UINT i = 0; i < m_TotalPath; ++i)
				if(m_PathInfo[i].NetID == m_RHNetID)
				{
					AddJobRequestRelay(RRT_CANCEL_RELAY, m_RHNetID, m_DestUID, m_PathInfo[i].RHUID);
					break;
				}
	}
	else
	{
		if(m_bServerRelayRight && iItem == 1)
			AddJobRequestRelay(RRT_REQUEST_SERVER, 0, m_DestUID, MASTER_SERVER_RELAY_ID);
		else
		{
			--iItem;
			if(m_bHasServerRelay)
				--iItem;
			AddJobRequestRelay(RRT_CLIENT_REQUEST, m_PathInfo[iItem].NetID, m_DestUID, m_PathInfo[iItem].RHUID);
		}
	}

	OnOK();
}

void CDlgTunnelPath::OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	//printx("new state: %d. old state: %d\n", pNMListView->uNewState, pNMListView->uOldState);

	if(pNMListView->uNewState == 0 && pNMListView->uOldState & LVIS_SELECTED) // Deselected.
	{
		EnableUseButton(FALSE);
		return;
	}

	if((pNMListView->uChanged & LVIF_STATE) && (pNMListView->uNewState & LVNI_SELECTED))
	{
		CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_PATH_LIST);
		INT iItem = pNMListView->iItem;
		ASSERT(iItem != -1);
		EnableUseButton((pList->GetItemText(iItem, 0) == _T("*")) ? FALSE : TRUE);
//		printx("Item changed!\n");
	}
}

BOOL CDlgTunnelPath::AddRelayPath(stGUIVLanInfo *pVNet, stGUIVLanMember *pRH)
{
	if(m_TotalPath == MAX_PATH_COUNT)
		return FALSE;

	m_PathInfo[m_TotalPath].NetworkGroup = pVNet->NetName;
	m_PathInfo[m_TotalPath].NetID = pVNet->NetIDCode;

	m_PathInfo[m_TotalPath].RelayHostName = pRH->HostName;
	m_PathInfo[m_TotalPath].vip = pRH->vip;
	m_PathInfo[m_TotalPath].RHUID = pRH->dwUserID;

	++m_TotalPath;

	return TRUE;
}


BEGIN_MESSAGE_MAP(CDlgPingHost, CDialog)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDOK, &CDlgPingHost::OnBnClickedOk)
	ON_MESSAGE(WM_PING_HOST_RESPONSE, &CDlgPingHost::OnPingHostResponse)
	ON_WM_SYSCOMMAND()
END_MESSAGE_MAP()


BOOL CDlgPingHost::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_Edit.SubclassDlgItem(IDC_EDIT, this);
	m_Edit.SetBkColor(RGB(255, 255, 255));

	CString csTitle = GUILoadString(IDS_PING_HOST);
	csTitle.Remove('&');
	INT pos = csTitle.Find('(');
	if(pos != -1)
		csTitle = csTitle.Left(pos);
	SetWindowText(csTitle);

	SetDlgItemText(IDOK, GUILoadString(IDS_CLOSE));
	SetTimer(TIMER_ID, 150, 0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgPingHost::OnSysCommand(UINT nID, LPARAM lParam)
{
	if(nID == SC_CLOSE)
	{
		DestroyWindow();
		return;
	}

	CDialog::OnSysCommand(nID, lParam);
}

void CDlgPingHost::OnTimer(UINT_PTR nIDEvent)
{
	ASSERT(nIDEvent == TIMER_ID);

	if(m_state == TS_START_PING)
	{
		ASSERT(!m_SendCount);

		CString info;
		info.Format(_T("\r\nPinging %s\r\n\r\n"), m_HostInfo);
		AppendText(info);
		PingOnce();
		m_state = TS_WAIT_RESPONSE;
	}
	else if(m_state == TS_WAIT_RESPONSE)
	{
		AppendText(_T("Request timed out!\r\n"));

		if(m_SendCount == MAX_PH_COUNT) // Request timed out.
		{
			KillTimer(TIMER_ID);
			m_state = TS_PH_OVER;
			ShowEndString();
		}
		else
			PingOnce();
	}
	else
	{
		ASSERT(m_state == TS_PING_AGAIN);
		PingOnce();
		m_state = TS_WAIT_RESPONSE;
	}

	CDialog::OnTimer(nIDEvent);
}

void CDlgPingHost::ShowEndString()
{
	AppendText(_T("\r\nPing completed!\r\n"));
}

void CDlgPingHost::PingOnce()
{
	CString info;
	SetTimer(TIMER_ID, 5000, 0); // Set wait time.
	++m_SendCount;
	AddJobPingHost(m_uid, m_SendCount);
	info.Format(_T("%dth ping......    "), m_SendCount);
	AppendText(info);
}

void CDlgPingHost::TestAgain()
{
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_EDIT);

	KillTimer(TIMER_ID);
	m_state = TS_START_PING;
	m_SendCount = 0;
	pEdit->SetReadOnly(FALSE);
	pEdit->SetSel(0, -1);
	pEdit->Clear();
	pEdit->SetReadOnly(TRUE);

	SetTimer(TIMER_ID, 150, 0);
}

void CDlgPingHost::AppendText(const TCHAR *pString)
{
	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_EDIT);

//	pEdit->SetSel(0, -1); // Select all text and move cursor at the end.
//	pEdit->SetSel(-1);    // Remove selection.

	INT pos = pEdit->GetWindowTextLength();
	pEdit->SetSel(pos, pos);

	CString temp;
	if(pString)
		pEdit->ReplaceSel(pString);
	else
		pEdit->ReplaceSel(_T("\r\n"));
}

void CDlgPingHost::OnCancel()
{
	DestroyWindow();
//	CDialog::OnCancel();
}

void CDlgPingHost::OnBnClickedOk()
{
	if(GetKeyState(VK_F2) < 0)
		TestAgain();
	else
		DestroyWindow();
}

LRESULT CDlgPingHost::OnPingHostResponse(WPARAM wParam, LPARAM lParam)
{
	WORD nID = HIWORD(lParam), time = LOWORD(lParam);
	CString cs;

//	printx("---> OnPingHostResponse. nID: %d Time: %d\n", nID, time);

	if(nID == m_SendCount)
	{
		if(!time)
			AppendText(_T("< 1 ms\r\n"));
		else
		{
			cs.Format(_T("%d ms\r\n"), time);
			AppendText(cs);
		}

		if(m_SendCount == MAX_PH_COUNT) // Request timed out.
		{
			KillTimer(TIMER_ID);
			m_state = TS_PH_OVER;
			ShowEndString();
		}
		else
		{
			m_state = TS_PING_AGAIN;
			SetTimer(TIMER_ID, 1000, 0);
		}
	}

	return 0;
}


BEGIN_MESSAGE_MAP(CDlgHostPicker, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgHostPicker::OnBnClickedOk)
	ON_BN_CLICKED(IDC_SELECT_ALL, &CDlgHostPicker::OnBnClickedSelectAll)
	ON_BN_CLICKED(IDC_REMOVE_ALL, &CDlgHostPicker::OnBnClickedRemoveAll)
	ON_NOTIFY(NM_CLICK, IDC_TREE, &CDlgHostPicker::OnNMClickTree)
END_MESSAGE_MAP()


BOOL CDlgHostPicker::OnInitDialog()
{
	CDialog::OnInitDialog();

	HTREEITEM hItem;
	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_TREE);

	// The code resolve that can't call CTreeCtrl::SetCheck in OnInitDialog directly.
	pTree->ModifyStyle(TVS_CHECKBOXES, 0);
	pTree->ModifyStyle(0, TVS_CHECKBOXES);

	CString HostName;
	stGUIVLanMember *pMember;
	CGUINetworkManager *pGUINetworkManager = (CGUINetworkManager*)m_pNetworkManager;

	if(m_pVNet != NULL)
	{
		CList<stGUIVLanMember*> &MemberList = m_pVNet->MemberList;

		for(POSITION pos = MemberList.GetHeadPosition(); pos; )
		{
			pMember = MemberList.GetNext(pos);

			if(pMember->LinkState == LS_OFFLINE)
				continue;

			HostName = _T(' ') + pMember->HostName;
			hItem = pTree->InsertItem(HostName);

			if(pMember->dwUserID == m_CheckedUID)
				pTree->SetCheck(hItem, TRUE);

			m_UIDArray[m_HostCount++] = pMember->dwUserID;
			if(m_HostCount == GUI_MAX_HOST_COUNT)
				break;
		}
	}
	else
	{
		BOOL bAdded;
		UINT i;
		DWORD dwHostUID;
		CList<stGUIVLanInfo*> &VNetList = pGUINetworkManager->GetVNetList();
		POSITION npos, mpos;

		for(npos = VNetList.GetHeadPosition(); npos; )
		{
			stGUIVLanInfo *pVNet = VNetList.GetNext(npos);
			CList<stGUIVLanMember*> &MemberList = pVNet->MemberList;

			for(mpos = MemberList.GetHeadPosition(); mpos; )
			{
				pMember = MemberList.GetNext(mpos);

				if(pMember->LinkState == LS_OFFLINE || pMember->LinkState == LS_NO_CONNECTION)
					continue;

				dwHostUID = pMember->dwUserID;
				for(bAdded = FALSE, i = 0; i < m_HostCount; ++i)
					if(dwHostUID == m_UIDArray[i])
					{
						bAdded = TRUE;
						break;
					}

				if(bAdded)
					continue;

				HostName = _T(' ') + pMember->HostName;
				hItem = pTree->InsertItem(HostName);

			//	if(dwHostUID == m_CheckedUID)
			//		pTree->SetCheck(hItem, TRUE);

				m_UIDArray[m_HostCount++] = dwHostUID;
				if(m_HostCount == GUI_MAX_HOST_COUNT)
					break;
			}
		}
	}


	SetWindowText(GUILoadString(IDS_SELECT_MEMBERS));
	SetDlgItemText(IDOK, GUILoadString(IDS_OK));
	SetDlgItemText(IDCANCEL, GUILoadString(IDS_CANCEL));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgHostPicker::OnBnClickedOk()
{
	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_TREE);
	HTREEITEM hItem;
	UINT index = 0, HostCount = 0;
	DWORD UIDArray[MAX_GROUP_CHAT_HOST];

	for(hItem = pTree->GetRootItem(); hItem; hItem = pTree->GetNextSiblingItem(hItem), index++)
	{
		if(!pTree->GetCheck(hItem))
			continue;

		UIDArray[HostCount++] = m_UIDArray[index];
		if(HostCount == (MAX_GROUP_CHAT_HOST - 1))
			break;
	}

	if(HostCount)
		memcpy(m_UIDArray, UIDArray, sizeof(DWORD) * HostCount);
	m_HostCount = HostCount;

	OnOK();
}

void CDlgHostPicker::OnBnClickedSelectAll()
{
	HTREEITEM hItem;
	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_TREE);

	for(hItem = pTree->GetRootItem(); hItem; hItem = pTree->GetNextSiblingItem(hItem))
		pTree->SetCheck(hItem);
}

void CDlgHostPicker::OnBnClickedRemoveAll()
{
	HTREEITEM hItem;
	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_TREE);

	for(hItem = pTree->GetRootItem(); hItem; hItem = pTree->GetNextSiblingItem(hItem))
		pTree->SetCheck(hItem, FALSE);
}

void OnTreeClick(CTreeCtrl *pTree)
{
	CPoint point;
	GetCursorPos(&point);
	pTree->ScreenToClient(&point);

	HTREEITEM hItem = pTree->HitTest(point);
	if(!hItem)
		return;

	BOOL bCheck = pTree->GetCheck(hItem);
	pTree->SetCheck(hItem, !bCheck);
}

void CDlgHostPicker::OnNMClickTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	//NM_TREEVIEW* pnmtv = (NM_TREEVIEW*) pNMHDR;
	CTreeCtrl *pTree = (CTreeCtrl*)GetDlgItem(IDC_TREE);
//	OnTreeClick(pTree);
	*pResult = 0;
}


BEGIN_MESSAGE_MAP(CDlgVNetSubgroup, CDialog)
	ON_BN_CLICKED(IDOK, &CDlgVNetSubgroup::OnBnClickedOk)
	ON_EN_CHANGE(IDC_GROUP_NAME, &CDlgVNetSubgroup::OnEnChangeGroupName)
END_MESSAGE_MAP()


BOOL CDlgVNetSubgroup::OnInitDialog()
{
	CDialog::OnInitDialog();

	CWnd *pOKButton = GetDlgItem(IDOK), *pCancelButton = GetDlgItem(IDCANCEL);
	GetDlgItem(IDOK)->EnableWindow(FALSE);
	m_bButtonEnabled = FALSE;

	pOKButton->SetWindowText(GUILoadString(IDS_OK));
	pCancelButton->SetWindowText(GUILoadString(IDS_CANCEL));

	CString cs = GUILoadString(IDS_SUBGROUP_NAME) + csGColon;
	SetDlgItemText(IDC_S_GROUP_NAME, cs);

	SetWindowText(GUILoadString(IDS_CREATE_SUBGROUP));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgVNetSubgroup::DoDataExchange(CDataExchange* pDX)
{
	DDX_Text(pDX, IDC_GROUP_NAME, m_csName);
	CDialog::DoDataExchange(pDX);
}

void CDlgVNetSubgroup::OnEnChangeGroupName()
{
	// TODO: If this is a RICHEDIT control, the control will not send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask() with the ENM_CHANGE flag ORed into the mask.
	// TODO: Add your control notification handler code here

	CString csGroupName;
	BOOL bConflict = FALSE;

	GetDlgItemText(IDC_GROUP_NAME, csGroupName);

	if(csGroupName == _T("") || csGroupName.GetLength() > MAX_GROUP_NAME_LEN)
		bConflict = TRUE;
	else
		for(UINT i = 0; i < m_TotalName; ++i)
			if(csGroupName == m_NameArray[i])
			{
				bConflict = TRUE;
				break;
			}

	if(bConflict)
	{
		if(m_bButtonEnabled)
		{
			GetDlgItem(IDOK)->EnableWindow(FALSE);
			m_bButtonEnabled = FALSE;
		}
	}
	else
	{
		if(!m_bButtonEnabled)
		{
			GetDlgItem(IDOK)->EnableWindow(TRUE);
			m_bButtonEnabled = TRUE;
		}
	}
}

void CDlgVNetSubgroup::OnBnClickedOk()
{
	OnOK();
}


