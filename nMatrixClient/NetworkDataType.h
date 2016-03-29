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


#ifndef __NETWORK_DATA_TYPE_H__
#define __NETWORK_DATA_TYPE_H__


#define IPB1(d) (((d) & 0x000000FF))
#define IPB2(d) (((d) & 0x0000FF00) >> 8)
#define IPB3(d) (((d) & 0x00FF0000) >> 16)
#define IPB4(d) (((d) & 0xFF000000) >> 24)

// For platform using little-endian.
#define MKIP(a, b, c, d) ( (0xFF & (a)) | ((0xFF & (b)) << 8) | ((0xFF & (c)) << 16) | ((0xFF & (d)) << 24) )
#define IP(a, b, c, d)   ( ((a) & 0xFF) | (((b) & 0xFF) << 8) | (((c) & 0xFF) << 16) | (((d) & 0xFF) << 24) )


typedef struct _IPV4
{
	union
	{
		DWORD ip;

		struct
		{
			BYTE b1;
			BYTE b2;
			BYTE b3;
			BYTE b4;
		};
	};

	_IPV4() {}
	_IPV4(DWORD dw) { ip = dw; }

	void SwapByteOrder()
	{
		IPV4 temp = ip;
		b1 = temp.b4;
		b2 = temp.b3;
		b3 = temp.b2;
		b4 = temp.b1;
	}

	DWORD operator=(DWORD i) { ip = i; return i; }
	operator DWORD() const { return ip; }

} IPV4;


typedef struct _IpAddress
{
	union
	{
		BYTE  IPV4[4];
		DWORD v4;
		BYTE  IPV6[16];
		DWORD v6[4];
	};

	BOOL IsZeroAddress()
	{
		for (INT i = 0; i < 4; ++i)
			if (v6[i] != 0)
				return FALSE;
		return TRUE;
	}

	BOOL operator==(_IpAddress &in) { return !memcmp(this, &in, sizeof(_IpAddress)); }
	BOOL operator!=(_IpAddress &in) { return memcmp(this, &in, sizeof(_IpAddress)); }

} IpAddress;


class CIpAddress : public IpAddress // The size of this class is 20 bytes.
{
public:

	inline CIpAddress() { ZeroInit(); }
	inline CIpAddress(DWORD ipv4, USHORT usPort = 0) { m_bIsIPV6 = FALSE; v4 = ipv4; m_port = usPort; }

	inline BOOL IsIPV6() const { return m_bIsIPV6; }
	inline void SetIPV6(BOOL bIsIPV6) { m_bIsIPV6 = bIsIPV6; }
	inline void ZeroInit() { memset(this, 0, sizeof(*this)); }

	CString GetString()
	{
		CString ip;
		if (m_bIsIPV6)
		{
		}
		else
		{
			ip.Format(_T("%d.%d.%d.%d"), IPV4[0], IPV4[1], IPV4[2], IPV4[3]);
		}
		return ip;
	}

	UINT GetStreamingSize() const // Return min size for CStreamBuffer operation.
	{
		if (m_bIsIPV6)
			return sizeof(v6) + sizeof(dwReserved);
		return sizeof(v4) + sizeof(dwReserved);
	}

	BOOL operator==(const CIpAddress &in) const // Compare IP address only. Exclude port number.
	{
		if(m_bIsIPV6)
		{
			if(!in.m_bIsIPV6)
				return FALSE;

			for(INT i = 0; i < 4; ++i)
				if(v6[i] != in.v6[i])
					return FALSE;
		}
		else
		{
			if(in.m_bIsIPV6 || v4 != in.v4)
				return FALSE;
		}
		return TRUE;
	}
	BOOL operator!=(const CIpAddress &in) const
	{
		return !(*this == in);
	}

	union
	{
		struct
		{
			USHORT m_bIsIPV6, m_port;
		};
		DWORD dwReserved;
	};


};


#endif // __NETWORK_DATA_TYPE_H__


