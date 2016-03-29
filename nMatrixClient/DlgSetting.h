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


class CDlgSettingBasic : public CDialog
{
public:
	CDlgSettingBasic(CWnd* pParent = NULL);
	virtual ~CDlgSettingBasic();


	enum { IDD = IDD_SETTING_BASIC };


	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);


protected:
	TCHAR m_LocalName[MAX_HOST_NAME_LENGTH + 1];
	DWORD m_UDPAddress;
	UINT m_UDPPort, m_LanListIndex, m_KeepTunnelTime;
	bool m_bAutoStart, m_bAutoReconnect, m_bAutoUpdate, m_bAutoSetFirewall;

	virtual void DoDataExchange(CDataExchange* pDX);

	friend class CDlgSetting;
	DECLARE_MESSAGE_MAP()


};


class CDlgSettingBandwidth : public CDialog
{
public:

	CDlgSettingBandwidth(CWnd* pParent = NULL);
	virtual ~CDlgSettingBandwidth();


	enum { IDD = IDD_SETTING_BANDWIDTH, MIN_VALUE = 0, MAX_VALUE = MAX_INT_VALUE };


	afx_msg void OnDeltaposSpinDatain(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinDataout(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);


protected:

	UINT m_DataInLimit, m_DataOutLimit;

	friend class CDlgSetting;
	DECLARE_MESSAGE_MAP()


};


class CDlgSetting : public CDialog
{
public:

	enum { IDD = IDD_DLG_SETTINGS, DLG_BASIC = 1, DLG_BANDWIDTH = 2 };

	CDlgSetting(CVPNClientDlg *pMainDlg, CnMatrixCore *pCore, stConfigData *pConfig);
	virtual ~CDlgSetting();

	virtual BOOL OnInitDialog();
	void ShowPanel(DWORD dwDlg);
	void UpdateSelectedCategory();

	afx_msg void OnBnClickedOk();
	afx_msg void OnLvnItemchangedListCategory(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickListCategory(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkListCategory(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRClickListCategory(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRDblclkListCategory(NMHDR *pNMHDR, LRESULT *pResult);


protected:

	CDlgSettingBasic     m_DlgBasic;
	CDlgSettingBandwidth m_DlgBandwidth;
	CDialog              *m_pCurrentDlg;
	CVPNClientDlg        *m_pMainDlg;
	CnMatrixCore         *m_pCore;
	stConfigData         *m_pConfig;
	CImageList           m_ImageList;

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()


};


void CloseServiceMode();
BOOL UpdateService(CVPNClientDlg *pMainDlg, CnMatrixCore *pnMatrixCore);


