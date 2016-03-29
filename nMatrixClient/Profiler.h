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


class CProfiler
{
public:
	CProfiler(){}
	~CProfiler(){}

	BOOL Attach(TCHAR *filename)
	{
		ZeroMemory(m_fullpath, sizeof(m_fullpath));

		if(filename[1] != ':')
		{
			GetCurrentDirectory(sizeof(m_fullpath), m_fullpath);
			_tcscat(m_fullpath, _T("\\"));
			_tcscat(m_fullpath, filename);
		}

		// Check if file exist.
		FILE *temp;
		if(temp = _tfopen(m_fullpath, _T("r")))
		{
			fclose(temp);
			return TRUE;
		}

		return FALSE;
	}

	INT ReadInt(TCHAR *section, TCHAR *key, INT idefault)
	{
		return GetPrivateProfileInt(section, key, idefault, m_fullpath);
	}

	FLOAT ReadFloat(TCHAR *section, TCHAR *key, FLOAT fdefault)
	{
		TCHAR buffer[128], buffer2[128];

		_stprintf(buffer2, _TEXT("%f"), fdefault);
		ReadString(section, key, buffer2, buffer, 128);
		return (FLOAT)TcharToDouble(buffer);
	}

	DWORD ReadString(TCHAR *section, TCHAR *key, TCHAR *cdefault, TCHAR *buffer, INT size)
	{
		return GetPrivateProfileString(section, key, cdefault, buffer, size, m_fullpath);
	}

	DOUBLE TcharToDouble(TCHAR *source)
	{
		INT    i = 0, counter = 0;
		DOUBLE f = 0, t = 1;
		BOOL   neg = (source[0] == '-'), dot = FALSE;

		while(source[counter])
		{
			switch(source[counter++])
			{
				case '0':break;
				case '1':f+=1*t; break;
				case '2':f+=2*t; break;
				case '3':f+=3*t; break;
				case '4':f+=4*t; break;
				case '5':f+=5*t; break;
				case '6':f+=6*t; break;
				case '7':f+=7*t; break;
				case '8':f+=8*t; break;
				case '9':f+=9*t; break;
				case '.':i=(int)f/10; f=0; dot=true; break;
			}
			if(dot) t *= 0.1f;
			else f *= 10;
		}
		return neg? (i+f)*-1: (i+f);
	}

	void WriteInt(TCHAR *section, TCHAR *key, INT i)
	{
		TCHAR buffer[128];
		_stprintf(buffer, _TEXT("%d"), i);
		WriteString(section, key, buffer);
	}

	void WriteFloat(TCHAR *section, TCHAR *key, FLOAT f)
	{
		TCHAR buffer[128];
		_stprintf(buffer, _TEXT("%f"), f);
		WriteString(section, key, buffer);
	}

	void WriteString(TCHAR *section, TCHAR *key, const TCHAR *string)
	{
		WritePrivateProfileString(section, key, string, m_fullpath);
	}


protected:
	TCHAR m_fullpath[MAX_PATH];

};


class CRegistryManager
{
public:
	CRegistryManager() {}
	~CRegistryManager() {}


protected:


};


