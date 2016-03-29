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


// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently


#pragma once


#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif

#include "targetver.h"


#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>      // MFC core and standard components
#include <afxext.h>      // MFC extensions
#include <afxsock.h>     // MFC socket extensions

#include <winsvc.h>
#include <string>

//#include <stdio.h>


#define POLARSSL_AES_C
#define UCHAR_AS_UTF8


#ifdef UCHAR_AS_UTF8
	typedef CHAR uChar; // UTF-8 character type.
	#define ustrlen strlen
#else
	typedef TCHAR uChar;
	#define ustrlen strlen
#endif


#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // MFC support for ribbons and control bars


