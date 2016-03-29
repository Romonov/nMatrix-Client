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


#include "DriverAPI.h"
#include "SocketSP.h"
#include "CnMatrixCore.h"


#define NAT_PUNCH_THROUGH_TRY_COUNT 10
#define NAT_PUNCH_THROUGH_IIP       5   // Test for host in the same lan.

//#define MIN_PUNCH_TRY_COUNT 5      // Use udp to punch at least try MIN_PUNCH_TRY_COUNT count, preventing packet lost or order problem.

#define KEEP_TUNNEL_PERIOD      20 // In second. Because timer proc is called per one second, there is +-1000ms inaccuracy.
#define KEEP_TUNNEL_WITH_SERVER 25 // 2012.10.06 This is controlled by server now.


#define SERVER_VIRTUAL_IP IP(6, 0, 0, 1)
#define SERVER_UID 0

#define DEFAULT_TUNNEL_PORT 65000

#define MAX_HOST_PER_TIMESTAMP 10
#define FIRST_PUNCH_TIME_BASE  1000000000 // This value will be overflowed after about 11.574 days.

#define DEFAULT_ARP_TABLE_SIZE 100


struct stClientInfo;


enum TUNNEL_PUNCH_THROUGH_FLAG
{
	TPTF_NULL       = 0x0000,
	TPTF_ACKED      = 0x0001,
	TPTF_RECV_HELLO = 0x0002,
	TPTF_USE_IIP    = 0x0004, // Use internal address to punch through.
	TPTF_SAME_LAN   = 0x0008, // Has same external address.
	TPTF_MOD_ADDR   = 0x0010, // Address is updated.
	TPTF_F_PUNCH    = 0x0020, // First time punch completed.
	TPTF_MISS_TIME  = 0x0040, // First punch miss time stamp.
	TPTF_ACTIVE     = 0x0080, // The client reports TPT result if needs.
	TPTF_SRC_NAT    = 0x0100, // Source host is behind nat.
	TPTF_DES_NAT    = 0x0200, // Destination host is behind nat.
	TPTF_FAILED_EIP = 0x0400, // Server didn't get client's external address.
	TPTF_PENDING    = 0x0800, // The tunnel is kept for specific time when lost connection to server.

	TPTF_FORCE_16   = 0xFFFF
};


typedef struct _stHostInfo
{
	DWORD uid;
	IPV4  vip;

	USHORT DriverMapIndex, PeerDMIndex;
	USHORT flag, nTryCount; // TUNNEL_PUNCH_THROUGH_FLAG. Data for tunnel punch-through.
	UINT64 nStartPunchTime;

	union
	{
		DWORD key;

		struct
		{
			USHORT dwTimeoutTick, dwPeriod;  // Data for keep tunnel.
		};
	};

	CIpAddress eip, ip; // The variable eip.m_port is in network byte order.
//	BYTE vmac[6];

} stKTInfo, stNatPunchInfo;


class CMapTable : public stMapTable
{
public:
	CMapTable() { ZeroInit(); }
	~CMapTable() {}


	void ZeroInit()
	{
		m_Count = 0;
		memset(Entry, 0, sizeof(Entry));
		memset(DataIn, 0, sizeof(DataIn));
		memset(DataOut, 0, sizeof(DataOut));
		memset(m_RelayNetworkID, 0, sizeof(m_RelayNetworkID));
	}
	DWORD GetRelayNetworkID(USHORT index)
	{
		ASSERT(index < m_Count);
		return m_RelayNetworkID[index];
	}
	BOOL IsUsingRelay(USHORT DMIndex, DWORD dwVNetID, USHORT RelayHostDMIndex)
	{
		stEntry *pEntry = Entry + DMIndex;
		if(!(pEntry->flag & AIF_RELAY_HOST) || m_RelayNetworkID[DMIndex] != dwVNetID || pEntry->rhindex != RelayHostDMIndex)
			return FALSE;
		return TRUE;
	}


	void CleanTable(HANDLE hAdapter, UINT nPendingTunnel = 0);
	USHORT AddTableEntry(HANDLE hAdapter, stEntry *pEntry); // Return table index.
	USHORT FindPendingIndex(DWORD vip);
	BOOL SetTableItem(HANDLE hAdapter, UINT index, stEntry *pEntry, BOOL bCleanTrafficData = FALSE);
	BOOL GetTableData(HANDLE hAdapter, stMapTable *pMapTable);
//	BOOL SetTableFlag(HANDLE hAdapter, USHORT Index, USHORT flag);
	BOOL UpdateTableFlag(HANDLE hAdapter, USHORT Index, USHORT AddFlag, USHORT CleanFlag = 0);
	BOOL SetTableEntryDIndex(HANDLE hAdapter, USHORT Index, USHORT dindex);
	BOOL SetRelayHostIndexInfo(HANDLE hAdapter, DWORD NetID, USHORT Index, BOOL bUseRH, USHORT rhindex, USHORT destindex);
	void UpdateUID(HANDLE hAdapter, USHORT usIndex, DWORD uid);
	void RecycleMapIndex(USHORT index, UINT nCallerLine);
	void UpdateTrafficData(HANDLE hAdapter);
	BOOL UpdateServerRelayFlag(HANDLE hAdapter, BOOL bSet, DWORD uid);
	DWORD* GetRelayNetworkIDArray() { return m_RelayNetworkID; }


protected:
	DWORD m_RelayNetworkID[MAX_NETWORK_CLIENT]; // Data need only in the clinet.
	CList<USHORT> m_RecycledIndexList;


};


struct stNetMember
{
	stNetMember() { ZeroInit();	}

	void ZeroInit()
	{
		bInNat = 0;
		eip.ZeroInit();
		iip.ZeroInit();
		UserID = 0;
		vip = 0;
		memset(VMac, 0, sizeof(VMac));
		DriverMapIndex = INVALID_DM_INDEX;
		GUIDriverMapIndex = INVALID_DM_INDEX;
		LinkState = LS_OFFLINE;
		GUILinkState = LS_OFFLINE;
		P2PLinkState = LS_OFFLINE;
	}
	stNetMember& operator=(const stNetMember &in)
	{
		HostName = in.HostName;
	//	memcpy(&bInNat, &in.bInNat, sizeof(*this) - sizeof(CString));

		bInNat = in.bInNat;
		eip = in.eip;
		iip = in.iip;
		UserID = in.UserID;
		vip = in.vip;
		memcpy(VMac, in.VMac, sizeof(VMac));

		DriverMapIndex = in.DriverMapIndex;
		GUIDriverMapIndex = in.GUIDriverMapIndex;
		LinkState = in.LinkState;
		GUILinkState = in.GUILinkState;
		P2PLinkState = in.P2PLinkState;
		Flag = in.Flag;
		dwGroupBitMask = in.dwGroupBitMask;
		GroupIndex = in.GroupIndex;

		return *this;
	}
	void UpdataOnlineInfo(stNetMember &Member)
	{
		bInNat = Member.bInNat;
		eip = Member.eip;
		iip = Member.iip;
		eip.m_port = Member.eip.m_port;
		iip.m_port = Member.iip.m_port;
		DriverMapIndex = Member.DriverMapIndex;
		LinkState = Member.LinkState;

		// Don't update group index here.
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
	BOOL IsOnline() { return eip.m_port; }
	BOOL IsNetOnline() { return Flag & VF_IS_ONLINE; }

	void ReadInfo(CStreamBuffer &sb);
	void GetDriverEntryData(stEntry *pEntry, CIpAddress &ClientIIP, CIpAddress &ClientEIP);
	void GetPunchThroughInfo(stNatPunchInfo &info, BOOL bUseIIP);
	UINT ExportGUIDataStream(CStreamBuffer *pStreamBuffer);


	CString    HostName;
	BOOL       bInNat;   // Don't change order of this member for operator=.
	CIpAddress eip, iip; // External & Internal IP.
	DWORD      UserID;

	DWORD   vip;
	BYTE    VMac[6];

	USHORT DriverMapIndex, GUIDriverMapIndex; // For pure gui requesting list data.
	USHORT LinkState, GUILinkState, P2PLinkState;
	USHORT Flag;
	DWORD  dwGroupBitMask;
	BYTE   GroupIndex;

};

struct stVPNet
{
public:

	stVPNet()
	:NetIDCode(0)
	{
	}
	~stVPNet()
	{
	//	printx(_T("VNet %s is deleted.\n"), Name);
	}

	CString Name;
	DWORD   NetIDCode;
	USHORT  NetworkType, Flag;
	DWORD   m_dwGroupBitMask;
	BYTE    m_GroupIndex;  // Group index of local host.

	void SetGroupData(BYTE byGroupIndex, DWORD dwGroupBitMask)
	{
		ASSERT(byGroupIndex < m_nTotalGroup);
		ASSERT(m_GroupIndex == byGroupIndex);
		ASSERT(m_dwGroupBitMask == dwGroupBitMask);
	}

	stNetMember* FindHost(DWORD UID)
	{
		POSITION pos;
		stNetMember *pMember;
		for(pos = m_List.GetHeadPosition(); pos;)
		{
			pMember = &(m_List.GetNext(pos));
			if(pMember->UserID == UID)
				return pMember;
		}
		return 0;
	}
	stNetMember* AddMember(stNetMember *pMember)
	{
		ASSERT(pMember->GroupIndex < this->m_nTotalGroup);

		POSITION pos = m_List.AddTail(*pMember);
		pMember = &m_List.GetAt(pos);
		return pMember;
	}
	BOOL RemoveMember(DWORD uid)
	{
		for(POSITION pos = m_List.GetHeadPosition(); pos; m_List.GetNext(pos))
		{
			stNetMember &Member = m_List.GetAt(pos);
			if(Member.UserID == uid)
			{
				m_List.RemoveAt(pos);
				return TRUE;
			}
		}
		return FALSE;
	}
	BOOL IsMember(stNetMember *pMember)
	{
		for(POSITION pos = m_List.GetHeadPosition(); pos; )
			if(&m_List.GetNext(pos) == pMember)
				return TRUE;
		return FALSE;
	}

	UINT ExportGUIDataStream(CStreamBuffer *pStreamBuffer);
	void UpdateFlag(USHORT FlagIn)
	{
		USHORT mask = VF_NEED_PASSWORD | VF_DISALLOW_USER; // For changeable data only.
		Flag &= ~mask;
		Flag |= (mask & FlagIn);
	}
	void SetFlag(USHORT FlagIn) { Flag = FlagIn; }
	BOOL IsNetOnline() { return Flag & VF_IS_ONLINE; }

	CList<stNetMember>* GetMemberList() { return &m_List; }
	UINT GetGroupCount() { return m_nTotalGroup; }
	void SetGroupCount(UINT nCount) { m_nTotalGroup = nCount; }
	stVNetGroup* GetGroupArrayAddress() { return m_GroupArray; }
	void SetLocalGroupIndex(UINT index) { m_GroupIndex = index; }

	UINT FindDefaultGroupIndex()
	{
		for(UINT i = 0; i < m_nTotalGroup; ++i)
			if(m_GroupArray[i].Flag & VGF_DEFAULT_GROUP)
				return i;
		return 0;
	}
	UINT CreateGroup(UINT nIndex, stVNetGroup *pGroup, UINT nSize)
	{
		ASSERT(nIndex < MAX_VNET_GROUP_COUNT && nIndex);
		ASSERT(nSize == sizeof(stVNetGroup));

		memcpy(&m_GroupArray[nIndex], pGroup, sizeof(stVNetGroup));
		++m_nTotalGroup;

		return m_nTotalGroup - 1; // Return index.
	}
	UINT DeleteGroup(UINT nIndex)
	{
		ASSERT(nIndex < m_nTotalGroup && nIndex);

		if(m_GroupArray[nIndex].Flag & VGF_DEFAULT_GROUP)
			m_GroupArray[0].Flag |= VGF_DEFAULT_GROUP;
		if(nIndex < m_nTotalGroup - 1) // Make sure move data is necessary.
			memcpy(&m_GroupArray[nIndex], &m_GroupArray[nIndex + 1], (m_nTotalGroup - nIndex - 1) * sizeof(stVNetGroup));
		--m_nTotalGroup;

		UINT DefaultGroupIndex = FindDefaultGroupIndex(); // Update index.
		DWORD dwMask = (0x01 << nIndex) - 1, dwReservedBits;

		for(POSITION pos = m_List.GetHeadPosition(); pos; )
		{
			stNetMember *pMember = &m_List.GetNext(pos);

			if(pMember->GroupIndex == nIndex)
				pMember->GroupIndex = DefaultGroupIndex;
			else if(pMember->GroupIndex > nIndex)
				pMember->GroupIndex--;

			dwReservedBits = pMember->dwGroupBitMask & dwMask;
			pMember->dwGroupBitMask = ((pMember->dwGroupBitMask >> 1) & ~dwMask) | dwReservedBits;
			pMember->dwGroupBitMask |= 0x80000000; // Reset unused bits to 1.
		}

		if(m_GroupIndex == nIndex) // Check local host.
			m_GroupIndex = DefaultGroupIndex;
		else if(m_GroupIndex > nIndex)
			m_GroupIndex--;

		dwReservedBits = m_dwGroupBitMask & dwMask;
		m_dwGroupBitMask = ((m_dwGroupBitMask >> 1) & ~dwMask) | dwReservedBits;
		m_dwGroupBitMask |= 0x80000000; // Reset unused bits to 1.

		return m_nTotalGroup - 1; // Return index.
	}
	stNetMember* UpdateMemberGroupIndex(DWORD UID, UINT nNewIndex)
	{
		stNetMember *pMember, *pOut = 0;
		for(POSITION pos = m_List.GetHeadPosition(); pos; )
		{
			pMember = &m_List.GetNext(pos);
			if(pMember->UserID != UID)
				continue;
			pMember->GroupIndex = nNewIndex;
			pOut = pMember;
			break;
		}
		return pOut;
	}
	BOOL SetDefaultGroup(UINT nIndex)
	{
		if(nIndex >= m_nTotalGroup)
			return FALSE;

		for(UINT i = 0; i < m_nTotalGroup; ++i)
			if(i != nIndex)
				m_GroupArray[i].Flag &= ~VGF_DEFAULT_GROUP;
			else
				m_GroupArray[i].Flag |= VGF_DEFAULT_GROUP;

		return TRUE;
	}
	BOOL UpdateGroupFlag(UINT nIndex, DWORD dwMask, DWORD dwFlag)
	{
		if(nIndex >= m_nTotalGroup /*|| !nIndex*/)
			return FALSE;

		m_GroupArray[nIndex].Flag &= ~dwMask;
		m_GroupArray[nIndex].Flag |= (dwMask & dwFlag);

		return TRUE;
	}
	BOOL UpdateGroupName(UINT nIndex, TCHAR *pNewName, UINT nLength)
	{
		if(nIndex >= m_nTotalGroup)
			return FALSE;
		memcpy(m_GroupArray[nIndex].GroupName, pNewName, sizeof(TCHAR) * nLength);
		m_GroupArray[nIndex].GroupName[nLength] = 0;
		return TRUE;
	}
	BOOL CheckGroupConnectionState(stNetMember *pMember)
	{
		if(m_nTotalGroup > 1)
		{
			if(m_GroupArray[m_GroupIndex].Flag & VGF_STATE_OFFLINE)
				return FALSE;
			if(m_GroupArray[pMember->GroupIndex].Flag & VGF_STATE_OFFLINE)
				return FALSE;

			if(! ((0x01 << m_GroupIndex) & pMember->dwGroupBitMask) )
				return FALSE;
			if(! ((0x01 << pMember->GroupIndex) & m_dwGroupBitMask) )
				return FALSE;
		}
		return TRUE;
	}
	BOOL UpdateGroupBitMask(UINT nGroupIndex, BOOL bOnline)
	{
		if(nGroupIndex >= m_nTotalGroup)
			return FALSE;
		DWORD dwMask = 0x01 << nGroupIndex;

		if( dwMask & m_dwGroupBitMask )
		{
			if(bOnline)
				return FALSE;
			m_dwGroupBitMask &= ~dwMask;
		}
		else
		{
			if(!bOnline)
				return FALSE;
			m_dwGroupBitMask |= dwMask;
		}

		return TRUE;
	}
	BOOL CheckGroupConnectionState(stNetMember *pMember1, stNetMember *pMember2)
	{
		if(m_nTotalGroup > 1)
		{
			if(m_GroupArray[pMember1->GroupIndex].Flag & VGF_STATE_OFFLINE)
				return FALSE;
			if(m_GroupArray[pMember2->GroupIndex].Flag & VGF_STATE_OFFLINE)
				return FALSE;

			if(! ((0x01 << pMember1->GroupIndex) & pMember2->dwGroupBitMask) )
				return FALSE;
			if(! ((0x01 << pMember2->GroupIndex) & pMember1->dwGroupBitMask) )
				return FALSE;
		}
		return TRUE;
	}

protected:

	friend class CNetworkManager;

	UINT m_nTotalGroup;
	stVNetGroup m_GroupArray[MAX_VNET_GROUP_COUNT]; // Index zero is the default group.
	CList<stNetMember> m_List; // Only worker thread can access this member.


};


struct stNetMemberInfo
{
	stVPNet *pVNet;
	stNetMember *pMember;
};


struct stTPTResult // 64 Bytes.
{
	USHORT bResult, TPTFlag;
	DWORD SrcUID, DestUID;
	CIpAddress SrcEIP, DesEIP;
	time_t time;
};


class CNetworkManager
{
public:

	CNetworkManager()
	{
		ASSERT(!pNetManager);
		pNetManager = this;
		m_UPNPPort = 0;
		m_FirewallPort = 0;
		m_IPCPort = 0;
		m_TunnelSocketMode = TSM_KERNEL_MODE;
		m_TunnelOpenState = FALSE;

		m_pEventArray = 0;
		m_pdwEventCount = 0;

		m_KEEP_TUNNEL_PERIOD = KEEP_TUNNEL_PERIOD;
		m_KEEP_TUNNEL_WITH_SERVER = KEEP_TUNNEL_WITH_SERVER;
	}
	~CNetworkManager()
	{
		Release();
		ASSERT(!m_TunnelOpenState);
		ASSERT(pNetManager == this);
		pNetManager = 0;
	}


	void SetEventHandle(HANDLE *pEventArray, DWORD *pdwEventCount)
	{
		m_pEventArray = pEventArray;
		m_pdwEventCount = pdwEventCount;
	}

//	stVPNet* Find(TCHAR *pName);
	stVPNet* AddVPNet(TCHAR *pName, DWORD NetIDCode);
	void AddVPNet(stVPNet *pNet, DWORD NetIDCode);
	BOOL ExitVPNet(HANDLE hAdapter, CMapTable *pTable, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList);
	BOOL OfflineVPNet(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList, BOOL bOnline);
	BOOL RemoveVNetMember(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, stNetMember *pMember, CMemberUpdateList &MemberUpdateList);
	BOOL GroupStateChanged(HANDLE hAdapter, stVPNet *pVNet, stClientInfo *pClientInfo, CMemberUpdateList &MemberUpdateList, BOOL *pbActive);
	UINT UpdateConnectionState(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, stNetMember *pMember, CMemberUpdateList &MemberUpdateList, BOOL bActive);
	BOOL UpdateConnectionState(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList, BOOL bActive);

	UINT GetNetCount() { return m_NetList.GetCount(); }
	stVPNet* AllocVNet() { return new stVPNet; }
	void Release(BOOL bCloseTunnel = TRUE);

	UINT ExportGUIDataStream(CStreamBuffer *pStreamBuffer);
	UINT ExportRelayDataStream(CStreamBuffer *pStreamBuffer);
	stVPNet* FindNet(const TCHAR *pVNetName, DWORD NetIDCode); // NetIDCode can be zero if don't care.
	void UpdateGUILinkState();
	UINT GetUserExistCount(DWORD UID, USHORT *pDriverMapIndex, USHORT *pLinkState = 0, USHORT *pP2PLinkState = 0);
	UINT GetConnectedStateInfo(stVPNet *pSkipVNet, stNetMember *pDestMember, USHORT *pDriverMapIndex, USHORT *pLinkState);
	void DebugCheckState();
	BOOL AddNewMember(CString NetName, stNetMember *pMember);
	INT  FindNetMember(DWORD uid, CList<stNetMemberInfo> *pList, BOOL &bNeedPTT);
	stNetMember* FindNetMember(DWORD_PTR uid);
//	stNetMember* FindNetMemberByDMIndex(USHORT DMIndex);
//	BOOL FindRelayHost(stVPNet *pVNet, stNetMember **pDestMember);
	void EnableReportTPTResult(DWORD ReportMode) { ASSERT(ReportMode < 0xFF); m_ReportTPT = (BYTE)ReportMode; }

	void SwitchRelayMode(HANDLE hAdapter, DWORD uid, BOOL bServerRelay);
	BOOL RequestHostRelay(DWORD NetID, DWORD RequestUID, DWORD DestUID, USHORT &SrcDMIndex, USHORT &DestDMIndex);
	BOOL SetRelayHostIndex(DWORD NetID, DWORD SrcUID, DWORD DestUID, DWORD RHUID, USHORT SrcIndex, USHORT DestIndex);
	void CancelVNetRelay(HANDLE hAdapter, CMapTable *pTable, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList);
	void CancelRelayHost(stVPNet *pVNet, stNetMember *pRelayHost, CMemberUpdateList &MemberUpdateList, BOOL bCheckRole = FALSE);
	BOOL CancelRelay(HANDLE hAdapter, CMapTable *pTable, stNetMember *pMember, DWORD NetID, CMemberUpdateList &MemberUpdateList);
	BOOL GetRelayHost(CMapTable *pTable, stVPNet *pVNet, stNetMember *pMember, stNetMember **pRelayHost);

	UINT UpdateMemberProfile(DWORD UserID, UINT nType, void* pData, UINT nDataSize);
	BOOL UpdateMemberRole(DWORD NetID, DWORD UserID, DWORD dwFlag, DWORD dwMask, stVPNet *&pDestVNet, stNetMember *&pDestMember, BOOL &bRelayHostCancelled);
	INT  UpdataMemberLinkState(DWORD uid, USHORT dwLinkState, BOOL bRestore = FALSE, CMemberUpdateList *pMemberUpdateList = 0, USHORT *pP2PLinkState = 0);
	INT  UpdateP2PLinkState(DWORD uid, USHORT dwLinkState);
	INT  UpdateMemberExternalAddress(DWORD uid, CIpAddress eip, USHORT eport);

	// NAT punch-through methods.
	INT  AddPunchHost(stNetMember *pNetMember, BOOL bActive = FALSE); // Return link state.
	UINT CancelPunchHost(DWORD uid, stNatPunchInfo *pOut = 0);
	BOOL PTSetAck(DWORD uid);
	INT  PTSetInitData(DWORD uid, UINT DMIndex, DWORD key, UINT64 nStartTime);
	void RemoveAllPunchHost();
	void CheckTable(HANDLE hAdapter, DWORD *UIDArray, UINT &nCount, UINT &nReportCount);
	void PunchOnce(HANDLE hDevice);
	void ReportPTResult(stClientInfo *pClientInfo, BOOL bSuccess, stNatPunchInfo *pDesInfo);
	UINT GetReportData(CStreamBuffer &sb);
	UINT GetReportCount() { return m_ReportList.GetCount(); }

	// Methods for TPT handshake.
	void PrepareHandshakeData();
	UINT GetHandshakeData(CStreamBuffer &sb, DWORD dwType, UINT nProspectiveCount = 0);
	UINT GetHandshakeHostCount() { return m_TempList.GetCount(); }

	// Keep udp tunnel methods.
	BOOL TickOnce();
	void KTAddHost(stKTInfo *pInfo, BOOL bServerSide = FALSE);
	UINT KTMarkPending(HANDLE hAdapter, DWORD uid, UINT nMinutes);
	UINT KTRemovePending(DWORD vip);
	UINT KTGetPendingCount();
	BOOL KTUnMarkPending(HANDLE hAdapter, DWORD vip, CMapTable *pTable);
	void KTRemoveAllHost();
	UINT KTRemoveHost(DWORD uid);
	BOOL KTTimerEntry(HANDLE hAdapter, stClientInfo *pClientInfo, CMapTable *pTable);
	void KTSetTimerTick(USHORT ServerTick, USHORT PeerTick) { m_KEEP_TUNNEL_WITH_SERVER = ServerTick; m_KEEP_TUNNEL_PERIOD = PeerTick; }

	// Arp methods.
	UINT ArpDeleteAddress(USHORT DriverIndex, DWORD ipv4, BOOL bDynamicOnly = TRUE, BOOL bDeleteAll = FALSE);

	// UPnP methods.
	void AddPortMapping(BOOL bUDP, CIpAddress iip, UINT iport, UINT eport);
	void RemovePortMapping(BOOL bUDP, UINT eport);

	// Windows firewall methods.
	BOOL OpenFirewallPort(UINT port);
	void CloseFirewallPort();
	BOOL WFWSetPort(BOOL bUDP, UINT PortOpen, UINT PortClose); // Don't call this directly.


	struct stPTHandshakeInfo
	{
		DWORD  UID, Key;
		USHORT DriverMapIndex, Flag;
		UINT64 nStartTime;  // Server time in seconds.
	};


	// Tunnel socket methods.
	enum TUNNEL_SOCKET_MODE
	{
		TSM_NULL,
		TSM_KERNEL_MODE,
		TSM_USER_MODE
	};


	BOOL OpenTunnelSocket(HANDLE hAdapter, DWORD ip, USHORT port);
	void CloseIPCSocket();
	void CloseTunnelSocket(HANDLE hAdapter, BOOL bForceCloseKMSocket = FALSE);
	BOOL SetTunnelSocketMode(HANDLE hAdapter, BYTE mode);
	void SetTunnelSocketMode(BYTE mode) { m_TunnelSocketMode = mode; }
	BOOL CheckTunnelSocketAddress(DWORD ip, USHORT &port);
	void DirectSend(HANDLE hAdapter, IpAddress ip, BOOL bIsIPV6, USHORT port, void *pData, UINT size); // Port is in network byte order.
	void AdjustAdapterParameter(IPV4 vipIn, BYTE *vmacIn, DWORD TaskOffloadCaps, DWORD Caps = 0); // Do initialization work in this method.
	void InitUserModeSocketThread(HANDLE hAdapter);
	void CloseUserModeSocketThread();
	DWORD GetTunnelSocketMode() { return m_TunnelSocketMode; }
	TUNNEL_SOCKET_TYPE* GetTunnelSocket() { return &m_TunnelSocket; }

	static BOOL IsValidIpAddress(IPV4 ip);


protected:

	CList<stNatPunchInfo>    m_HostList, m_FirstTimeList;
	CList<stKTInfo>          m_KeepTunnelList;
	CList<stVPNet*>          m_NetList; // Only worker thread can access this.
	CList<stPTHandshakeInfo> m_TempList;
	CList<stTPTResult>       m_ReportList;

	// 2012-09-10 Now timer function runs in the worker thread.
//	CCriticalSectionUTX m_cs;   // Protect m_HostList & m_FirstTimeList.
//	CCriticalSectionUTX m_csKT; // Protect m_KeepTunnelList.

	BYTE   m_ReportTPT;
	ULONG  m_AdapterIndex; // The adapter index may change when an adapter is disabled and then enabled, or under other circumstances, and should not be considered persistent.
	USHORT m_UPNPPort, m_FirewallPort, m_IPCPort;
	USHORT m_KEEP_TUNNEL_PERIOD, m_KEEP_TUNNEL_WITH_SERVER;

	UINT64 m_nLastAssignedTime; // For first punching.
	UINT   m_AssignedCount;

	// Tunnel socket data.
	TUNNEL_SOCKET_TYPE m_TunnelSocket;
	BYTE m_TunnelSocketMode, m_TunnelOpenState; // Open state doesn't mean tunnel socket is really open. It means if need to open socket when switch mode.

	// Event array of the worker thread.
	HANDLE *m_pEventArray;
	DWORD *m_pdwEventCount;

	friend CNetworkManager* AppGetNetManager();
	static CNetworkManager* pNetManager;


};


inline CNetworkManager* AppGetNetManager()
{
	return CNetworkManager::pNetManager;
}

inline BOOL NeedConnect(stVPNet *pVNet, stNetMember *pMember)
{
	ASSERT(pVNet->IsMember(pMember));

	if(pVNet->NetworkType == VNT_HUB_AND_SPOKE && !(pVNet->Flag & VF_HUB) && !(pMember->Flag & VF_HUB))
		return FALSE;

	// Check group state.
	if(!pVNet->CheckGroupConnectionState(pMember))
		return FALSE;

	return TRUE;
}

inline BOOL NeedConnect(stVPNet *pVNet, stNetMember *pMember1, stNetMember *pMember2)
{
	ASSERT(pVNet->IsMember(pMember1) && pVNet->IsMember(pMember2));

	if(pVNet->NetworkType == VNT_HUB_AND_SPOKE && !(pMember1->Flag & VF_HUB) && !(pMember2->Flag & VF_HUB))
		return FALSE;

	// Check group state.
	if(!pVNet->CheckGroupConnectionState(pMember1, pMember2))
		return FALSE;

	return TRUE;
}


