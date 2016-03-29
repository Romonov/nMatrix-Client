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


#include "UTX.h"
#include "StreamBuffer.h"
#include "NetworkDataType.h"
#include "NamedPipe.h"


//#define DEFAULT_SERVER_IP    _T("192.168.1.102")
#define DEFAULT_SERVER_IP    _T("220.132.75.189")
#define WINDOWS_SERVICE_NAME _T("nMatrix VPN service")

#define MAX_NETWORK_CLIENT 256

#define INVALID_DM_INDEX 65535  // Must match value of the driver side.

#define MAX_HOST_NAME_LENGTH 64 // New version use UTF-8 to communicate with server. So length is in char count.

#define MAX_NET_NAME_LENGTH  64 // Must match value of the server side.
#define MAX_NET_PASSWORD_LEN 40

#define MAX_VNET_GROUP_COUNT 12
#define MAX_GROUP_NAME_LEN   40

#define MAX_BYTE_COUNT_FOR_VAR_INT 4

#define LOGIN_ID_LEN       31
#define LOGIN_PASSWORD_LEN 15

#define MAX_CHAT_STRING_LENGTH 400

#define MAX_GROUP_CHAT_SESSION 5
#define MAX_GROUP_CHAT_HOST    64
#define MAX_GROUP_CHAT_LENGTH  400 // Not include terminating null-character.

#define MASTER_SERVER_RELAY_ID 1

#define BANDWIDTH_UL_TIME_RATE 10 // Control how often to check data rate per second.
#define BANDWIDTH_DL_TIME_RATE 10

#define APP_VERSION(Main, Sub, BuildNum) (DWORD)(((Main & 0x3F) << 26) | ((Sub & 0xFF) << 18) | (BuildNum & 0x0003FFFF))
#define APP_BUILD_DATE(year, month, date, hour, minute) (DWORD)((((year - 2000) & 0x7F ) << 20) | ((month & 0x0F) << 16) | ((date & 0x1F) << 11) | ((hour & 0x1F) << 6) | (minute & 0x3F))


inline DWORD AppGetVersion()
{
	return APP_VERSION(0, 9, 580);
}

inline DWORD AppGetDriverResourceVersion()
{
	return APP_VERSION(0, 9, 565);
}


struct stVersionInfo // Structure for index 1.
{
	stVersionInfo() { ZeroInit(); VersionIndex = 1; Size = sizeof(stVersionInfo); }
	void ZeroInit() { ZeroMemory(this, sizeof(stVersionInfo)); }


	enum VERSION_FLAG
	{
		VF_NULL  = 0x0000,
		VF_ALPHA = 0x01,
		VF_BETA  = 0x01 << 1,
		VF_DEBUG = 0x01 << 2,

		VF_SERVICE = 0x01 << 13,
		VF_CUSTOM  = 0x01 << 14,
		VF_REGISTERED = 0x01 << 15
	};

	USHORT VersionIndex, Size;
	USHORT AppVerFlag, DriVerFlag;

	DWORD AppVer;
	DWORD AppBuildDate;
	DWORD DriVer;
	DWORD DriBuildDate;


};


inline DWORD AppGetBuildDate()
{
	INT year, month, date, hour, minute;
	ParseDateTime(__DATE__, __TIME__, year, month, date, hour, minute);
	return APP_BUILD_DATE(year, month, date, hour, minute);
}

inline void AppGetBuildDateDetail(DWORD BuildDate, INT &year, INT &month, INT &date, INT &hour, INT &minute)
{
	year = ((BuildDate >> 20) & 0x7F) + 2000; // Start from year 2000.
	month = (BuildDate >> 16) & 0x0F;
	date = (BuildDate >> 11) & 0x1F;
	hour = (BuildDate >> 6) & 0x1F;
	minute = BuildDate & 0x3F;
}

inline void AppGetVersionDetail(DWORD version, INT &main, INT &sub, INT &buildnum)
{
	main = (version >> 26);
	sub = (version >> 18) & 0xFF;
	buildnum = version & 0x0003FFFF;
}


struct stRegisterID
{
public:
	stRegisterID() { ZeroInit(); }

	void ZeroInit() { ZeroMemory(this, sizeof(stRegisterID)); };

	BOOL IsNull()
	{
		for(INT i = 0; i < 4; ++i)
			if(!d[i])
				return TRUE;
		return FALSE;
	}

	union
	{
		struct
		{
			DWORD d1, d2, d3, d4;
		};

		DWORD d[4];
	};

};


enum SERVER_CONTROL_FLAG
{
	SCF_RESERVED        = 0x01,
	SCF_ACCOUNT_LOGIN   = (0x01 << 1),
	SCF_RENAME_NETWORK  = (0x01 << 2),
	SCF_CLIENT_RELAY    = (0x01 << 3),
	SCF_SERVER_NEWS     = (0x01 << 4), // Force client to query server news.
	SCF_TPT_REPORT      = (0x01 << 5), // Report result of tunnel punch through.
	SCF_TPT_REPORT_OAS  = (0x01 << 6), // Only active side needs to report.
	SCF_FMAC_INDEX      = (0x01 << 7), // Enable client to use mac as fast index.
	SCF_SERVER_REALY    = (0x01 << 8),

	SCF_ALL_FLAG = 0xFFFFFFFF
};

enum COMMON_SERVER_RETURN_CODE
{
	CSRD_FAILED,

	CSRC_SUCCESS,
	CSRC_NOTIFY,
	CSRC_ADM_OPERATION,

	CSRC_UNKNOW_ERROR = 200,
	CSRC_ACCESS_DENIED,
	CSRC_INVALID_PARAM,

	CSRC_OBJECT_NOT_FOUND,
	CSRC_OBJECT_ALREADY_EXIST,
	CSRC_PASSWORD_ERROR,
	CSRC_NETWORK_LOCKED,
	CSRC_ALREADY_JOINED,
	CSRC_INVALID_OPERATION,

};

enum NAT_TYPE
{
	NT_OFFLINE,
	NT_PHYSICAL_IP,
	NT_FULL_CONE,
	NT_ADDRESS_RESTRICTED_CONE,
	NT_PORT_RESTRICTED_CONE,
	NT_SYMMETRIC
};

enum CLIENT_REPORT_TYPE
{
	CRT_BUG,
	CRT_TPT_RESULT,

	CRT_DWORD = 0xFFFFFFFF
};

enum CLIENT_PROFILE_TYPE
{
	CPT_NULL,
	CPT_HOST_NAME,

};

enum CLIENT_BUG_CODE
{
	CBC_PTSetInitData = 20,
	CBC_PTTimeOrderError,

	CBC_DuplicateDMIndex,
};


enum VNET_FLAG // Must match definition of server side (16 bits only).
{
	VF_IS_ONLINE     = 0x01,
	VF_IS_OWNER      = 0x01 << 1,
	VF_HUB           = 0x01 << 2,
	VF_GATEWAY       = 0x01 << 2,
	VF_RELAY_HOST    = 0x01 << 3,
	VF_ALLOW_RELAY   = 0x01 << 4,

	// The following flags are for virtual network.
	VF_NEED_PASSWORD = 0x01 << 8,
	VF_DISALLOW_USER = 0x01 << 9

};


enum VIRTUAL_NETWORK_TYPE
{
	VNT_RESERVED = 0,
	VNT_MESH,
	VNT_HUB_AND_SPOKE,
	VNT_GATEWAY,
};


enum JOB_SETTING_TYPE
{
	JST_RESERVED = 0,
	JST_NET_PASSWORD,
	JST_NET_FLAG,
	JST_HOST_ROLE_HUB,
	JST_HOST_ROLE_RELAY,

	// Client only setting flag.
	JST_CLOSED_SERVER_MSG_ID = 0x01 << 20,

	JST_SET_DATA = 0x01 << 31
};


enum GROUP_CHAT_REQUEST_TYPE
{
	GCRT_NULL,
	GCRT_REQUEST,
	GCRT_INVITE,
	GCRT_LEAVE,
	GCRT_CLOSE,
	GCRT_EVICT,
	GCRT_CHAT,
};

enum GROUP_CHAT_CMD_STATUS_CODE
{
	GCCSC_ERROR = 0,
	GCCSC_OK,
	GCCSC_NOTIFY,
	GCCSC_RESUME,

	GCCSC_INVALID_SESSION,

};

enum VNET_GROUP_FLAG
{
	VGF_DEFAULT_GROUP  = 0x01,
	VGF_STATE_OFFLINE  = 0x01 << 1,
	VNET_ALLOW_OFFLINE = 0x01 << 2,

};

enum SUBNET_SUBGROUP_CMD_TYPE
{
	SSCT_NULL,
	SSCT_CREATE,
	SSCT_DELETE,
	SSCT_MOVE,
	SSCT_OFFLINE,
	SSCT_SET_FLAG,
	SSCT_RENAME,
	SSCT_EXT,

};

enum CLIENT_QUERY_TYPE
{
	CQT_SERVER_NEWS = 1,
	CQT_HOST_LAST_ONLINE_TIME,
	CQT_VNET_MEMBER_INFO,
};

enum REQUEST_RELAY_TYPE
{
	RRT_CLIENT_REQUEST,
	RRT_RELAY_HOST_ACK,
	RRT_CANCEL_RELAY,
	RRT_REQUEST_SERVER,
	RRT_REQUEST_SERVER_ACK,
};

enum CORE_EVENT_INDEX
{
	CEI_JOB_QUEUE_EVENT_INDEX,
	CEI_CORE_TIMER_EVENT_INDEX,
	CEI_SERVER_SOCKET_EVENT_INDEX,
	CEI_IPC_SOCKET_EVENT_INDEX,

	CEI_TOTAL_EVENT,
};


struct stQueryTimeStruct
{
	LARGE_INTEGER ServerTime;
	LARGE_INTEGER ServerFreq;
	LARGE_INTEGER ClientTime;
};

struct stVNetMemberInfo
{
	DWORD uid, flag;
	DWORD IP, dwReserved;
	time_t Time;
};

struct stVNetGroup
{
	uChar GroupName[MAX_GROUP_NAME_LEN + 1];
	UINT  nGUIOrder, nMaxHost;
	DWORD Flag;
};

struct stGUIObjectHeader
{
	stGUIObjectHeader(DWORD dwSize)
	:dwObjectSize(dwSize)
	{
	//	printx("Object size: %d bytes\n", dwSize);
	}

	const DWORD dwObjectSize;
};

struct stGUIVLanInfo;

struct stGUISubgroup : public stGUIObjectHeader
{
	stGUISubgroup()
	:stGUIObjectHeader(sizeof(*this))
	{
		ZeroMemory(&VNetGroup, sizeof(VNetGroup));
	}

	stVNetGroup VNetGroup;
	stGUIVLanInfo *pVNetInfo;
	void* GUIHandle, *DummyData; // Avoid size confliction.
};

struct stGUIVLanMember : public stGUIObjectHeader
{
	stGUIVLanMember()
	:stGUIObjectHeader(sizeof(*this))
	{
	}


	UINT ReadFromStream(CStreamBuffer &sb);
	UINT WriteStream(CStreamBuffer *psb);

	BOOL IsNetOnline() { return Flag & VF_IS_ONLINE; }
	void UpdataOnlineInfo(const stGUIVLanMember &In)
	{
		ASSERT(HostName == In.HostName && dwUserID == In.dwUserID && vip == In.vip);

		LinkState = In.LinkState;
		DriverMapIndex = In.DriverMapIndex;
		eip = In.eip;
	}
	stGUIVLanMember& operator=(const stGUIVLanMember &In)
	{
		HostName = In.HostName;
		dwUserID = In.dwUserID;
		RelayVNetID = In.RelayVNetID;
		vip = In.vip;
		LinkState = In.LinkState;
		memcpy(vmac, In.vmac, sizeof(vmac));

		DriverMapIndex = In.DriverMapIndex;
		eip = In.eip;
		Flag = In.Flag;
		dwGroupBitMask = In.dwGroupBitMask;
		GroupIndex = In.GroupIndex;
		GUIHandle = In.GUIHandle;

		return *this;
	}

	BOOL UpdateGroupBitMask(UINT nGroupIndex, BOOL bOnline)
	{
		DWORD dwMask = 0x01 << nGroupIndex;

		if( dwMask & dwGroupBitMask )
		{
			if(bOnline)
				return FALSE;
			dwGroupBitMask &= ~dwMask;
		}
		else
		{
			if(!bOnline)
				return FALSE;
			dwGroupBitMask |= dwMask;
		}

		return TRUE;
	}

	CString HostName;
	DWORD   dwUserID, RelayVNetID; // Only valid if LinkState is LS_RELAYED_TUNNEL.
	DWORD   vip;
	CIpAddress eip;
	BYTE    vmac[6];
	USHORT  Flag, DriverMapIndex, LinkState;
	DWORD   dwGroupBitMask;
	BYTE    GroupIndex;
	void*   GUIHandle;


};

struct stGUIVLanInfo : public stGUIObjectHeader
{
	stGUIVLanInfo()
	:stGUIObjectHeader(sizeof(*this)), nOnline(0)
	{
	}


	UINT ReadFromStream(CStreamBuffer &sb);
	void UpdataToolTipString();
	BOOL IsOwner() { return Flag & VF_IS_OWNER; }
	BOOL IsNetOnline() { return Flag & VF_IS_ONLINE; }
	void UpdateFlag(USHORT FlagIn)
	{
		USHORT mask = VF_NEED_PASSWORD | VF_DISALLOW_USER; // Changeable data only.
		Flag &= ~mask; // Clean bits.
		Flag |= (mask & FlagIn);
	}
	stGUIVLanMember* AddVLanMember(stGUIVLanMember *pMemberIn)
	{
		stGUIVLanMember* pMember = new stGUIVLanMember;
		if(pMember)
		{
			if(pMemberIn)
				*pMember = *pMemberIn;
			pMember->GroupIndex = FindDefaultGroupIndex();
			MemberList.AddTail(pMember);
		}
		return pMember;
	}
	stGUIVLanMember* FindMember(DWORD uid)
	{
		stGUIVLanMember *pMember;
		for(POSITION pos = MemberList.GetHeadPosition(); pos; )
		{
			pMember = MemberList.GetNext(pos);
			if(pMember->dwUserID == uid)
				return pMember;
		}
		return NULL;
	}
	void* GetMemberGUIParentHandle(stGUIVLanMember *pMember)
	{
		if(m_nTotalGroup >= 2)
		{
			ASSERT(pMember->GroupIndex < m_nTotalGroup);
			return m_GroupArray[pMember->GroupIndex].GUIHandle;
		}
		return GUIHandle;
	}

	UINT FindDefaultGroupIndex()
	{
		for(UINT i = 0; i < m_nTotalGroup; ++i)
			if(m_GroupArray[i].VNetGroup.Flag & VGF_DEFAULT_GROUP)
				return i;
		return 0;
	}
	UINT CreateGroup(UINT nIndex, stVNetGroup *pGroup, UINT nSize)
	{
		ASSERT(nIndex < MAX_VNET_GROUP_COUNT && nIndex);
		ASSERT(nSize == sizeof(stVNetGroup));

		m_GroupArray[nIndex].VNetGroup = *pGroup;
		m_GroupArray[nIndex].pVNetInfo = this;
		++m_nTotalGroup;

		return m_nTotalGroup - 1; // Return index.
	}
	BOOL DeleteGroup(UINT nIndex, void** pGUIHandle)
	{
		if(nIndex >= m_nTotalGroup || !nIndex)
			return FALSE;

		*pGUIHandle = m_GroupArray[nIndex].GUIHandle;

		if(m_GroupArray[nIndex].VNetGroup.Flag & VGF_DEFAULT_GROUP)
			m_GroupArray[0].VNetGroup.Flag |= VGF_DEFAULT_GROUP;
		if(nIndex < m_nTotalGroup - 1) // Make sure move data is necessary.
			memcpy(&m_GroupArray[nIndex], &m_GroupArray[nIndex + 1], (m_nTotalGroup - nIndex - 1) * sizeof(stGUISubgroup));
		--m_nTotalGroup;

		UINT DefaultGroupIndex = FindDefaultGroupIndex();
		DWORD dwMask = (0x01 << nIndex) - 1, dwReservedBits;

		for(POSITION pos = MemberList.GetHeadPosition(); pos; )
		{
			stGUIVLanMember *pMember = MemberList.GetNext(pos);

			if(pMember->GroupIndex == nIndex)
				pMember->GroupIndex = DefaultGroupIndex;
			else if(pMember->GroupIndex > nIndex)
				pMember->GroupIndex--;

			dwReservedBits = pMember->dwGroupBitMask & dwMask;
			pMember->dwGroupBitMask = ((pMember->dwGroupBitMask >> 1) & ~dwMask) | dwReservedBits;
			pMember->dwGroupBitMask |= 0x80000000; // Reset unused bits to 1.
		}

		if(GroupIndex == nIndex) // Check local host.
			GroupIndex = DefaultGroupIndex;
		else if(GroupIndex > nIndex)
			GroupIndex--;

		dwReservedBits = dwGroupBitMask & dwMask;
		dwGroupBitMask = ((dwGroupBitMask >> 1) & ~dwMask) | dwReservedBits;
		dwGroupBitMask |= 0x80000000; // Reset unused bits to 1.

		return TRUE;
	}
	UINT GetGroupIndex(stGUISubgroup *pGroup)
	{
		for(UINT i = 0; i < _countof(m_GroupArray); ++i)
			if(pGroup == &m_GroupArray[i])
				return i;
		return MAX_VNET_GROUP_COUNT;
	}
	void InitDefaultGroup(const TCHAR *pGroupName)
	{
		memcpy(m_GroupArray[0].VNetGroup.GroupName, pGroupName, (_tcslen(pGroupName) + 1) * sizeof(TCHAR));
	}
	stGUIVLanMember* UpdateMemberGroupIndex(DWORD uid, UINT nNewGroupIndex, UINT &nOldGroupIndex)
	{
		stGUIVLanMember *pMemberOut = 0;

		for(POSITION pos = MemberList.GetHeadPosition(); pos; )
		{
			stGUIVLanMember *pMember = MemberList.GetNext(pos);
			if(pMember->dwUserID != uid)
				continue;
			nOldGroupIndex = pMember->GroupIndex;
			pMember->GroupIndex = nNewGroupIndex;
			pMemberOut = pMember;
			break;
		}

		return pMemberOut;
	}
	BOOL SetDefaultGroup(UINT nIndex)
	{
		if(nIndex >= m_nTotalGroup)
			return FALSE;

		for(UINT i = 0; i < m_nTotalGroup; ++i)
			if(i != nIndex)
				m_GroupArray[i].VNetGroup.Flag &= ~VGF_DEFAULT_GROUP;
			else
				m_GroupArray[i].VNetGroup.Flag |= VGF_DEFAULT_GROUP;

		return TRUE;
	}
	UINT GetGroupMemberCount(UINT nGroupIndex)
	{
		UINT nCount = 0;

		for(POSITION pos = MemberList.GetHeadPosition(); pos; )
			if(MemberList.GetNext(pos)->GroupIndex == nGroupIndex)
				nCount++;

		return nCount;
	}
	BOOL CanShowGroupMoveMenu()
	{
		if(m_nTotalGroup == 2 && CanHideDefaultGroup())
			return FALSE;
		if(m_nTotalGroup > 1)
			return TRUE;
		return FALSE;
	}
	BOOL UpdateGroupName(UINT nIndex, TCHAR *pNewName, UINT nLength)
	{
		if(nIndex >= m_nTotalGroup)
			return FALSE;
		memcpy(m_GroupArray[nIndex].VNetGroup.GroupName, pNewName, sizeof(TCHAR) * nLength);
		m_GroupArray[nIndex].VNetGroup.GroupName[nLength] = 0;
		return TRUE;
	}
	BOOL UpdateGroupFlag(UINT nIndex, DWORD dwMask, DWORD dwFlag)
	{
		if(nIndex >= m_nTotalGroup /*|| !nIndex*/)
			return FALSE;

		m_GroupArray[nIndex].VNetGroup.Flag &= ~dwMask;
		m_GroupArray[nIndex].VNetGroup.Flag |= (dwMask & dwFlag);

		return TRUE;
	}
	BOOL IsSubgroupOnline(UINT nIndex)
	{
		if(nIndex >= m_nTotalGroup /*|| !nIndex*/)
			return FALSE;
		return (0x01 << nIndex) & dwGroupBitMask;
	}
	BOOL IsSubgroupOnline(stGUISubgroup *pGroup)
	{
		return IsSubgroupOnline(GetGroupIndex(pGroup));
	}

	BOOL UpdateGroupBitMask(UINT nGroupIndex, BOOL bOnline)
	{
		DWORD dwMask = 0x01 << nGroupIndex;

		if( dwMask & dwGroupBitMask )
		{
			if(bOnline)
				return FALSE;
			dwGroupBitMask &= ~dwMask;
		}
		else
		{
			if(!bOnline)
				return FALSE;
			dwGroupBitMask |= dwMask;
		}

		return TRUE;
	}

	BOOL CanHideDefaultGroup() { return (FindDefaultGroupIndex() != 0 && GetGroupMemberCount(0) == 0 && GroupIndex); }


	DWORD_PTR  NetIDCode;
	CString    NetName, ToolTipInfo;
	USHORT     NetworkType, Flag, nOnline;
	void*      GUIHandle;
	UINT       m_nTotalGroup;
	DWORD      dwGroupBitMask;
	BYTE       GroupIndex;

	CList<stGUIVLanMember*> MemberList;
	stGUISubgroup m_GroupArray[MAX_VNET_GROUP_COUNT]; // Index zero is the default group.


private:

	stGUIVLanInfo& operator=(const stGUIVLanInfo &In);


};


struct stConfigData
{
	stConfigData() { ZeroMemory(this, sizeof(stConfigData)); }

	TCHAR LocalName[MAX_HOST_NAME_LENGTH + 1];
	DWORD dwModFlag, UDPTunnelAddress;

	UINT LanguageID;
	UINT DataOutLimit, DataInLimit, KeepTunnelTime;
	USHORT UDPTunnelPort;
	bool bAutoStart, bAutoReconnect, bAutoUpdate, bAutoSetFirewall;


};


struct stGUIEventMsg
{
public:
	stGUIEventMsg(DWORD UsageFlagIn = 0)
	:pHeapData(0), HeapDataSize(0)
	{
		UsageFlag = UsageFlagIn;
	}
	~stGUIEventMsg() { ASSERT(!pHeapData && !HeapDataSize); }


	enum USAGE_FLAGS
	{
		UF_NULL = 0,
		UF_STRING = 0x01,
		UF_MEMBER = 0x01 << 1,
		UF_DWORD1 = 0x01 << 2,
		UF_DWORD2 = 0x01 << 3,
		UF_DWORD3 = 0x01 << 4,
		UF_DWORD4 = 0x01 << 5,
		UF_DWORD5 = 0x01 << 6,
		UF_HEAP   = 0x01 << 7
	};

	DWORD UsageFlag; // Indicate which data is useful.
	CString string;
	stGUIVLanMember member;
	union
	{
		DWORD DWORD_1;
		DWORD dwResult;
	};
	DWORD DWORD_2, DWORD_3, DWORD_4, DWORD_5;

	void* Heap(UINT size, void *pSrc = NULL)
	{
		ASSERT(!pHeapData);
		UsageFlag |= UF_HEAP;
		HeapDataSize = size;
		pHeapData = malloc(size);
		if(pHeapData && pSrc)
			memcpy(pHeapData, pSrc, size);
		return pHeapData;
	}
	void DeleteHeap()
	{
		if(pHeapData)
		{
			ASSERT(UsageFlag & UF_HEAP);
			UsageFlag &= ~UF_HEAP;
			free(pHeapData);
			pHeapData = 0;
			HeapDataSize = 0;
		}
	}
	void* GetHeapData() { return pHeapData; }
	UINT  GetHeapDataSize() { return HeapDataSize; }

	UINT ReadFromStream(CStreamBuffer &sb); // For service mode.
	UINT WriteStream(CStreamBuffer *psb);

	DWORD FlagOr(DWORD flags) { UsageFlag |= flags; return UsageFlag; }


protected:
	void *pHeapData;
	UINT HeapDataSize;


};


// LS_NO_CONNECTION has two meaning. DriverMapIndex == INVALID_DM_INDEX means it's online but in offline mode of the vnet. DriverMapIndex != INVALID_DM_INDEX means no tunnel between two host.
enum LINK_STATE
{
	LS_OFFLINE,        // Gray.
	LS_NO_CONNECTION,  // Red.

	LS_NO_TUNNEL,      // Red.
	LS_TRYING_TPT,     // Red.

	LS_RELAYED_TUNNEL, // Above this value tunnel is available.
	LS_SERVER_RELAYED, // Yellow.
	LS_CONNECTED,      // Green.

	LS_TOTAL
};


enum CORE_SERVICE_ERROR_CODE
{
	CSEC_NULL,
	CSEC_TUNNEL_ERROR,
};


enum GUI_EVENT_TYPE
{
	GET_UPDATE_NET_LIST = 1,
	GET_ADD_USER,
	GET_HOST_ONLINE,
	GET_HOST_EXIT_NET,
	GET_DELETE_NET_RESULT,
	GET_EXIT_NET_RESULT,
	GET_OFFLINE_SUBNET,
	GET_OFFLINE_SUBNET_PEER,
	GET_UPDATE_CONNECT_STATE,
	GET_JOIN_NET_RESULT,
	GET_CREATE_NET_RESULT,
	GET_RELAY_EVENT,
	GET_REGISTER_EVENT,    // For register ID.
	GET_REGISTER_RESULT,   // For account login.
	GET_LOGIN_EVENT,
	GET_UPDATE_MEMBER_STATE,
	GET_UPDATE_TRAFFIC_INFO,
	GET_DATA_EXCHANGE,
	GET_ON_RECEIVE_CHAT_TEXT,
	GET_SETTING_RESPONSE,
	GET_CLIENT_QUERY,
	GET_UPDATE_ADAPTER_CONFIG,
	GET_CORE_ERROR,
	GET_RELAY_INFO,
	GET_SERVICE_VERSION,
	GET_SERVICE_STATE,
	GET_PING_HOST,
	GET_GROUP_CHAT,
	GET_SUBNET_SUBGROUP,
	GET_CLIENT_PROFILE,

	GET_READY
};

enum LOGOUT_REASON
{
	LOR_NULL,
	LOR_GUI,
	LOR_SYSTEM_POWER_EVENT,
	LOR_DISCONNECTED,
	LOR_QUERY_ADDRESS_FAILED,
	LOR_ERROR,
};

enum SERVICE_STATE_CODE
{
	SSC_TERMINATED,
	SSC_DELETE_READY,
};

enum LOGIN_RESULT_CODE
{
	LRC_LOGIN_SUCCESS,
	LRC_NAME_NOT_FOUND,
	LRC_PASSWORD_ERROR,
	LRC_REG_ID_NOT_FOUND,
	LRC_UNSUPPORTED_VERSION,
	LRC_SERVER_REJECTED, // Above is the same with server side.

	LRC_CONNECTING,
	LRC_ADAPTER_ERROR,
	LRC_SOCKET_ERROR,
	LRC_UDP_PORT_ERROR,
	LRC_CONNECT_FAILED,
	LRC_THREAD_FAILED,
	LRC_HAS_LONIN,
	LRC_LOGIN_CODE, // Above is result code for login.

	LRC_LOGOUT_SUCCESS,
	LRC_LOGOUT_DISCONNECT,
	LRC_LOGOUT_NOT_LOGIN,
	LRC_QUERY_ADDRESS_FAILED,
	LRC_LOGOUT_CORE_ERROR,
};


enum DATA_EXCHANGE_TYPE
{
	DET_LOGIN_INFO,
	DET_REG_ID,
	DET_CONFIG_DATA,
	DET_SERVER_MSG_ID,
};


struct CMessagePipe : public IPipeDataDest
{
public:

	CMessagePipe()
	:m_bConnected(FALSE), m_NamedPipe(this)
	{
	}
	virtual ~CMessagePipe()
	{
	}

	// Interface.
	void OnConnectingPipe();
	void OnDisConnectingPipe(CPipeListener* pReader);
	void OnIncomingData(const BYTE *pData, DWORD nSize);

	BOOL IsConnected() { return m_bConnected; }
	BOOL WriteMsg(const BYTE *pData, UINT nCount) { return m_NamedPipe.WriteMsg(pData, nCount); }
	CPipeListener* GetPipeObject() { return &m_NamedPipe; }


protected:
	BOOL m_bConnected;
	CPipeListener m_NamedPipe;


};


//#define USE_PLATFORM_TIMER


class CnMatrixCore
{
public:

	CnMatrixCore()
	:m_flags(0), m_hWorkerThread(NULL), m_pPipe(NULL), m_hTimer(NULL), m_hTimerEvent(NULL)
	{
		UTXLibraryInit();

		ASSERT(pnMatrixCore == NULL);
		pnMatrixCore = this;
	}
	~CnMatrixCore()
	{
		UTXLibraryEnd();

		ASSERT(!(m_flags & CF_RUNNING) && m_hWorkerThread == NULL && m_pPipe == NULL);
		ASSERT(pnMatrixCore == this);
		pnMatrixCore = NULL;
	}

	enum CORE_FLAG
	{
		CF_SERVICE_MODE = 0x00000001,
		CF_PURE_GUI     = 0x00000002,
		CF_RUNNING      = 0x00000004,
		CF_GUI_READY    = 0x00000008,
	};


	BOOL Init(HWND hWnd, CMessagePipe *pPipe = NULL, BOOL bForceGUIMode = FALSE);
	BOOL Close();
	void Run();

	BOOL CoreCreateTimer(HANDLE *pWaitEvent); // Abstraction layer for different timer implements.
	void CoreCloseTimer();
	void CoreStartTimer(INT iFirstTime);
	void CoreStopTimer();

	BOOL IsServiceMode() { return m_flags & CF_SERVICE_MODE; }
	BOOL IsPureGUIMode() { return m_flags &  CF_PURE_GUI; }
	BOOL IsRunning() { return m_flags & CF_RUNNING; }
	BOOL IsGUIReady() { return m_flags & CF_GUI_READY; }
	void SetGUIFlag(BOOL bAttach) { if(bAttach) m_flags |= CF_GUI_READY; else m_flags &= ~CF_GUI_READY; }

	void EnableServicePipe() { ASSERT(m_pPipe && IsServiceMode()); m_pPipe->GetPipeObject()->StartReader(); }
	BOOL IsPipeConnected() { return m_pPipe ? m_pPipe->IsConnected() : FALSE; }
	BOOL PipeSend(BYTE *pData, UINT Size) { if(m_pPipe && m_pPipe->IsConnected()) return m_pPipe->WriteMsg(pData, Size); return FALSE; }

	static DWORD WINAPI KernelReadThread(LPVOID lpParameter);
	static DWORD WINAPI KernelWriteThread(LPVOID lpParameter);

	static DWORD WINAPI KernelReadThreadEx(LPVOID lpParameter);
	static DWORD WINAPI KernelWriteThreadEx(LPVOID lpParameter);


protected:

	volatile DWORD m_flags;
	HANDLE   m_hWorkerThread, m_hReadThread;
	CMessagePipe *m_pPipe;

	HANDLE m_hTimer, m_hTimerEvent;

	friend CnMatrixCore *AppGetnMatrixCore();
	static CnMatrixCore *pnMatrixCore;
	static HWND hGUIHandle;
	static DWORD WINAPI WorkerThread(LPVOID lpParameter);

	friend void NotifyGUIMessage(DWORD type, stGUIEventMsg *pMsg);
	friend void NotifyGUIMessage(DWORD type, DWORD dwMsg);


};


struct stMemberUpdateInfo // Data for update GUI member link state only.
{
	DWORD_PTR UserID;
	USHORT LinkState;
};


class CMemberUpdateList
{
public:

	void AddData(stMemberUpdateInfo &Info)
	{
		for(POSITION pos = m_list.GetHeadPosition(); pos;)
		{
			stMemberUpdateInfo &Member = m_list.GetNext(pos);
			if(Member.UserID != Info.UserID)
				continue;
			if(Member.LinkState != Info.LinkState)
				Member.LinkState = Info.LinkState;
			return;
		}

		m_list.AddHead(Info); // AddHead to get better performance when need to find duplication.
	}

	void AddData(DWORD UID, USHORT LinkState)
	{
		stMemberUpdateInfo Info = { UID, LinkState };
		AddData(Info);
	}

	void Remove(DWORD UID)
	{
		POSITION pos, oldpos;
		for(pos = m_list.GetHeadPosition(); pos;)
		{
			oldpos = pos;
			stMemberUpdateInfo &Member = m_list.GetNext(pos);
			if(Member.UserID != UID)
				continue;
			m_list.RemoveAt(oldpos);
			break;
		}
	}

	void WriteStream(CStreamBuffer &sb)
	{
		sb << (UINT)m_list.GetCount();
		POSITION pos;
		for(pos = m_list.GetHeadPosition(); pos;)
		{
			stMemberUpdateInfo &info = m_list.GetNext(pos);
			sb.Write(&info, sizeof(info));
		}
		m_list.RemoveAll();
	}

	UINT GetStreamSize() { return sizeof(UINT) + m_list.GetCount() * sizeof(stMemberUpdateInfo); }
	UINT GetDataCount()	{ return (UINT)m_list.GetCount(); }
	void Release() { m_list.RemoveAll(); }


protected:
	CList<stMemberUpdateInfo> m_list;


};


inline CnMatrixCore* AppGetnMatrixCore()
{
	return CnMatrixCore::pnMatrixCore;
}


inline USHORT AppGetVersionFlag()
{
	USHORT flag = stVersionInfo::VF_BETA;
	if(AppGetnMatrixCore()->IsServiceMode())
		flag |= stVersionInfo::VF_SERVICE;

#ifdef _DEBUG
	flag |= stVersionInfo::VF_DEBUG;
#endif

	return flag;
}


enum APP_STRING_INDEX
{
	ASI_NULL,

	ASI_GUI,
	ASI_top,
	ASI_bottom,
	ASI_left,
	ASI_right,
	ASI_IPV4_FORMAT,
	ASI_PIPE_NAME,
	ASI_SINGLE_INSTANCE_MUTEX_NAME,

	ASI_TOTAL,
};

const TCHAR* APP_STRING(APP_STRING_INDEX index);


// All AddJobxxx functions are interfaces between gui and core service.
// Don't use core function directly in the interfaces.
BOOL AddJobRegister(const TCHAR *pName, const TCHAR *pPassword);
BOOL AddJobLogin(UINT nLogoutReason, const TCHAR *pUserName, const TCHAR *pPassword, BOOL bCloseTunnel);
void AddJobDetectNatFirewall();
void AddJobRetrieveNetList(DWORD NetIDCode, const TCHAR *pNetName);

void AddJobCreateJoinSubnet(const TCHAR *pNetName, const TCHAR *pPassword, BOOL bCreate, USHORT NetworkType);
void AddJobExitSubnet(DWORD NetID, DWORD UID);
void AddJobDeleteSubNet(DWORD NetID, DWORD UID);
void AddJobRemoveUser(DWORD NetID, DWORD UserID);
void AddJobOfflineSubNet(BOOL bOnline, DWORD UID, DWORD NetID);
BOOL AddJobTextChat(const TCHAR *pText, DWORD dwHostUID, DWORD dwPeerUID);
void AddJobResetNetworkPassword(DWORD NetID, const TCHAR *NetName, const TCHAR *Password);
void AddJobResetNetworkFlag(DWORD NetID, const TCHAR *NetName, DWORD dwFlag, DWORD dwMask);
void AddJobSetHostRole(DWORD NetID, const TCHAR *NetName, DWORD UID, DWORD dwFlag, DWORD dwMask);
void AddJobQueryServerNews();
void AddJobQueryHostLastOnlineTime(DWORD NetID, DWORD UID);
void AddJobQueryVNetMemberInfo(DWORD NetID);
void AddJobReportBug(DWORD BugID, void *pRefData = 0, UINT nDataSize = 0);
void AddJobQueryServerTime(BOOL bTest = TRUE);
void AddJobRequestRelay(DWORD dwRequestType, DWORD NetID, DWORD DestUID, DWORD RelayHostUID);
void AddJobServerRelay(DWORD uid, void *pData, UINT nDataLen);

void AddJobSystemPowerEvent(BOOL bResume);
void AddJobReadTraffic(); // Client job only.
UINT AddJobDataExchange(BOOL bRead, DWORD DataTypeEnum, void *pWriteValue, BOOL bPackDataOnly = FALSE, void *pBuffer = 0, UINT nBufferSize = 0);
void AddJobEnableUMAccess(BOOL bEnable, BOOL bUserModeBuffer);
UINT AddJobUpdateConfig(stConfigData *pConfigData, BOOL bRead, BOOL bPackDataOnly = FALSE, void *pBuffer = 0, UINT nBufferSize = 0);
void AddJobUpdateHostName(DWORD UID, const TCHAR *pNewName, UINT nLen);
void AddJobPingHost(DWORD uid, UINT nID);
void AddJobGUIPipeEvent(BOOL bAttach);
void AddJobCloseMsg(DWORD dwMsgID);
void AddJobServiceState(BOOL bQueryVersion = FALSE);
void AddJobDebugFunction(DWORD type, DWORD value1, DWORD value2);

void AddJobGroupChatSetup(DWORD HostUID, DWORD VNetID, UINT nUserCount, DWORD *UIDArray);
void AddJobGroupChatInvite(DWORD VNetID, UINT nSessionID, UINT nUserCount, DWORD *UIDArray);
void AddJobGroupChatLeave(DWORD HostUID, UINT nSessionID);
void AddJobGroupChatClose(DWORD HostUID, UINT nSessionID);
void AddJobGroupChatEvict(DWORD DestUID, UINT nSessionID);
BOOL AddJobGroupChatSend(DWORD HostUID, UINT nSessionID, const TCHAR *pString, UINT nLength);

void AddJobCreateGroup(DWORD VNetID, const TCHAR *pNameString, UINT nLength);
void AddJobDeleteGroup(DWORD VNetID, UINT nIndex);
void AddJobMoveMember(DWORD VNetID, DWORD UserID, UINT nNewIndex);
void AddJobSetGroupFlag(DWORD VNetID, UINT nGroupIndex, DWORD dwMask, DWORD dwFlag);
void AddJobRenameGroup(DWORD VNetID, UINT nGroupIndex, const TCHAR *pNewName, UINT nLength);
void AddJobSubgroupOffline(DWORD UID, DWORD VNetID, UINT nGroupIndex, BOOL bOnline);
void AddJobDebugTest();


BOOL uCharToCString(uChar* const pStrIn, INT iLen, CString &out);
INT  CheckUTF8Length(const TCHAR *str, INT iStrLen);


// GUI methods.
void NotifyGUIMessage(DWORD type, stGUIEventMsg *pMsg);
void NotifyGUIMessage(DWORD type, DWORD dwMsg);

stGUIEventMsg* AllocGUIEventMsg(DWORD UsageFlag = 0);
void ReleaseGUIEventMsg(stGUIEventMsg *pMsg);


