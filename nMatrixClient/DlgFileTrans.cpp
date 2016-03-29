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
#include "DlgFileTrans.h"


CDlgFileTrans::CDlgFileTrans(CWnd* pParent /*=NULL*/)
: CDialog(CDlgFileTrans::IDD, pParent)
{
}

CDlgFileTrans::~CDlgFileTrans()
{
}

void CDlgFileTrans::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


//IMPLEMENT_DYNAMIC(CDlgFileTrans, CDialog)
BEGIN_MESSAGE_MAP(CDlgFileTrans, CDialog)
END_MESSAGE_MAP()


void CDlgFileTrans::JobSendFile(const TCHAR *pFileName, DWORD vip, DWORD dwJobKey)
{
}

void CDlgFileTrans::JobReceiveFile(const TCHAR *pFileName, UINT nSize, DWORD vip, DWORD dwJobKey)
{
}

void CDlgFileTrans::StartTaskThread()
{
}

void CDlgFileTrans::EndTaskThread()
{
}