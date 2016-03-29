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


#include "NetworkDataType.h"


class CStreamBuffer
{
public:

	enum BUFFER_TYPE
	{
		BT_NONE = 0,
		BT_USER,      // Attach to user provided buffer.
		BT_HEAP,
	};

	CStreamBuffer()
	:m_pBuffer(NULL), m_BufferSize(0), m_pos(0), m_flag(BT_NONE)
	{
	}
	CStreamBuffer(UINT BufferSize)
	:m_pBuffer(NULL), m_BufferSize(0), m_pos(0), m_flag(BT_NONE)
	{
		AllocateBuffer(BufferSize);
	}
	~CStreamBuffer()
	{
		if (m_flag == BT_HEAP && m_pBuffer != NULL)
			free(m_pBuffer);
	}


	BOOL AllocateBuffer(UINT size);
	void Release();
	UINT GetDataSize() { return m_pos; }
	UINT GetBufferSize() { return m_BufferSize; }
	UINT GetRemainingSize() { return m_BufferSize - m_pos; }
	void SetPos(UINT pos) { m_pos = pos; }
	void Skip(UINT SkipSize) { m_pos += SkipSize; }
	BYTE* GetBuffer() { return m_pBuffer; }
	BYTE* GetCurrentBuffer() { return m_pBuffer + m_pos; }

	void* AttachBuffer(void *pBuffer, UINT size)
	{
		ASSERT(m_flag == BT_NONE && m_pBuffer == NULL);
		m_flag = BT_USER; m_pBuffer = (BYTE*)pBuffer;
		m_BufferSize = size;
		m_pos = 0;
		return pBuffer;
	}
	void DetachBuffer()
	{
		ASSERT(m_flag == BT_USER && m_pos <= m_BufferSize);
		m_flag = BT_NONE;
		m_pBuffer = NULL;
		m_BufferSize = 0; // Keep data position m_pos.
	}


	// Store method.
	CStreamBuffer& operator<<(bool b);
	CStreamBuffer& operator<<(BYTE b);
	CStreamBuffer& operator<<(USHORT us);
	CStreamBuffer& operator<<(UINT ui);
	CStreamBuffer& operator<<(DWORD d);
	CStreamBuffer& operator<<(UINT64 ui);

	CStreamBuffer& operator<<(CHAR c);
	CStreamBuffer& operator<<(SHORT s);
	CStreamBuffer& operator<<(INT i);
	CStreamBuffer& operator<<(LONG l);
	CStreamBuffer& operator<<(INT64 i);

	CStreamBuffer& operator<<(FLOAT f);
	CStreamBuffer& operator<<(DOUBLE d);

	// Load method.
	CStreamBuffer& operator>>(bool &b);
	CStreamBuffer& operator>>(BYTE &b);
	CStreamBuffer& operator>>(USHORT &us);
	CStreamBuffer& operator>>(UINT &ui);
	CStreamBuffer& operator>>(DWORD &d);
	CStreamBuffer& operator>>(UINT64 &ui);

	CStreamBuffer& operator>>(CHAR &c);
	CStreamBuffer& operator>>(SHORT &s);
	CStreamBuffer& operator>>(INT &i);
	CStreamBuffer& operator>>(LONG &l);
	CStreamBuffer& operator>>(INT64 &i);

	CStreamBuffer& operator>>(FLOAT &f);
	CStreamBuffer& operator>>(DOUBLE &d);

	INT Read(void *pData, UINT size);
	INT Write(const void *pData, UINT size);
	INT BoolRead(BOOL b, void *pData, UINT size) { return b ? Read(pData, size) : 0; }
	INT BoolWrite(BOOL b, void *pData, UINT size) { return b ? Write(pData, size) : 0; }


	template<class T>
	__forceinline void TryRead(T &data) { memcpy(&data, &m_pBuffer[m_pos], sizeof(T)); }

	template <class T, class CharType>
	__forceinline INT ReadString(T &LenSize, UINT nMaxLen, CharType *pBuffer, UINT BufferSize) // BufferSize in bytes.
	{
		ASSERT(T(0) < T(-1)); // Make sure LenSize is unsigned.

		*this >> LenSize;
		if (nMaxLen && LenSize > nMaxLen)
			return 0;

		ASSERT((LenSize + 1) * sizeof(CharType) <= BufferSize);
		Read(pBuffer, LenSize * sizeof(CharType));
		pBuffer[LenSize] = 0;

		return LenSize; // Return character count only, not include null terminator.
	}

	template <class T, class CharType>
	__forceinline UINT WriteString(T nStrLen, const CharType *pString)
	{
		ASSERT(T(0) < T(-1)); // Make sure LenSize is unsigned.

		if(!nStrLen && pString != NULL)
			nStrLen = (T)std::char_traits<CharType>::length(pString);

		*this << nStrLen;
		if(nStrLen)
			Write(pString, sizeof(CharType) * nStrLen);

		ASSERT(GetDataSize() <= GetBufferSize());

		return sizeof(T) + sizeof(CharType) * nStrLen;
	}


#ifdef __NETWORK_DATA_TYPE_H__
	CStreamBuffer& operator<<(CIpAddress &ip);
	CStreamBuffer& operator>>(CIpAddress &ip);
#endif


protected:

	BYTE *m_pBuffer;
	UINT  m_BufferSize, m_pos;
	DWORD m_flag; // buffer type

	CStreamBuffer& operator=(CStreamBuffer &src); // The method is not allowed.


};


