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


#define NETWORK_CONNECTION_NAME "nMatrix Virtual Network"
#define NMATRIX_SETUP_INFO_DIALOG_NAME "NMATRIX_SETUP_INFO_DIALOG"
#define THREAD_INIT_WAIT_TIME 100


#define DEFAULT_LANGUAGE_ID 0x0409 // English.


typedef void (*LanEventCallback)(void *pData, DWORD LanID);


enum LANGUAGE_LIST_ID
{
	LLI_AUTO = 0,
	LLI_ENGLISH,
	LLI_CHT,
	LLI_CHS,

	LLI_TOTAL
};

extern TCHAR *GLanguageName[LLI_TOTAL];
extern CString csGColon;

void  RegLanEventCB(LanEventCallback pFn, void *pData);
BOOL  SetGUILanguage(UINT LanListID);
DWORD GetUILanguage();
CString GUILoadString(DWORD StringID);

BOOL GetServiceFileDirectory(CString &csPath);
BOOL SetupServiceFile(BOOL &bVersionOK, CString &csPath); // If this function succeeds, bVersionOK must be true.


enum SETUP_RETURN_VALUE
{
	SRV_SUCCESS,
	SRV_REBOOT,

	SRV_NO_ADAPTER,
	SRV_UNINSTALL_SUCCESS

};


class CInfoDialog : public CDialog
{
public:

	CInfoDialog(CWnd *pParent = NULL)
	: CDialog(CInfoDialog::IDD, pParent), m_bEndDialog(FALSE), m_bResult(FALSE)
	{
		m_bDebug = FALSE;
		m_bUpdate = FALSE;
	}
	virtual ~CInfoDialog() {}


	enum { IDD = IDD_INFO };

	void SetMsg(CString msg) { m_NewMsg = msg; }
	void SetErrorMsg(CString &ErrorMsg) { m_ErrorMsg = ErrorMsg; }
	CString& GetErrorMsg() { return m_ErrorMsg; }
	void CloseDialog(BOOL bResult) { m_bResult = bResult; m_bEndDialog = TRUE; }

	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	void SetOSVersion(BOOL bIs64Bits) { m_b64Bit = bIs64Bits; }
	BOOL Is64BitsOS() { return m_b64Bit; }
	void SetDebug(BOOL bDebug) { m_bDebug = bDebug; }
	void SetUpdate() { m_bUpdate = TRUE; }
	void SetNdis6(BOOL bNdis6) { m_bNdis6 = bNdis6; }
	BOOL IsUpdate() { return m_bUpdate; }
	BOOL IsDebug() { return m_bDebug; }
	BOOL IsNdis6() { return m_bNdis6; }


protected:

	BOOL m_bEndDialog, m_bResult, m_b64Bit, m_bDebug, m_bUpdate, m_bNdis6;
	CString m_NewMsg, m_ErrorMsg;

	void UpdateMessage(TCHAR *pMsg);
	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_DYNAMIC(CInfoDialog)
	DECLARE_MESSAGE_MAP()


};


class CSetupDialog : public CDialog
{
public:

	CSetupDialog(CWnd* pParent = NULL);
	virtual ~CSetupDialog();

	enum { IDD = IDD_SETUP, MODE_INSTALL, MODE_UPDATE, MODE_UNINSTALL };


	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedDebug();
	afx_msg void OnBnClickedCopyErr();
	afx_msg void OnBnClickedNext();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);

	void EnableSilentUpdate() { m_bSilentUpdate = TRUE; }
	void SetMode(DWORD mode) { m_Mode = mode; }
	void SetInfoText(const TCHAR *pText);
	BOOL CheckDriverVersion();


	BOOL IsSetupSuccessful()
	{
		if(m_Mode == MODE_INSTALL)
			if(!m_bResult || m_bResult == 1)
				return TRUE;
		return FALSE;
	}


protected:

	DWORD m_Mode;
	BOOL  m_bDebug, m_bComplete, m_bResult, m_b64Bit, m_bSilentUpdate;
	CFont m_BoldFont;

	virtual void OnCancel();
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_DYNAMIC(CSetupDialog)
	DECLARE_MESSAGE_MAP()


};


