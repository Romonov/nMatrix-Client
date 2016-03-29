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


#include "StdAfx.h"
#include "StreamBuffer.h"


BOOL CStreamBuffer::AllocateBuffer(UINT size)
{
	ASSERT(m_flag != BT_USER); // Must detach manually.
	m_pos = 0;
	if(!size || size <= m_BufferSize)
		return 0;

	if(m_BufferSize)
		m_pBuffer = (BYTE*)realloc(m_pBuffer, size);
	else
		m_pBuffer = (BYTE*)malloc(size);

	if(m_pBuffer)
	{
		m_BufferSize = size;
		m_flag = BT_HEAP;
	}
	else
	{
		m_BufferSize = 0;
		m_flag = BT_NONE;
	}

	return (BOOL)m_pBuffer;
}

void CStreamBuffer::Release()
{
	switch(m_flag)
	{
		case BT_NONE:
			ASSERT(!m_pBuffer && !m_BufferSize);
			return;

		case BT_USER:
			DetachBuffer();
			break;

		case BT_HEAP:
			ASSERT(m_pBuffer && m_BufferSize);
			free(m_pBuffer); // Debug failed here may be caused by forgetting call DetachBuffer().
			m_pBuffer = NULL;
			m_BufferSize = 0;
			m_flag = BT_NONE;
			break;
	}
}


CStreamBuffer& CStreamBuffer::operator<<(bool b)
{
	BYTE byte = b;
	m_pBuffer[m_pos] = byte;
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(BYTE b)
{
	m_pBuffer[m_pos] = b;
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(USHORT us)
{
	memcpy(&m_pBuffer[m_pos], &us, sizeof(us));
	m_pos += sizeof(us);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(UINT ui)
{
	memcpy(&m_pBuffer[m_pos], &ui, sizeof(ui));
	m_pos += sizeof(ui);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(DWORD d)
{
	memcpy(&m_pBuffer[m_pos], &d, sizeof(d));
	m_pos += sizeof(d);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(UINT64 ui)
{
	memcpy(&m_pBuffer[m_pos], &ui, sizeof(ui));
	m_pos += sizeof(ui);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(CHAR c)
{
	m_pBuffer[m_pos] = c;
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(SHORT s)
{
	memcpy(&m_pBuffer[m_pos], &s, sizeof(s));
	m_pos += sizeof(s);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(INT i)
{
	memcpy(&m_pBuffer[m_pos], &i, sizeof(i));
	m_pos += sizeof(i);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(LONG l)
{
	memcpy(&m_pBuffer[m_pos], &l, sizeof(l));
	m_pos += sizeof(l);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(INT64 i)
{
	memcpy(&m_pBuffer[m_pos], &i, sizeof(i));
	m_pos += sizeof(i);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(FLOAT f)
{
	memcpy(&m_pBuffer[m_pos], &f, sizeof(f));
	m_pos += sizeof(f);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator<<(DOUBLE d)
{
	memcpy(&m_pBuffer[m_pos], &d, sizeof(d));
	m_pos += sizeof(d);
	return *this;
}


CStreamBuffer& CStreamBuffer::operator>>(bool &b)
{
	b = m_pBuffer[m_pos] & 0x01;
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(BYTE &b)
{
	b = m_pBuffer[m_pos];
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(USHORT &us)
{
	memcpy(&us, &m_pBuffer[m_pos], sizeof(us));
	m_pos += sizeof(us);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(UINT &ui)
{
	memcpy(&ui, &m_pBuffer[m_pos], sizeof(ui));
	m_pos += sizeof(ui);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(DWORD &d)
{
	memcpy(&d, &m_pBuffer[m_pos], sizeof(d));
	m_pos += sizeof(d);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(UINT64 &ui)
{
	memcpy(&ui, &m_pBuffer[m_pos], sizeof(ui));
	m_pos += sizeof(ui);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(CHAR &c)
{
	c = m_pBuffer[m_pos];
	++m_pos;
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(SHORT &s)
{
	memcpy(&s, &m_pBuffer[m_pos], sizeof(s));
	m_pos += sizeof(s);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(INT &i)
{
	memcpy(&i, &m_pBuffer[m_pos], sizeof(i));
	m_pos += sizeof(i);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(LONG &l)
{
	memcpy(&l, &m_pBuffer[m_pos], sizeof(l));
	m_pos += sizeof(l);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(INT64 &i)
{
	memcpy(&i, &m_pBuffer[m_pos], sizeof(i));
	m_pos += sizeof(i);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(FLOAT &f)
{
	memcpy(&f, &m_pBuffer[m_pos], sizeof(f));
	m_pos += sizeof(f);
	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(DOUBLE &d)
{
	memcpy(&d, &m_pBuffer[m_pos], sizeof(d));
	m_pos += sizeof(d);
	return *this;
}


INT CStreamBuffer::Write(const void *pData, UINT size)
{
	memcpy(&m_pBuffer[m_pos], pData, size);
	m_pos += size;
	return m_pos;
}

INT CStreamBuffer::Read(void *pData, UINT size)
{
	memcpy(pData, &m_pBuffer[m_pos], size);
	m_pos += size;
	return m_pos;
}


#ifdef __NETWORK_DATA_TYPE_H__


CStreamBuffer& CStreamBuffer::operator<<(CIpAddress &ip)
{
	operator << (ip.dwReserved);

	if(ip.m_bIsIPV6)
		Write(ip.v6, sizeof(ip.v6));
	else
		Write(&ip.v4, sizeof(ip.v4));

	return *this;
}

CStreamBuffer& CStreamBuffer::operator>>(CIpAddress &ip)
{
	operator >> (ip.dwReserved);

	if(ip.m_bIsIPV6)
		Read(ip.v6, sizeof(ip.v6));
	else
		Read(&ip.v4, sizeof(ip.v4));

	return *this;
}

#endif


