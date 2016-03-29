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
#include "CnMatrixCore.h"
#include "NetManager.h"
#include "CoreAPI.h"
#include "DriverAPI.h"
#include <Natupnp.h>
#include <netfw.h>


CNetworkManager* CNetworkManager::pNetManager = NULL;


void CMapTable::CleanTable(HANDLE hAdapter, UINT nPendingTunnel)
{
	if (nPendingTunnel)
	{
		stEntry item;
		ZeroMemory(&item, sizeof(item));
		UINT nCount = 0, LastUsedIndex = 0;

		m_RecycledIndexList.RemoveAll();
		for (UINT i = 0; i < m_Count; ++i)
			if (Entry[i].flag & AIF_PENDING)
			{
				LastUsedIndex = i;
				++nCount;
			}
			else
			{
				m_RecycledIndexList.AddTail(i);
				SetTableItem(hAdapter, i, &item, TRUE);
			}

		m_Count = LastUsedIndex + 1;

	//	printx("CleanTable %d - %d\n", nCount, nPendingTunnel);
		ASSERT(nCount == nPendingTunnel);
	}
	else
	{
		ZeroInit();
		m_RecycledIndexList.RemoveAll();
		::CleanTable(hAdapter);
	}
}

USHORT CMapTable::AddTableEntry(HANDLE hAdapter, stEntry *pEntry)
{
	USHORT index = (USHORT)m_Count;

	if (m_Count == MAX_NETWORK_CLIENT && !m_RecycledIndexList.GetCount())
		return INVALID_DM_INDEX;

	if (m_RecycledIndexList.GetCount())
	{
		index = m_RecycledIndexList.RemoveHead();

		ASSERT(index < m_Count);
		ASSERT(Entry[index].pip.IsZeroAddress() && Entry[index].vip == 0 && Entry[index].port == 0);

		RtlCopyMemory(&Entry[index], pEntry, sizeof(stEntry));
		::SetTableItem(hAdapter, 1, index, pEntry);
	}
	else
	{
		RtlCopyMemory(&Entry[index], pEntry, sizeof(stEntry));
		m_Count++;

		::AddTableEntry(hAdapter, pEntry, 1);
	}

	DataIn[index] = DataOut[index] = 0;
	ASSERT(m_RelayNetworkID[index] == 0); // Reset the value when recycle driver map index.

	return index;
}

USHORT CMapTable::FindPendingIndex(DWORD vip)
{
	UINT i;
	for (i = 0; i < m_Count; ++i)
		if (Entry[i].vip == vip)
		{
			ASSERT(Entry[i].flag & AIF_OK);
			ASSERT(Entry[i].flag & AIF_PENDING);

			return i;
		}

	return INVALID_DM_INDEX;
}


BOOL CMapTable::SetTableItem(HANDLE hAdapter, UINT index, stEntry *pEntry, BOOL bCleanTrafficData)
{
	ASSERT(index < m_Count);

	if (index >= m_Count)
		return FALSE;

	if (bCleanTrafficData)
	{
		ZeroMemory(&DataIn[index], sizeof(DataIn[0]));
		ZeroMemory(&DataOut[index], sizeof(DataOut[0]));
	}

	if (pEntry)
		memcpy(&Entry[index], pEntry, sizeof(stEntry));
	else
		memset(&Entry[index], 0, sizeof(stEntry));

	return ::SetTableItem(hAdapter, 1, index, &Entry[index]);
}

BOOL CMapTable::GetTableData(HANDLE hAdapter, stMapTable *pMapTable)
{
	BOOL result = ::GetTableData(hAdapter, this);

	if (pMapTable != NULL)
		memcpy(pMapTable, this, sizeof(stMapTable));

	return result;
}

//BOOL CMapTable::SetTableFlag(HANDLE hAdapter, USHORT Index, USHORT flag)
//{
//	ASSERT(Index < MAX_NETWORK_CLIENT);
//
//	Entry[Index].flag = flag;
//	return ::SetEntryData(hAdapter, Index, 0, flag);
//}

BOOL CMapTable::UpdateTableFlag(HANDLE hAdapter, USHORT Index, USHORT AddFlag, USHORT CleanFlag)
{
	ASSERT(Index < MAX_NETWORK_CLIENT);

	Entry[Index].flag |= AddFlag;
	if (CleanFlag)
		Entry[Index].flag &= ~CleanFlag;

	return ::SetEntryData(hAdapter, Index, 0, Entry[Index].flag);
}

BOOL CMapTable::SetTableEntryDIndex(HANDLE hAdapter, USHORT Index, USHORT dindex)
{
	ASSERT(Index < MAX_NETWORK_CLIENT);

	Entry[Index].dindex = dindex;
	return ::SetEntryData(hAdapter, Index, 1, dindex);
}

BOOL CMapTable::SetRelayHostIndexInfo(HANDLE hAdapter, DWORD NetID, USHORT Index, BOOL bUseRH, USHORT rhindex, USHORT destindex)
{
	ASSERT(Index < MAX_NETWORK_CLIENT);

	UINT value;
	if (Index >= m_Count)
		return FALSE;

	if (bUseRH)
	{
		m_RelayNetworkID[Index] = NetID;
		value = (rhindex << 16) | destindex;
		Entry[Index].rhindex = rhindex;
		Entry[Index].destindex = destindex;
		Entry[Index].flag |= AIF_RELAY_HOST;
	}
	else
	{
		ASSERT(m_RelayNetworkID[Index] == NetID);
		m_RelayNetworkID[Index] = 0;
		value = 0xFFFFFFFF;
		Entry[Index].flag &= ~AIF_RELAY_HOST;
	}

	return ::SetEntryData(hAdapter, Index, 2, value); // This will update driver flag.
}

void CMapTable::UpdateUID(HANDLE hAdapter, USHORT usIndex, DWORD uid)
{
	ASSERT(usIndex < MAX_NETWORK_CLIENT);

	Entry[usIndex].uid = uid;
	::SetEntryData(hAdapter, usIndex, 3, uid);
}

void CMapTable::RecycleMapIndex(USHORT index, UINT nCallerLine)
{
	POSITION pos;
	ASSERT(index < m_Count);

	for (pos = m_RecycledIndexList.GetHeadPosition(); pos; ) // Code for debug check.
		if (m_RecycledIndexList.GetNext(pos) == index)
		{
			AddJobReportBug(CBC_DuplicateDMIndex, &nCallerLine, sizeof(nCallerLine));
			ASSERT(0);
			return;
		}

	for (pos = m_RecycledIndexList.GetHeadPosition(); pos; m_RecycledIndexList.GetNext(pos))
		if (index < m_RecycledIndexList.GetAt(pos))
		{
			m_RecycledIndexList.InsertBefore(pos, index);
			break;
		}

	if (pos == NULL)
		m_RecycledIndexList.AddTail(index);

	m_RelayNetworkID[index] = 0;
}

void CMapTable::UpdateTrafficData(HANDLE hAdapter)
{
	::WriteTrafficInfo(hAdapter, DataIn, DataOut, m_Count);
}

BOOL CMapTable::UpdateServerRelayFlag(HANDLE hAdapter, BOOL bSet, DWORD uid)
{
	UINT i;
	BOOL bFound = FALSE;

	for(i = 0; i < m_Count; ++i)
		if(Entry[i].uid == uid)
		{
			if(bSet)
				UpdateTableFlag(hAdapter, i, AIF_SERVER_RELAY, 0);
			else
				UpdateTableFlag(hAdapter, i, 0, AIF_SERVER_RELAY);

			bFound = TRUE;
			break;
		}

	return bFound;
}


void stNetMember::ReadInfo(CStreamBuffer &sb)
{
	// Must match server function : CAccountInfo::WriteInfo.
	// All data must be the same size with server side.
	DWORD dwFlag;

	sb >> UserID;
	CoreReadString(sb, HostName);

	sb >> bInNat >> dwFlag;
	sb >> eip >> eip.m_port;
	sb >> iip >> iip.m_port >> vip;
	sb.Read(VMac, sizeof(VMac));
	sb >> dwGroupBitMask >> GroupIndex;

	Flag = (USHORT)dwFlag;
}

void stNetMember::GetDriverEntryData(stEntry *pEntry, CIpAddress &ClientIIP, CIpAddress &ClientEIP)
{
	memcpy(pEntry->mac, VMac, sizeof(pEntry->mac));
	pEntry->vip = vip;
	pEntry->pip = eip;

	pEntry->port = NBPort(eip.m_port);
	pEntry->dindex = INVALID_DM_INDEX;

	pEntry->uid = (DWORD_PTR)this; // Setup key for TPT.
	pEntry->flag = AIF_START_PT;

	if (eip == ClientEIP && bInNat) // In the same subnet.
	{
		pEntry->pip = iip;
		pEntry->port = NBPort(iip.m_port);

		if (ClientIIP == ClientEIP)
			printx("Host is NAT.\n");
	}
}

void stNetMember::GetPunchThroughInfo(stNatPunchInfo &info, BOOL bUseIIP)
{
	info.uid = UserID;
	info.DriverMapIndex = DriverMapIndex;
	info.nTryCount = NAT_PUNCH_THROUGH_TRY_COUNT;

	// Wait notification of the peer.
	info.PeerDMIndex = 0;
	info.key = 0;

	if(bUseIIP)
	{
		info.eip = iip;
		info.eip.m_port = NBPort(iip.m_port);
		info.ip = eip;
		info.ip.m_port = NBPort(eip.m_port);
	}
	else
	{
		info.eip = eip;
		info.eip.m_port = NBPort(eip.m_port);
	}

	info.vip = vip;
//	memcpy(info.vmac, VMac, sizeof(info.vmac));
}

UINT stNetMember::ExportGUIDataStream(CStreamBuffer *pStreamBuffer)
{
	// Must match stGUIVLanMember::ReadFromStream().
	UINT BytesWritten;
	BYTE byLen = HostName.GetLength();
	ASSERT(byLen && byLen <= MAX_HOST_NAME_LENGTH);

	if (pStreamBuffer != NULL)
	{
		pStreamBuffer->WriteString(byLen, (const TCHAR*)HostName);

		*pStreamBuffer << UserID << vip << GUILinkState;
		pStreamBuffer->Write(VMac, sizeof(VMac));
		*pStreamBuffer << GUIDriverMapIndex << eip << eip.m_port << Flag << dwGroupBitMask << GroupIndex;
	}

	BytesWritten = sizeof(byLen) + sizeof(TCHAR) * byLen + sizeof(UserID) + sizeof(vip) + sizeof(GUILinkState) + sizeof(VMac) + sizeof(GUIDriverMapIndex)
					+ eip.GetStreamingSize() + sizeof(eip.m_port) + sizeof(Flag) + sizeof(dwGroupBitMask) + sizeof(GroupIndex);

	return BytesWritten;
}

UINT stVPNet::ExportGUIDataStream(CStreamBuffer *pStreamBuffer)
{
	UINT BytesWritten = 0;
	USHORT HostCount = m_List.GetCount(), len = Name.GetLength();

	if (pStreamBuffer != NULL) // Match stGUIVLanInfo::ReadFromStream.
	{
		*pStreamBuffer << HostCount << len;
		pStreamBuffer->Write(Name.GetBuffer(), sizeof(TCHAR) * len);
		*pStreamBuffer << NetIDCode << NetworkType << Flag;

		*pStreamBuffer << m_dwGroupBitMask << m_GroupIndex << m_nTotalGroup;
		pStreamBuffer->Write(m_GroupArray, sizeof(stVNetGroup) * m_nTotalGroup);
	}

	BytesWritten = sizeof(HostCount) + sizeof(len) + sizeof(TCHAR) * len + sizeof(NetIDCode) + sizeof(NetworkType) + sizeof(Flag)
				 + sizeof(m_dwGroupBitMask) + sizeof(m_GroupIndex) + sizeof(m_nTotalGroup) + m_nTotalGroup * sizeof(stVNetGroup);

	POSITION pos;
	for(pos = m_List.GetHeadPosition(); pos != NULL; HostCount--)
	{
		stNetMember *pMember = &m_List.GetNext(pos);
		BytesWritten += pMember->ExportGUIDataStream(pStreamBuffer);
	}
	ASSERT(!HostCount);

	return BytesWritten;
}


//stVPNet* CNetworkManager::Find(TCHAR *pName)
//{
//	POSITION pos = m_NetList.GetHeadPosition();
//	stVPNet *pNet;
//
//	for(; pos; )
//	{
//		pNet = m_NetList.GetNext(pos);
//		if(pNet->Name == pName)
//			return pNet;
//	}
//
//	return 0;
//}

stVPNet* CNetworkManager::AddVPNet(TCHAR *pName, DWORD NetIDCode)
{
	stVPNet *pNet = AllocVNet();

	UINT length = _tcslen(pName);

	pNet->NetIDCode = NetIDCode;
	pNet->Name = pName;

	m_NetList.AddTail(pNet);

	return pNet;
}

void CNetworkManager::AddVPNet(stVPNet *pNet, DWORD NetIDCode)
{
	pNet->NetIDCode = NetIDCode;
	m_NetList.AddTail(pNet);
}

BOOL CNetworkManager::ExitVPNet(HANDLE hAdapter, CMapTable *pTable, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList)
{
	stNetMember *pMember;
	USHORT DriverMapIndex, LinkState;

	for (POSITION pos = pVNet->m_List.GetHeadPosition(); pos;)
	{
		pMember = &(pVNet->m_List.GetNext(pos));
		if(!pMember->IsOnline())
			continue;
		if (!pVNet->IsNetOnline() || !pMember->IsNetOnline() || !NeedConnect(pVNet, pMember))
			continue;

		UINT ExistCount = GetConnectedStateInfo(pVNet, pMember, &DriverMapIndex, &LinkState);

		if(!ExistCount || DriverMapIndex == INVALID_DM_INDEX)
		{
			ASSERT((!ExistCount && DriverMapIndex == INVALID_DM_INDEX) || (ExistCount > 0 && DriverMapIndex == INVALID_DM_INDEX));

			CancelPunchHost(pMember->UserID);
			KTRemoveHost(pMember->UserID);

			pTable->SetTableItem(hAdapter, pMember->DriverMapIndex, 0); // Clean driver entry.
			pTable->RecycleMapIndex(pMember->DriverMapIndex, __LINE__);

			if(ExistCount)
				MemberUpdateList.AddData(pMember->UserID, LS_NO_CONNECTION);
		}
	}

	POSITION pos = m_NetList.Find(pVNet);
	delete m_NetList.GetAt(pos);
	m_NetList.RemoveAt(pos);

#ifdef _DEBUG
	DebugCheckState();
#endif

	return TRUE;
}

BOOL CNetworkManager::OfflineVPNet(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList, BOOL bOnline)
{
	ASSERT((bOnline && !pVNet->IsNetOnline()) || (!bOnline && pVNet->IsNetOnline()));

	if(bOnline)
		pVNet->Flag |= VF_IS_ONLINE;
	else
		pVNet->Flag &= ~VF_IS_ONLINE;

	return UpdateConnectionState(hAdapter, pClientInfo, pVNet, MemberUpdateList, TRUE);
}

BOOL CNetworkManager::RemoveVNetMember(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, stNetMember *pMember, CMemberUpdateList &MemberUpdateList)
{
	pMember->Flag &= ~VF_IS_ONLINE; // Let member be offline to emulate the connection state.

	if(pMember->IsOnline())
	{
		UINT nExistCount = UpdateConnectionState(hAdapter, pClientInfo, pVNet, pMember, MemberUpdateList, FALSE);
		if(!nExistCount)
			MemberUpdateList.Remove(pMember->UserID);
	}

	pVNet->RemoveMember(pMember->UserID);

	return TRUE;
}

BOOL CNetworkManager::GroupStateChanged(HANDLE hAdapter, stVPNet *pVNet, stClientInfo *pClientInfo, CMemberUpdateList &MemberUpdateList, BOOL *pbActive)
{
	stNetMember *pMember;
	USHORT DriverMapIndex, LinkState;
	CMapTable *pTable = &pClientInfo->MapTable;
	DWORD LocalUID = pClientInfo->ID1;
	BOOL bActive;
	stEntry entry;

	PrepareHandshakeData();
	for(POSITION pos = pVNet->m_List.GetHeadPosition(); pos;)
	{
		pMember = &(pVNet->m_List.GetNext(pos));
		if(!pMember->IsOnline())
			continue;

		UINT ExistCount = GetConnectedStateInfo(pVNet, pMember, &DriverMapIndex, &LinkState);

		if(!ExistCount || DriverMapIndex == INVALID_DM_INDEX)
		{
			if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
			{
				if(pMember->LinkState == LS_NO_CONNECTION)
				{
					if(!pbActive)
						bActive = LocalUID > pMember->UserID ? TRUE : FALSE;
					else
						bActive = *pbActive;

					pMember->GetDriverEntryData(&entry, pClientInfo->ClientInternalIP, pClientInfo->ClientExternalIP);
					pMember->DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
					pMember->LinkState = AddPunchHost(pMember, bActive);

					MemberUpdateList.AddData(pMember->UserID, pMember->LinkState);
				}
			}
			else
			{
				if(pMember->LinkState != LS_NO_CONNECTION)
				{
					CancelPunchHost(pMember->UserID);
					KTRemoveHost(pMember->UserID);

					pTable->SetTableItem(hAdapter, pMember->DriverMapIndex, 0); // Clean driver entry.
					pTable->RecycleMapIndex(pMember->DriverMapIndex, __LINE__);

					pMember->DriverMapIndex = INVALID_DM_INDEX;
					pMember->LinkState = LS_NO_CONNECTION;

					MemberUpdateList.AddData(pMember->UserID, pMember->LinkState);
				}
			}
		}
		else
		{
			if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
			{
				pMember->DriverMapIndex = DriverMapIndex;
				pMember->LinkState = LinkState;
			}
			else
			{
				pMember->DriverMapIndex = INVALID_DM_INDEX;
				pMember->LinkState = LS_NO_CONNECTION;
			}
		}
	}

#ifdef _DEBUG
	DebugCheckState();
#endif

	return TRUE;
}

UINT CNetworkManager::UpdateConnectionState(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, stNetMember *pMember, CMemberUpdateList &MemberUpdateList, BOOL bActive)
{
	ASSERT(pVNet->IsMember(pMember) && pMember->IsOnline());
	PrepareHandshakeData();

	stEntry entry;
	USHORT DriverMapIndex, LinkState;
	CMapTable *pTable = &pClientInfo->MapTable;

	UINT ExistCount = GetConnectedStateInfo(pVNet, pMember, &DriverMapIndex, &LinkState);

	if(!ExistCount || DriverMapIndex == INVALID_DM_INDEX)
	{
		if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
		{
			if(pMember->LinkState == LS_NO_CONNECTION)
			{
				pMember->GetDriverEntryData(&entry, pClientInfo->ClientInternalIP, pClientInfo->ClientExternalIP);
				pMember->DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
				pMember->LinkState = AddPunchHost(pMember, bActive);

				MemberUpdateList.AddData(pMember->UserID, pMember->LinkState);
			}
		}
		else
		{
			if(pMember->LinkState != LS_NO_CONNECTION)
			{
				CancelPunchHost(pMember->UserID);
				KTRemoveHost(pMember->UserID);

				pTable->SetTableItem(hAdapter, pMember->DriverMapIndex, 0); // Clean driver entry.
				pTable->RecycleMapIndex(pMember->DriverMapIndex, __LINE__);

				pMember->DriverMapIndex = INVALID_DM_INDEX;
				pMember->LinkState = LS_NO_CONNECTION;

				MemberUpdateList.AddData(pMember->UserID, LS_NO_CONNECTION);
			}
		}

		pMember->GUILinkState = pMember->LinkState;
		pMember->GUIDriverMapIndex = pMember->DriverMapIndex;
	}
	else
	{
		if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
		{
			pMember->DriverMapIndex = DriverMapIndex;
			pMember->LinkState = LinkState;
		}
		else
		{
			pMember->DriverMapIndex = INVALID_DM_INDEX;
			pMember->LinkState = LS_NO_CONNECTION;
		}

		pMember->GUILinkState = LinkState;
		pMember->GUIDriverMapIndex = DriverMapIndex;
	}

#ifdef _DEBUG
	DebugCheckState();
#endif

	return ExistCount;
}

BOOL CNetworkManager::UpdateConnectionState(HANDLE hAdapter, stClientInfo *pClientInfo, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList, BOOL bActive)
{
	stNetMember *pMember;
	USHORT DriverMapIndex, LinkState;
	CMapTable *pTable = &pClientInfo->MapTable;
	stEntry entry;

	PrepareHandshakeData();
	for(POSITION pos = pVNet->m_List.GetHeadPosition(); pos;)
	{
		pMember = &(pVNet->m_List.GetNext(pos));
		if(!pMember->IsOnline())
			continue;

		UINT ExistCount = GetConnectedStateInfo(pVNet, pMember, &DriverMapIndex, &LinkState);

		if(!ExistCount || DriverMapIndex == INVALID_DM_INDEX)
		{
			if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
			{
				if(pMember->LinkState == LS_NO_CONNECTION)
				{
					pMember->GetDriverEntryData(&entry, pClientInfo->ClientInternalIP, pClientInfo->ClientExternalIP);
					pMember->DriverMapIndex = pTable->AddTableEntry(hAdapter, &entry);
					pMember->LinkState = AddPunchHost(pMember, bActive);

					MemberUpdateList.AddData(pMember->UserID, pMember->LinkState);
				}
			}
			else
			{
				if(pMember->LinkState != LS_NO_CONNECTION)
				{
					CancelPunchHost(pMember->UserID);
					KTRemoveHost(pMember->UserID);

					pTable->SetTableItem(hAdapter, pMember->DriverMapIndex, 0); // Clean driver entry.
					pTable->RecycleMapIndex(pMember->DriverMapIndex, __LINE__);

					pMember->DriverMapIndex = INVALID_DM_INDEX;
					pMember->LinkState = LS_NO_CONNECTION;

					MemberUpdateList.AddData(pMember->UserID, pMember->LinkState);
				}
			}
		}
		else
		{
			if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
			{
				pMember->DriverMapIndex = DriverMapIndex;
				pMember->LinkState = LinkState;
			}
			else
			{
				pMember->DriverMapIndex = INVALID_DM_INDEX;
				pMember->LinkState = LS_NO_CONNECTION;
			}
		}
	}

#ifdef _DEBUG
	DebugCheckState();
#endif

	return TRUE;
}

void CNetworkManager::Release(BOOL bCloseTunnel)
{
	stVPNet *pNet;
	POSITION pos = m_NetList.GetHeadPosition();

	for(; pos;)
	{
		pNet = m_NetList.GetNext(pos);
		delete pNet;
	}
	m_NetList.RemoveAll();

	RemoveAllPunchHost();

	if(bCloseTunnel)
	{
		KTRemoveAllHost();

		if(m_UPNPPort)
		{
			RemovePortMapping(TRUE, m_UPNPPort);
			ASSERT(m_UPNPPort == 0);
		}
		CloseFirewallPort();
	}
}

UINT CNetworkManager::ExportGUIDataStream(CStreamBuffer *pStreamBuffer)
{
	UINT SizeNeeded = 0;
	POSITION pos;

	for(pos = m_NetList.GetHeadPosition(); pos;)
	{
		stVPNet *pNet = m_NetList.GetNext(pos);
		SizeNeeded += pNet->ExportGUIDataStream(pStreamBuffer);
	}

	return SizeNeeded;
}

UINT CNetworkManager::ExportRelayDataStream(CStreamBuffer *pStreamBuffer)
{
	CMapTable *pTable = &AppGetClientInfo()->MapTable;
	UINT i, nCount = pTable->m_Count, nTotal = 0;
	UINT nSizeNeeded = sizeof(nTotal);
	DWORD *pRelayNetworkIDArray = pTable->GetRelayNetworkIDArray(), UID;
//	stNetMember *pMember;

	if (pStreamBuffer == NULL)
	{
		for (i = 0; i < nCount; i++)
			if (pRelayNetworkIDArray[i])
				nSizeNeeded += sizeof(DWORD) * 2;
	}
	else
	{
		for (i = 0; i < nCount; i++)
			if (pRelayNetworkIDArray[i])
				nTotal++;

		*pStreamBuffer << nTotal;

		for(i = 0; i < nCount; i++)
			if (pRelayNetworkIDArray[i])
			{
			//	pMember = FindNetMemberByDMIndex(i);
			//	UID = pMember ? pMember->UserID : 0;
				UID = pTable->Entry[i].uid;

				*pStreamBuffer << UID;
				*pStreamBuffer << pRelayNetworkIDArray[i];
			}

		ASSERT(pStreamBuffer->GetDataSize() == pStreamBuffer->GetBufferSize());
	}

	return nSizeNeeded;
}

stVPNet* CNetworkManager::FindNet(const TCHAR *pVNetName, DWORD NetIDCode)
{
	stVPNet *pNet;
	POSITION pos;

	if (NetIDCode)
		for (pos = m_NetList.GetHeadPosition(); pos != NULL;)
		{
			pNet = m_NetList.GetNext(pos);
			if (pNet->NetIDCode == NetIDCode)
				return pNet;
		}
	else if (pVNetName != NULL)
		for (pos = m_NetList.GetHeadPosition(); pos != NULL;)
		{
			pNet = m_NetList.GetNext(pos);
			if(pNet->Name == pVNetName)
				return pNet;
		}

	return NULL;
}

void CNetworkManager::UpdateGUILinkState() // Must check all the virtual networks even if only one network changed.
{
//	UINT ExistCount;
	USHORT DriverMapIndex, LinkState;

	for (POSITION pos = m_NetList.GetHeadPosition(); pos != NULL;)
	{
		stVPNet *pNet = m_NetList.GetNext(pos);

		for(POSITION mpos = pNet->m_List.GetHeadPosition(); mpos;)
		{
			stNetMember *pMember = &pNet->m_List.GetNext(mpos);

			if(!pMember->eip.m_port)
			{
				pMember->GUILinkState = LS_OFFLINE;
				pMember->GUIDriverMapIndex = INVALID_DM_INDEX;
				continue;
			}

			GetUserExistCount(pMember->UserID, &DriverMapIndex, &LinkState);
			pMember->GUILinkState = LinkState;
			pMember->GUIDriverMapIndex = DriverMapIndex;

			ASSERT(LinkState != LS_OFFLINE);
		}
	}
}

UINT CNetworkManager::GetUserExistCount(DWORD UID, USHORT *pDriverMapIndex, USHORT *pLinkState, USHORT *pP2PLinkState)
{
	INT count = 0;
	POSITION pos, mpos;

	*pDriverMapIndex = INVALID_DM_INDEX;
	*pLinkState = LS_OFFLINE;

	for (pos = m_NetList.GetHeadPosition(); pos != NULL;)
	{
		stVPNet *pNet = m_NetList.GetNext(pos);

		for (mpos = pNet->m_List.GetHeadPosition(); mpos != NULL;)
		{
			stNetMember *pMember = &pNet->m_List.GetNext(mpos);

			if (pMember->UserID == UID)
			{
				count++;
				if (pMember->DriverMapIndex != INVALID_DM_INDEX)
				{
					ASSERT(*pDriverMapIndex == INVALID_DM_INDEX || *pDriverMapIndex == pMember->DriverMapIndex);
					*pDriverMapIndex = pMember->DriverMapIndex;
				}
				if (pMember->LinkState != LS_OFFLINE && pMember->LinkState > *pLinkState)
					*pLinkState = pMember->LinkState;
				if (pP2PLinkState != NULL)
					*pP2PLinkState = pMember->P2PLinkState;
				break;
			}
		}
	}

	return count;
}

UINT CNetworkManager::GetConnectedStateInfo(stVPNet *pSkipVNet, stNetMember *pDestMember, USHORT *pDriverMapIndex, USHORT *pLinkState)
{
	INT count = 0;
	DWORD dwUID = pDestMember->UserID;
	POSITION pos, mpos;
	stVPNet *pNet;
	stNetMember *pMember;

	*pDriverMapIndex = INVALID_DM_INDEX;
	*pLinkState = LS_NO_CONNECTION; // This function started from the premise that the user is online.

	ASSERT(pDestMember->IsOnline());
	ASSERT(pSkipVNet->IsMember(pDestMember));

	for(pos = m_NetList.GetHeadPosition(); pos != NULL;)
	{
		if((pNet = m_NetList.GetNext(pos)) == pSkipVNet)
			continue;

		for(mpos = pNet->m_List.GetHeadPosition(); mpos;)
		{
			pMember = &pNet->m_List.GetNext(mpos);
			if(pMember->UserID != dwUID)
				continue;

			if(pMember->DriverMapIndex != INVALID_DM_INDEX)
			{
				ASSERT(*pDriverMapIndex == INVALID_DM_INDEX || *pDriverMapIndex == pMember->DriverMapIndex);
				*pDriverMapIndex = pMember->DriverMapIndex;
			}
			if(pMember->LinkState > *pLinkState)
				*pLinkState = pMember->LinkState;
			count++;
			break;
		}
	}

	return count;
}

void CNetworkManager::DebugCheckState()
{
	UINT nCount;
	POSITION pos, mpos;

	for(pos = m_NetList.GetHeadPosition(); pos;)
	{
		stVPNet *pNet = m_NetList.GetNext(pos);
		nCount = (pNet->Flag & VF_RELAY_HOST) ? 1 : 0;

		for(mpos = pNet->m_List.GetHeadPosition(); mpos;)
		{
			stNetMember *pMember = &pNet->m_List.GetNext(mpos);

			if(pMember->Flag & VF_RELAY_HOST)
				nCount++;
			if(!pMember->IsOnline())
			{
				ASSERT(pMember->DriverMapIndex == INVALID_DM_INDEX && pMember->LinkState == LS_OFFLINE);
				continue;
			}

			if(!pNet->IsNetOnline() || !pMember->IsNetOnline() || !NeedConnect(pNet, pMember))
			{
				ASSERT(pMember->DriverMapIndex == INVALID_DM_INDEX && pMember->LinkState == LS_NO_CONNECTION);
				if(pMember->DriverMapIndex != INVALID_DM_INDEX || pMember->LinkState != LS_NO_CONNECTION)
					printx("Net(%S) member(%S) status failed! Index: %d. Link state: %d.\n", pNet->Name, pMember->HostName, pMember->DriverMapIndex, pMember->LinkState);
			}
		}

	//	ASSERT(!nCount || nCount == 1);
	}
}

BOOL CNetworkManager::AddNewMember(CString NetName, stNetMember *pMember)
{
	stVPNet *pNet;
	POSITION pos;

	for(pos = m_NetList.GetHeadPosition(); pos;)
	{
		pNet = m_NetList.GetNext(pos);

		if(pNet->Name == NetName)
		{
			pNet->m_List.AddTail(*pMember);
			return TRUE;
		}
	}

	return 0;
}

INT CNetworkManager::FindNetMember(DWORD uid, CList<stNetMemberInfo> *pList, BOOL &bNeedPTT)
{
	ASSERT(pList && !pList->GetCount());

	POSITION npos, mpos;
	stVPNet *pVNet;
	stNetMember *pMember;
	stNetMemberInfo info;
	INT nCount = 0;

	bNeedPTT = FALSE;

	for(npos = m_NetList.GetHeadPosition(); npos;)
	{
		pVNet = m_NetList.GetNext(npos);
		for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
		{
			pMember = &(pVNet->m_List.GetNext(mpos));

			if(pMember->UserID == uid)
			{
				if(pMember->IsNetOnline() && pVNet->IsNetOnline() && NeedConnect(pVNet, pMember))
					bNeedPTT = TRUE;

				info.pVNet = pVNet;
				info.pMember = pMember;
				pList->AddTail(info);
				nCount++;
				break;
			}
		}
	}

	return nCount;
}

stNetMember* CNetworkManager::FindNetMember(DWORD_PTR uid)
{
	POSITION npos, mpos;
	stVPNet *pNet;
	stNetMember *pMember;

	for(npos = m_NetList.GetHeadPosition(); npos;)
	{
		pNet = m_NetList.GetNext(npos);
		for(mpos = pNet->m_List.GetHeadPosition(); mpos;)
		{
			pMember = &(pNet->m_List.GetNext(mpos));
			if(pMember->UserID == uid)
				return pMember;
		}
	}

	return 0;
}

//stNetMember* CNetworkManager::FindNetMemberByDMIndex(USHORT DMIndex)
//{
//	POSITION npos, mpos;
//	stVPNet *pNet;
//	stNetMember *pMember;
//
//	for(npos = m_NetList.GetHeadPosition(); npos;)
//	{
//		pNet = m_NetList.GetNext(npos);
//		for(mpos = pNet->m_List.GetHeadPosition(); mpos;)
//		{
//			pMember = &(pNet->m_List.GetNext(mpos));
//			if(pMember->DriverMapIndex == DMIndex)
//				return pMember;
//		}
//	}
//
//	return 0;
//}

//BOOL CNetworkManager::FindRelayHost(stVPNet *pVNet, stNetMember **pDestMember)
//{
//	POSITION pos;
//	stNetMember *pMember;
//
//	*pDestMember = 0;
//	if(pVNet->Flag & VF_RELAY_HOST)
//		return TRUE;
//
//	for(pos = pVNet->m_List.GetHeadPosition(); pos;)
//	{
//		pMember = &(pVNet->m_List.GetNext(pos));
//		if(pMember->Flag & VF_RELAY_HOST)
//		{
//			*pDestMember = pMember;
//			return TRUE;
//		}
//	}
//
//	return FALSE;
//}

void CNetworkManager::SwitchRelayMode(HANDLE hAdapter, DWORD uid, BOOL bServerRelay)
{
	UINT i;
	CMapTable *pMapTable = &AppGetClientInfo()->MapTable;

	if(bServerRelay) // New relay mode.
	{
		for(i = 0; i < pMapTable->m_Count; ++i)
			if(pMapTable->Entry[i].uid == uid)
			{
				pMapTable->SetRelayHostIndexInfo(hAdapter, pMapTable->GetRelayNetworkID(i), i, FALSE, 0, 0);
				break;
			}
	}
	else
	{
		pMapTable->UpdateServerRelayFlag(hAdapter, FALSE, uid);
	}
}

BOOL CNetworkManager::RequestHostRelay(DWORD NetID, DWORD RequestUID, DWORD DestUID, USHORT &SrcDMIndex, USHORT &DestDMIndex)
{
	stVPNet *pVNet = 0, *pTemp;
	SrcDMIndex = DestDMIndex = INVALID_DM_INDEX;
	for(POSITION pos = m_NetList.GetHeadPosition(); pos;)
	{
		pTemp = m_NetList.GetNext(pos);
		if(pTemp->NetIDCode == NetID)
		{
			pVNet = pTemp;
			break;
		}
	}
	if(!pVNet || !(pVNet->Flag & VF_RELAY_HOST))
		return FALSE;

	stNetMember *pSrcHost = pVNet->FindHost(RequestUID);
	stNetMember *pDestHost = pVNet->FindHost(DestUID);

	// Relay host must connect two peers directly.
	if(!pSrcHost || !pDestHost || !pSrcHost->IsOnline() || pSrcHost->LinkState != LS_CONNECTED || !pDestHost->IsOnline() || pDestHost->LinkState != LS_CONNECTED)
		return FALSE;

	// Check network role.
	if(pVNet->NetworkType == VNT_HUB_AND_SPOKE && !(pSrcHost->Flag & VF_HUB) && !(pDestHost->Flag & VF_HUB))
		return FALSE;

	IPV4 SrcVip = pSrcHost->vip, DestVip = pDestHost->vip;
	printx("Prepare relay for host: %d.%d.%d.%d and %d.%d.%d.%d\n", SrcVip.b1, SrcVip.b2, SrcVip.b3, SrcVip.b4, DestVip.b1, DestVip.b2, DestVip.b3, DestVip.b4);

	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hAdapter = OpenAdapter();
	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		SrcDMIndex = pSrcHost->DriverMapIndex;
		DestDMIndex = pDestHost->DriverMapIndex;
		pClientInfo->MapTable.UpdateTableFlag(hAdapter, SrcDMIndex, AIF_RH_RIGHT);
		pClientInfo->MapTable.UpdateTableFlag(hAdapter, DestDMIndex, AIF_RH_RIGHT);

		CloseAdapter(hAdapter);
	}

	return TRUE;
}

BOOL CNetworkManager::SetRelayHostIndex(DWORD NetID, DWORD SrcUID, DWORD DestUID, DWORD RHUID, USHORT SrcIndex, USHORT DestIndex)
{
	BOOL bResult = FALSE;
	stVPNet *pNet = 0, *pTemp;
	for(POSITION pos = m_NetList.GetHeadPosition(); pos;)
	{
		pTemp = m_NetList.GetNext(pos);
		if(pTemp->NetIDCode == NetID)
		{
			pNet = pTemp;
			break;
		}
	}
	if(!pNet)
		return FALSE;

	stClientInfo *pClientInfo = AppGetClientInfo();
	stNetMember *pRelayHost = pNet->FindHost(RHUID), *pHost = 0;
	DWORD SelfUID = pClientInfo->ID1;
	if(SelfUID == SrcUID)
		pHost = pNet->FindHost(DestUID);
	else if(SelfUID == DestUID)
	{
		pHost = pNet->FindHost(SrcUID);
		DestIndex = SrcIndex;
	}

	if(!pRelayHost || !(pRelayHost->Flag & VF_RELAY_HOST) || !pHost)
		return FALSE;

	HANDLE hAdapter = OpenAdapter();
	if(hAdapter != INVALID_HANDLE_VALUE)
	{
		SwitchRelayMode(hAdapter, pHost->UserID, FALSE);
		pClientInfo->MapTable.SetRelayHostIndexInfo(hAdapter, pNet->NetIDCode, pHost->DriverMapIndex, TRUE, pRelayHost->DriverMapIndex, DestIndex);
		CloseAdapter(hAdapter);
	}

	UpdataMemberLinkState(pHost->UserID, LS_RELAYED_TUNNEL);

	return TRUE;
}

void CNetworkManager::CancelVNetRelay(HANDLE hAdapter, CMapTable *pTable, stVPNet *pVNet, CMemberUpdateList &MemberUpdateList)
{
	USHORT DriverMapIndex;
	DWORD dwNetID = pVNet->NetIDCode;

	for(POSITION pos = pVNet->m_List.GetHeadPosition(); pos;)
	{
		stNetMember *pMember = &pVNet->m_List.GetNext(pos);

		DriverMapIndex = pMember->DriverMapIndex;
		if(!pMember->IsOnline() || DriverMapIndex == INVALID_DM_INDEX || pTable->GetRelayNetworkID(DriverMapIndex) != dwNetID)
			continue;

		pTable->SetRelayHostIndexInfo(hAdapter, dwNetID, DriverMapIndex, FALSE, 0, 0);
		UpdataMemberLinkState(pMember->UserID, 0, TRUE, &MemberUpdateList);
	}
}

void CNetworkManager::CancelRelayHost(stVPNet *pVNet, stNetMember *pRelayHost, CMemberUpdateList &MemberUpdateList, BOOL bCheckRole)
{
	USHORT DriverMapIndex, RHDMIndex = pRelayHost->DriverMapIndex;
	stNetMember *pMember;

	HANDLE hAdapter = OpenAdapter();
	CMapTable *pTable = &AppGetClientInfo()->MapTable;

	for(POSITION pos = pVNet->m_List.GetHeadPosition(); pos;)
	{
		pMember = &pVNet->m_List.GetNext(pos);
		if(!pMember->IsOnline() || pMember->DriverMapIndex == INVALID_DM_INDEX)
			continue;

		if(bCheckRole) // The role of the relay host was changed from hub to spoke.
		{
			ASSERT(pVNet->NetworkType == VNT_HUB_AND_SPOKE);
			if((pVNet->Flag & VF_HUB) && (pMember->Flag & VF_HUB)) // The local host and remote host is still connected with relay host.
				continue;
		}

		DriverMapIndex = pMember->DriverMapIndex;
		if(!pTable->IsUsingRelay(DriverMapIndex, pVNet->NetIDCode, RHDMIndex))
			continue;

		pTable->SetRelayHostIndexInfo(hAdapter, pVNet->NetIDCode, DriverMapIndex, FALSE, 0, 0);
		UpdataMemberLinkState(pMember->UserID, 0, TRUE, &MemberUpdateList);
	}

	CloseAdapter(hAdapter);
}

BOOL CNetworkManager::CancelRelay(HANDLE hAdapter, CMapTable *pTable, stNetMember *pMember, DWORD NetID, CMemberUpdateList &MemberUpdateList)
{
	USHORT DriverMapIndex = pMember->DriverMapIndex;
	if(DriverMapIndex == INVALID_DM_INDEX || pTable->GetRelayNetworkID(DriverMapIndex) != NetID)
		return FALSE;

	pTable->SetRelayHostIndexInfo(hAdapter, NetID, DriverMapIndex, FALSE, 0, 0);
	UpdataMemberLinkState(pMember->UserID, 0, TRUE, &MemberUpdateList);

	return TRUE;
}

BOOL CNetworkManager::GetRelayHost(CMapTable *pTable, stVPNet *pVNet, stNetMember *pMember, stNetMember **pRelayHost)
{
	USHORT DriverMapIndex = pMember->DriverMapIndex;

	if(DriverMapIndex == INVALID_DM_INDEX || pTable->GetRelayNetworkID(DriverMapIndex) != pVNet->NetIDCode)
		return FALSE;

	stEntry *pEntry = &pTable->Entry[DriverMapIndex];
	if(!(pEntry->flag & AIF_RELAY_HOST))
		return FALSE;

	*pRelayHost = pVNet->FindHost(pTable->Entry[pEntry->rhindex].uid);
	if(!*pRelayHost)
		return FALSE;

	return TRUE;
}

UINT CNetworkManager::UpdateMemberProfile(DWORD UserID, UINT nType, void* pData, UINT nDataSize)
{
	UINT nFound = 0;
	stVPNet *pVNet;
	stNetMember *pMember;
	POSITION npos, mpos;

	for(npos = m_NetList.GetHeadPosition(); npos;)
	{
		pVNet = m_NetList.GetNext(npos);

		for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
		{
			pMember = &(pVNet->m_List.GetNext(mpos));
			if(pMember->UserID != UserID)
				continue;

			switch(nType)
			{
				case CPT_HOST_NAME:
					pMember->HostName = (TCHAR*)pData;
					break;
			}

			++nFound;
			break;
		}
	}

	return nFound;
}

BOOL CNetworkManager::UpdateMemberRole(DWORD NetID, DWORD UserID, DWORD dwFlag, DWORD dwMask, stVPNet *&pDestVNet, stNetMember *&pDestMember, BOOL &bRelayHostCancelled)
{
	INT bFound = FALSE;
	POSITION npos, mpos;
	stVPNet *pVNet;
	stNetMember *pMember;
	USHORT flag = (USHORT)dwFlag, mask = (USHORT)dwMask;

	bRelayHostCancelled = FALSE;
	ASSERT(!pDestVNet && !pDestMember);

	if(dwMask == VF_RELAY_HOST)
	{
		if(dwFlag & VF_RELAY_HOST) // Set as relay host.
		{
			for(npos = m_NetList.GetHeadPosition(); npos;) // Find target.
			{
				pVNet = m_NetList.GetNext(npos);
				if(pVNet->NetIDCode != NetID)
					continue;
				pDestVNet = pVNet;
				if(UserID == AppGetClientInfo()->ID1) // Test self.
				{
					pVNet->Flag |= VF_RELAY_HOST;
					bFound = TRUE;
				}
				else
					for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
					{
						pMember = &(pVNet->m_List.GetNext(mpos));
						if(pMember->UserID != UserID)
							continue;
						pMember->Flag |= VF_RELAY_HOST;
						pDestMember = pMember;
						bFound = TRUE;
						break;
					}

				//if(bFound) // Remove other one.
				//{
				//	if(UserID != AppGetClientInfo()->ID1 && pVNet->Flag & VF_RELAY_HOST) // Test self.
				//	{
				//		pVNet->Flag &= ~VF_RELAY_HOST;
				//		bRelayHostCancelled = TRUE;
				//	}
				//	else
				//		for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
				//		{
				//			pMember = &(pVNet->m_List.GetNext(mpos));
				//			if(pMember->UserID != UserID && pMember->Flag & VF_RELAY_HOST)
				//			{
				//				pMember->Flag &= ~VF_RELAY_HOST;
				//				bRelayHostCancelled = TRUE;
				//				break;
				//			}
				//		}
				//}
				break;
			}
		}
		else // Cancel relay host.
		{
			for(npos = m_NetList.GetHeadPosition(); npos;)
			{
				pVNet = m_NetList.GetNext(npos);
				if(pVNet->NetIDCode != NetID)
					continue;
				pDestVNet = pVNet;
				if(UserID == AppGetClientInfo()->ID1) // Test self.
				{
					pVNet->Flag &= ~VF_RELAY_HOST;
					bFound = TRUE;
					bRelayHostCancelled = TRUE;
				}
				else
					for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
					{
						pMember = &(pVNet->m_List.GetNext(mpos));
						if(pMember->UserID != UserID)
							continue;
						pMember->Flag &= ~VF_RELAY_HOST;
						pDestMember = pMember;
						bFound = TRUE;
						bRelayHostCancelled = TRUE;
						break;
					}
				break;
			}
		}
	}
	else if(dwMask == VF_ALLOW_RELAY && !(dwFlag & VF_ALLOW_RELAY))
	{
		for(npos = m_NetList.GetHeadPosition(); npos;)
		{
			pVNet = m_NetList.GetNext(npos);
			if(pVNet->NetIDCode != NetID)
				continue;
			pDestVNet = pVNet;
			if(UserID == AppGetClientInfo()->ID1) // Test self.
			{
				pVNet->Flag &= ~(VF_ALLOW_RELAY | VF_RELAY_HOST);
				bFound = TRUE;
				bRelayHostCancelled = TRUE;
			}
			else
				for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
				{
					pMember = &(pVNet->m_List.GetNext(mpos));
					if(pMember->UserID != UserID)
						continue;
					pMember->Flag &= ~(VF_ALLOW_RELAY | VF_RELAY_HOST);
					pDestMember = pMember;
					bFound = TRUE;
					bRelayHostCancelled = TRUE;
					break;
				}
			break;
		}
	}
	else
		for(npos = m_NetList.GetHeadPosition(); npos;)
		{
			pVNet = m_NetList.GetNext(npos);
			if(pVNet->NetIDCode != NetID)
				continue;
			pDestVNet = pVNet;
			if(UserID == AppGetClientInfo()->ID1) // Test self.
			{
				pVNet->Flag &= ~mask;
				pVNet->Flag |= (flag & mask);
				bFound = TRUE;
				break;
			}

			for(mpos = pVNet->m_List.GetHeadPosition(); mpos;)
			{
				pMember = &(pVNet->m_List.GetNext(mpos));
				if(pMember->UserID != UserID)
					continue;
				pMember->Flag &= ~mask;
				pMember->Flag |= (flag & mask);
				pDestMember = pMember;
				bFound = TRUE;
				break; // Only one host can exist in the VLAN.
			}
			break;
		}

	return bFound;
}

INT CNetworkManager::UpdataMemberLinkState(DWORD uid, USHORT dwLinkState, BOOL bRestore, CMemberUpdateList *pMemberUpdateList, USHORT *pP2PLinkState)
{
	INT nCount = 0;

	for(POSITION npos = m_NetList.GetHeadPosition(); npos;)
	{
		stVPNet *pVNet = m_NetList.GetNext(npos);
		if(!pVNet->IsNetOnline())
			continue;

		for(POSITION mpos = pVNet->m_List.GetHeadPosition(); mpos;)
		{
			stNetMember *pMember = &(pVNet->m_List.GetNext(mpos));
			if(pMember->UserID != uid)
				continue;
			
			if(pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
			{
				if(bRestore)
				{
					pMember->LinkState = pMember->P2PLinkState;
					if(pP2PLinkState)
						*pP2PLinkState = pMember->P2PLinkState;
				}
				else
					pMember->LinkState = dwLinkState;

				if(pMemberUpdateList)
					pMemberUpdateList->AddData(uid, pMember->LinkState);

				++nCount;
			}

			break; // Only one host can exist in the VLAN.
		}
	}

	return nCount;
}

INT CNetworkManager::UpdateP2PLinkState(DWORD uid, USHORT dwLinkState) // Update all p2p link state of the member.
{
	INT nCount = 0;

	ASSERT(dwLinkState == LS_CONNECTED || dwLinkState == LS_NO_TUNNEL);

	for(POSITION npos = m_NetList.GetHeadPosition(); npos;)
	{
		stVPNet *pVNet = m_NetList.GetNext(npos);
	//	if(!pVNet->IsNetOnline()) // Don't do this check.
	//		continue;

		for(POSITION mpos = pVNet->m_List.GetHeadPosition(); mpos;)
		{
			stNetMember *pMember = &(pVNet->m_List.GetNext(mpos));

			if(pMember->UserID == uid)
			{
				pMember->P2PLinkState = dwLinkState;
				++nCount;

				if(pVNet->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVNet, pMember))
					pMember->LinkState = dwLinkState;

				break; // Only one host can exist in the VLAN.
			}
		}
	}

	return nCount;
}

INT CNetworkManager::UpdateMemberExternalAddress(DWORD uid, CIpAddress eip, USHORT eport)
{
	INT nCount = 0;
	POSITION npos, mpos;
	stVPNet *pNet;
	stNetMember *pMember;

	for(npos = m_NetList.GetHeadPosition(); npos;)
	{
		pNet = m_NetList.GetNext(npos);
		for(mpos = pNet->m_List.GetHeadPosition(); mpos;)
		{
			pMember = &(pNet->m_List.GetNext(mpos));

			if(pMember->UserID == uid)
			{
				pMember->eip = eip;
				pMember->eip.m_port = eport;
				++nCount;
				break; // Only one host can exist in the VLAN.
			}
		}
	}

	return nCount;
}

INT CNetworkManager::AddPunchHost(stNetMember *pNetMember, BOOL bActive)
{
	ASSERT(!pNetMember->eip.IsZeroAddress() && pNetMember->DriverMapIndex != INVALID_DM_INDEX);

	stClientInfo *pClientInfo = AppGetClientInfo();
	stNatPunchInfo info;
	info.flag = bActive ? TPTF_ACTIVE : TPTF_NULL;

	if(pClientInfo->bInNat)
		info.flag |= TPTF_SRC_NAT;
	if(pNetMember->bInNat)
		info.flag |= TPTF_DES_NAT;
	if(pClientInfo->bEipFailed)
		info.flag |= TPTF_FAILED_EIP;

	if(pNetMember->eip == pClientInfo->ClientExternalIP) // In the same LAN.
	{
		printx("In the same lan.\n");
		info.flag |= (TPTF_SAME_LAN | TPTF_USE_IIP);
		pNetMember->GetPunchThroughInfo(info, TRUE);
	}
	else
	{
		pNetMember->GetPunchThroughInfo(info, FALSE);
	}

	DOUBLE dServerTime = pClientInfo->GetServerTime();
	UINT64 nServerTime = (INT64)dServerTime;
	UINT64 nTimeStamp = nServerTime + (1000 - (nServerTime % 1000)) + 1000;

	if(m_nLastAssignedTime >= nTimeStamp && m_nLastAssignedTime - nTimeStamp < 500000) // 500 seconds.
	{
		if(m_AssignedCount == MAX_HOST_PER_TIMESTAMP)
		{
			m_nLastAssignedTime += 1000;
			m_AssignedCount = 1;
		}
		else
			m_AssignedCount++;

		nTimeStamp = m_nLastAssignedTime;
	}
	else
	{
		m_nLastAssignedTime = nTimeStamp;
		m_AssignedCount = 1;
	}
	info.nStartPunchTime = nTimeStamp;
	printx("Current Server Time: %f. Start Punch Time: %d.\n", dServerTime, info.nStartPunchTime);

	// Must add to the tail so we can make sure first punching time is larger in the tail of the list.
	m_FirstTimeList.AddTail(info);

	// Add to handshake list.
	stPTHandshakeInfo HSInfo;
	HSInfo.DriverMapIndex = info.DriverMapIndex;
	HSInfo.Key = (DWORD_PTR)pNetMember; // Must match value of pEntry->uid in stNetMember::GetDriverEntryData.
	HSInfo.UID = info.uid;
	HSInfo.Flag = info.flag;
	HSInfo.nStartTime = info.nStartPunchTime;
	m_TempList.AddTail(HSInfo);

#ifdef DEBUG

	POSITION pos;
	UINT nCount = 0;
	// Check first time list.
	for(pos = m_FirstTimeList.GetHeadPosition(); pos;)
		if(m_FirstTimeList.GetNext(pos).uid == info.uid)
			nCount++;
	for(pos = m_HostList.GetHeadPosition(); pos;)
		if(m_HostList.GetNext(pos).uid == info.uid)
			nCount++;
	ASSERT(nCount == 1);

#endif

	return LS_TRYING_TPT;
}

UINT CNetworkManager::CancelPunchHost(DWORD uid, stNatPunchInfo *pOut)
{
	UINT nCount = 0;
	POSITION cpos, pos;
	stNatPunchInfo *pInfo;

	for(pos = m_FirstTimeList.GetHeadPosition(); pos;)
	{
		cpos = pos;
		pInfo = &m_FirstTimeList.GetNext(pos);

		if(pInfo->uid == uid)
		{
			if(pOut)
				*pOut = *pInfo;
			printx("Punch host canceled.\n");
			m_FirstTimeList.RemoveAt(cpos);
			++nCount;
		}
	}
	for(pos = m_HostList.GetHeadPosition(); pos;)
	{
		cpos = pos;
		pInfo = &m_HostList.GetNext(pos);

		if(pInfo->uid == uid)
		{
			if(pOut)
				*pOut = *pInfo;
			printx("Punch host canceled.\n");
			m_HostList.RemoveAt(cpos);
			++nCount;
		}
	}

	ASSERT(nCount <= 1);

	return nCount;
}

BOOL CNetworkManager::PTSetAck(DWORD uid)
{
	POSITION pos;
	stNatPunchInfo *pInfo;
	BOOL bFound = FALSE;

	for(pos = m_HostList.GetHeadPosition(); pos;)
	{
		pInfo = &m_HostList.GetNext(pos);
		if(pInfo->uid == uid)
		{
			pInfo->flag |= TPTF_ACKED;
			bFound = TRUE;
			break;
		}
	}

//	ASSERT(bFound);
	return bFound;
}

INT CNetworkManager::PTSetInitData(DWORD uid, UINT DMIndex, DWORD key, UINT64 nStartTime)
{
	POSITION oldpos, pos;
	stNatPunchInfo *pInfo;
	INT bFound = 0;
	HANDLE hAdapter = OpenAdapter();

	for(pos = m_FirstTimeList.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pInfo = &m_FirstTimeList.GetNext(pos);
		if(pInfo->uid == uid)
		{
			bFound = 1;
			AppGetClientInfo()->MapTable.SetTableEntryDIndex(hAdapter, pInfo->DriverMapIndex, DMIndex);
			ASSERT(!pInfo->PeerDMIndex && !pInfo->key);
			pInfo->PeerDMIndex = DMIndex; // This value could be zero.
			pInfo->key = key;

			printx("Self time: %lld. Peer time: %lld.\n", pInfo->nStartPunchTime, nStartTime);
			if(nStartTime > pInfo->nStartPunchTime)
			{
				pInfo->nStartPunchTime = nStartTime;

				for(POSITION tpos = pos, tailpos = m_FirstTimeList.GetTailPosition(); tpos; m_FirstTimeList.GetNext(tpos)) // Re-order.
				{
					if(m_FirstTimeList.GetAt(tpos).nStartPunchTime >= nStartTime)
					{
						if(tpos == pos)
							break;
						tpos = m_FirstTimeList.InsertBefore(tpos, *pInfo);
						m_FirstTimeList.RemoveAt(oldpos);
						pInfo = &m_FirstTimeList.GetAt(tpos);
						break;
					}
					if(tpos == tailpos)
					{
						tpos = m_FirstTimeList.AddTail(*pInfo);
						m_FirstTimeList.RemoveAt(oldpos);
						pInfo = &m_FirstTimeList.GetAt(tpos);
						break;
					}
				}
			}
			printx("Set TPT data for vip: %d.%d.%d.%d Time: %lld\n", pInfo->vip.b1, pInfo->vip.b2, pInfo->vip.b3, pInfo->vip.b4, pInfo->nStartPunchTime);
			break;
		}
	}

	CloseAdapter(hAdapter);

	if(bFound)
	{
		// Check list time order.
		UINT64 ctime = m_FirstTimeList.GetHead().nStartPunchTime;
		for(pos = m_FirstTimeList.GetHeadPosition(); pos;)
		{
			stNatPunchInfo &Info = m_FirstTimeList.GetNext(pos);

			if(Info.nStartPunchTime > ctime)
			{
				ctime = Info.nStartPunchTime;
			}
			else if(Info.nStartPunchTime < ctime && ctime - Info.nStartPunchTime <= 5000000) // Prevent overflow.
			{
				ASSERT(0);
				bFound = -1;
				break;
			}
		}
	}

	return bFound;
}

void CNetworkManager::RemoveAllPunchHost()
{
	m_FirstTimeList.RemoveAll();
	m_HostList.RemoveAll();
}

void CNetworkManager::CheckTable(HANDLE hAdapter, DWORD *UIDArray, UINT &nCount, UINT &nReportCount)
{
	ASSERT(!nCount && !nReportCount);
	if(!m_HostList.GetCount())
		return;

	IPV4 ip;
	USHORT DriverFlag;
	CIpAddress tempIP;
	POSITION pos, oldpos;
	stNatPunchInfo *pInfo;
	stClientInfo *pClientInfo = AppGetClientInfo();
	CMapTable *pMapTable = &(pClientInfo->MapTable);

	// Read data.
	if(m_TunnelSocketMode == TSM_KERNEL_MODE)
		pMapTable->GetTableData(hAdapter, 0);

//	m_cs.EnterCriticalSection();
	for(pos = m_HostList.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pInfo = &m_HostList.GetNext(pos);
		if(!pInfo->key)
			continue;
		ip = pInfo->vip;
		DriverFlag = pMapTable->Entry[pInfo->DriverMapIndex].flag;

		if((DriverFlag & AIF_ACK_RECEIVED) && !(pInfo->flag & TPTF_RECV_HELLO))
		{
			UIDArray[nCount++] = pInfo->uid;
			pInfo->flag |= TPTF_RECV_HELLO;

			// Check if eip and eport is updated for symmetric NAT.
			memcpy(&tempIP, &pMapTable->Entry[pInfo->DriverMapIndex].pip, sizeof(IpAddress));
			tempIP.SetIPV6(DriverFlag & AIF_IPV6);
			tempIP.m_port = pMapTable->Entry[pInfo->DriverMapIndex].port;
			if(tempIP != pInfo->eip || tempIP.m_port != pInfo->eip.m_port)
			{
				pInfo->flag |= TPTF_MOD_ADDR;
				UpdateMemberExternalAddress(pInfo->uid, tempIP, NBPort(tempIP.m_port));
				printx("Address updated for vip:%d.%d.%d.%d\nFrom %d.%d.%d.%d:%d to %d.%d.%d.%d:%d.\n", ip.b1, ip.b2, ip.b3, ip.b4,
					pInfo->eip.IPV4[0], pInfo->eip.IPV4[1], pInfo->eip.IPV4[2], pInfo->eip.IPV4[3], NBPort(pInfo->eip.m_port),
					tempIP.IPV4[0], tempIP.IPV4[1], tempIP.IPV4[2], tempIP.IPV4[3], NBPort(tempIP.m_port));
				pInfo->eip = tempIP;
			}
		}

	//	if(pInfo->flag & (TPTF_RECV_HELLO | TPTF_ACKED)) // Logical error.
		if(pInfo->flag & TPTF_RECV_HELLO && pInfo->flag & TPTF_ACKED)
		{
			// Connection success. pInfo->DriverMapIndex and pInfo->nTryCount become invalid, don't access them.
			printx("Punch-through successfully for vip:%d.%d.%d.%d.\n", ip.b1, ip.b2, ip.b3, ip.b4);

			if(pClientInfo->ServerCtrlFlag & SCF_FMAC_INDEX)
				ArpDeleteAddress(pInfo->DriverMapIndex, pInfo->vip);

			pMapTable->UpdateTableFlag(hAdapter, pInfo->DriverMapIndex, AIF_PT_FIN | AIF_OK, AIF_ACK_RECEIVED | AIF_PENDING);
			pMapTable->UpdateUID(hAdapter, pInfo->DriverMapIndex, pInfo->uid);
			UpdateP2PLinkState(pInfo->uid, LS_CONNECTED); // Update LinkState of the Host.

		//	if(pClientInfo->bInNat || pClientInfo->bInFirewall)
				KTAddHost(pInfo, FALSE);

			// Notify GUI.
			stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_MEMBER | stGUIEventMsg::UF_DWORD1);
			pMsg->dwResult = 1;
			pMsg->member.dwUserID = pInfo->uid;
			pMsg->member.DriverMapIndex = pInfo->DriverMapIndex;
			pMsg->member.eip = pInfo->eip;
			pMsg->member.eip.m_port = NBPort(pInfo->eip.m_port);
			NotifyGUIMessage(GET_UPDATE_CONNECT_STATE, pMsg);

			ReportPTResult(pClientInfo, TRUE, pInfo);
			m_HostList.RemoveAt(oldpos); // Remove at last.
			continue;
		}

		pInfo->nTryCount--;
		if((NAT_PUNCH_THROUGH_TRY_COUNT - pInfo->nTryCount) == NAT_PUNCH_THROUGH_IIP && pInfo->flag & TPTF_USE_IIP) // Test if nat support loopback.
		{
			printx("Detect with eip!\n");
			pInfo->eip = pInfo->ip;
			pInfo->flag &= ~TPTF_USE_IIP;
		}
		if(!pInfo->nTryCount)
		{
			// Connection failure.
			printx("Punch-through failed for vip:%d.%d.%d.%d.\n", pInfo->vip.b1, pInfo->vip.b2, pInfo->vip.b3, pInfo->vip.b4);
			ASSERT(pInfo->DriverMapIndex < pMapTable->m_Count /*&& !(pInfo->flag & TPTF_RECV_HELLO) && !(pInfo->flag & TPTF_ACKED)*/);
			pMapTable->UpdateTableFlag(hAdapter, pInfo->DriverMapIndex, AIF_PT_FIN, AIF_START_PT | AIF_PENDING);
			pMapTable->UpdateUID(hAdapter, pInfo->DriverMapIndex, pInfo->uid);
			UpdateP2PLinkState(pInfo->uid, LS_NO_TUNNEL); // Update LinkState of the Host.

			// Notify GUI.
			stGUIEventMsg *pMsg = AllocGUIEventMsg(stGUIEventMsg::UF_MEMBER | stGUIEventMsg::UF_DWORD1);
			pMsg->dwResult = 0;
			pMsg->member.dwUserID = pInfo->uid;
			pMsg->member.DriverMapIndex = pInfo->DriverMapIndex;
			NotifyGUIMessage(GET_UPDATE_CONNECT_STATE, pMsg);

			ReportPTResult(pClientInfo, FALSE, pInfo);
			m_HostList.RemoveAt(oldpos); // Remove at last.
		}
		//printx("Try count: %d.\n", pInfo->nTryCount);
	}

	nReportCount = m_ReportList.GetCount();
//	m_cs.LeaveCriticalSection();
}

void CNetworkManager::PunchOnce(HANDLE hDevice)
{
	ASSERT(sizeof(stHelloPacket) == 12 && hDevice); // Make sure the size of stHelloPacket is the same with driver.

	POSITION pos, oldpos;
	stNatPunchInfo *pInfo;
	stHelloPacket packet;
	stClientInfo *pClientInfo = AppGetClientInfo();
	memcpy(packet.vmac, pClientInfo->vmac, sizeof(packet.vmac));

	for(pos = m_HostList.GetHeadPosition(); pos;)
	{
		pInfo = &m_HostList.GetNext(pos);
		if(!pInfo->key || pInfo->flag & TPTF_ACKED)
			continue;
		printx("Punch once for vip: %d.%d.%d.%d.\n", pInfo->vip.b1, pInfo->vip.b2, pInfo->vip.b3, pInfo->vip.b4);

		packet.Index = pInfo->PeerDMIndex;
		packet.key = pInfo->key;
		DirectSend(hDevice, pInfo->eip, pInfo->eip.IsIPV6(), pInfo->eip.m_port, &packet, sizeof(packet));
	}

	if(!m_FirstTimeList.GetCount())
		return;

	BOOL bMissTimeStamp = FALSE;
	DOUBLE dServerTime, dEntryServerTime = pClientInfo->GetServerTime();
	UINT64 nWorkTime = (UINT64)dEntryServerTime;
	UINT WaitTime = 1000 - (nWorkTime % 1000);

	if(WaitTime > 500) // The timer gets large delay occasionally.
	{
		dServerTime = dEntryServerTime;
		nWorkTime = nWorkTime + WaitTime - 1000;
		bMissTimeStamp = TRUE;
	}
	else
	{
		nWorkTime = nWorkTime + WaitTime;

		while(1)
		{
			dServerTime = pClientInfo->GetServerTime();
			if(dServerTime + 20 < (DOUBLE)nWorkTime) // Don't waste too much cpu time so reserve 20 ms. Windows timer default precision: 10 ~ 16 ms.
				Sleep(1);
			else
				break;
		}
		while(1)
		{
			dServerTime = pClientInfo->GetServerTime();
			if(dServerTime < (DOUBLE)nWorkTime)
				Sleep(0);
			else
				break;
		}
	}

	for(pos = m_FirstTimeList.GetHeadPosition(); pos;) // The following loop is time critical. Must complete as fast as possible.
	{
		pInfo = &m_FirstTimeList.GetNext(pos);

		if(pInfo->nStartPunchTime > nWorkTime)
			break; // All hosts in the list wait more time.

		if(!pInfo->key || pInfo->flag & TPTF_ACKED /*|| GetKeyState(VK_F2) < 0*/) // Press f2 to force delay on purpose.
			continue;

		if(nWorkTime != pInfo->nStartPunchTime || bMissTimeStamp)
		{
			printx("First punch miss time stamp!\n");
			pInfo->flag |= TPTF_MISS_TIME;
		}
		ASSERT(!(pInfo->flag & TPTF_F_PUNCH));
		pInfo->flag |= TPTF_F_PUNCH;

		packet.Index = pInfo->PeerDMIndex;
		packet.key = pInfo->key;
		DirectSend(hDevice, pInfo->eip, pInfo->eip.IsIPV6(), pInfo->eip.m_port, &packet, sizeof(packet));
	}

	if(m_TunnelSocketMode == TSM_KERNEL_MODE) // Let kernel thread send data immediately.
		Sleep(1);

	printx("Origin: %.3f. After Sleeping: %.3f. Work Time: %lld.\n", dEntryServerTime, dServerTime, nWorkTime);
	for(pos = m_FirstTimeList.GetHeadPosition(); pos;)
	{
		oldpos = pos;
		pInfo = &m_FirstTimeList.GetNext(pos);

		if(pInfo->nStartPunchTime > nWorkTime)
			break; // All hosts in the list wait more time.

		if(pInfo->flag & TPTF_F_PUNCH) // Don't break the loop even if this statement is false.
		{
			printx("First punch for VIP: %d.%d.%d.%d.\n", pInfo->vip.b1, pInfo->vip.b2, pInfo->vip.b3, pInfo->vip.b4);
			m_HostList.AddTail(*pInfo);
			m_FirstTimeList.RemoveAt(oldpos);
		}
	}
}

void CNetworkManager::ReportPTResult(stClientInfo *pClientInfo, BOOL bSuccess, stNatPunchInfo *pDesInfo)
{
	stTPTResult TPTResult;
	if(!(m_ReportTPT & SCF_TPT_REPORT))
		return;
	if(m_ReportTPT & SCF_TPT_REPORT_OAS && !(pDesInfo->flag & TPTF_ACTIVE))
		return;

//	printx("---> ReportPTResult. Size: %d\n", sizeof(stTPTResult));

	TPTResult.bResult = bSuccess;
	TPTResult.TPTFlag = pDesInfo->flag;
	TPTResult.SrcUID  = pClientInfo->ID1;
	TPTResult.DestUID = pDesInfo->uid;
//	TPTResult.SrcHostVip = pClientInfo->vip;
//	TPTResult.DesHostVip = pDesInfo->vip;

	TPTResult.SrcEIP = pClientInfo->ClientExternalIP;
	TPTResult.DesEIP = pDesInfo->eip;
	TPTResult.DesEIP.m_port = NBPort(pDesInfo->eip.m_port);
	TPTResult.time = time(0);

	m_ReportList.AddTail(TPTResult);
}

UINT CNetworkManager::GetReportData(CStreamBuffer &sb)
{
	POSITION pos;
	USHORT usMaxCount = (sb.GetRemainingSize() - sizeof(USHORT) * 3) / sizeof(stTPTResult);
	USHORT usTotal = m_ReportList.GetCount(), usActiveCount = 0, usActiveSuccess = 0;

	if(usTotal > usMaxCount)
		usTotal = usMaxCount;
	sb << usTotal;
	BYTE *pWriteAddress = sb.GetCurrentBuffer();
	sb.Skip( sizeof(usActiveCount) + sizeof(usActiveSuccess) );

	printx("---> GetReportData: %d\n", usTotal);

	for(pos = m_ReportList.GetHeadPosition(); usTotal; usTotal--)
	{
		stTPTResult &Data = m_ReportList.GetNext(pos);
		sb.Write(&Data, sizeof(stTPTResult));

		if(Data.TPTFlag & TPTF_ACTIVE)
		{
			if(Data.bResult)
				++usActiveSuccess;
			usActiveCount++;
		}

		m_ReportList.RemoveHead();
	}

	memcpy(pWriteAddress, &usActiveCount, sizeof(usActiveCount));
	memcpy(pWriteAddress + sizeof(usActiveCount), &usActiveSuccess, sizeof(usActiveSuccess));

	return usTotal;
}

void CNetworkManager::PrepareHandshakeData() // For debug only.
{
	ASSERT(!m_TempList.GetCount());
}

UINT CNetworkManager::GetHandshakeData(CStreamBuffer &sb, DWORD dwType, UINT nProspectiveCount)
{
	UINT nCount = m_TempList.GetCount(), nActiveCount = 0;
	ASSERT(nCount);

	if(nProspectiveCount)
		ASSERT(nCount == nProspectiveCount);

	UINT BufferSize = sb.GetRemainingSize();
	UINT SingleCmdSize = sizeof(DWORD) + sizeof(UINT) + sizeof(dwType) + sizeof(USHORT) + sizeof(DWORD) + sizeof(UINT64); // 26 bytes.
	UINT MaxCount = (BufferSize - sizeof(nCount) - sizeof(nActiveCount)) / SingleCmdSize; // Max 54 hosts for single packet.

	nCount = (MaxCount < nCount) ? MaxCount : nCount;

	ASSERT(sb.GetDataSize() == 8 && BufferSize >= (sizeof(nCount) + SingleCmdSize));
	printx("---> GetHandshakeData Host count: %d.\n", nCount);

	sb << nCount;
	POSITION pos = m_TempList.GetHeadPosition(), oldpos;
	for(UINT i = 0; i < nCount; ++i)
	{
		oldpos = pos;
		stPTHandshakeInfo &HSInfo = m_TempList.GetNext(pos);

		if(HSInfo.Flag & TPTF_ACTIVE)
			++nActiveCount;

		sb << HSInfo.UID << (UINT)(sizeof(dwType) + sizeof(HSInfo.DriverMapIndex) + sizeof(HSInfo.Key) + sizeof(HSInfo.nStartTime));
		sb << dwType << HSInfo.DriverMapIndex << HSInfo.Key << HSInfo.nStartTime;

		m_TempList.RemoveAt(oldpos);
	}
	sb << nActiveCount;

	ASSERT(sb.GetDataSize() <= sb.GetBufferSize());

	return nCount;
}

void CNetworkManager::KTAddHost(stKTInfo *pInfo, BOOL bServerSide)
{
//	ASSERT(AppGetClientInfo()->bInNat || AppGetClientInfo()->bInFirewall);
	UINT nPending, nCount;

	if(bServerSide)
	{
		if(!m_KEEP_TUNNEL_WITH_SERVER)
			return;
		pInfo->dwPeriod = m_KEEP_TUNNEL_WITH_SERVER;
		pInfo->dwTimeoutTick = m_KEEP_TUNNEL_WITH_SERVER;
	}
	else
	{
		if(!m_KEEP_TUNNEL_PERIOD)
			return;
		pInfo->dwPeriod = m_KEEP_TUNNEL_PERIOD;
		pInfo->dwTimeoutTick = m_KEEP_TUNNEL_PERIOD;
	}

	nPending = KTRemovePending(pInfo->vip);
	nCount = KTRemoveHost(pInfo->uid);

//	printx("%d %d\n", nCount, nPending);
	ASSERT(nCount == 0);
	ASSERT(nPending <= 1);

//	m_csKT.EnterCriticalSection();
	m_KeepTunnelList.AddTail(*pInfo);
//	m_csKT.LeaveCriticalSection();
}

UINT CNetworkManager::KTMarkPending(HANDLE hAdapter, DWORD uid, UINT nMinutes)
{
	POSITION pos, oldpos;
	CMapTable *pMapTable = &(AppGetClientInfo()->MapTable);
	stEntry *pEntry = pMapTable->Entry;
	UINT i, nTotalPendingTunnel = 0;

	if(uid)
	{
		for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
		{
			stKTInfo &info = m_KeepTunnelList.GetNext(pos);
			if(uid != info.uid || !(pEntry[info.DriverMapIndex].flag & AIF_OK))
				continue;

			ASSERT(info.uid == pEntry[info.DriverMapIndex].uid);
			ASSERT(!(info.flag & TPTF_PENDING));

			info.flag |= TPTF_PENDING;
			info.nTryCount = nMinutes * 60;
			pMapTable->UpdateTableFlag(hAdapter, info.DriverMapIndex, AIF_PENDING);
			nTotalPendingTunnel = 1;
			break;
		}
	}
	else // Mark all tunnels.
	{
		for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
		{
			oldpos = pos;
			stKTInfo &info = m_KeepTunnelList.GetNext(pos);
			if(info.flag & TPTF_PENDING)
				continue;

			if(info.uid)
			{
				info.flag |= TPTF_PENDING;
				info.nTryCount = nMinutes * 60;
				++nTotalPendingTunnel;
			}
			else
				m_KeepTunnelList.RemoveAt(oldpos); // No need to keep tunnel with server.
		}

		for(i = 0; i < pMapTable->m_Count; ++i)
			if(pEntry[i].flag & AIF_OK)
				pMapTable->UpdateTableFlag(hAdapter, i, AIF_PENDING);

		//printx("Tunnel pending count: %d - %d\n", i, nTotalPendingTunnel);
		ASSERT(i == nTotalPendingTunnel);

		stKTInfo info;
		ZeroMemory(&info, sizeof(info));
		info.flag |= TPTF_PENDING;
		info.nTryCount = nMinutes * 60;
		m_KeepTunnelList.AddTail(info); // Add a timer into list tail to close tunnel.
	}

	return nTotalPendingTunnel;
}

UINT CNetworkManager::KTRemovePending(DWORD vip)
{
	UINT nCount = 0;
	POSITION pos, oldpos;

	for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
	{
		oldpos = pos;
		stKTInfo &info = m_KeepTunnelList.GetNext(pos);

		if(info.flag & TPTF_PENDING && info.vip == vip)
		{
			m_KeepTunnelList.RemoveAt(oldpos);
			++nCount;
		}
	}

	return nCount;
}

UINT CNetworkManager::KTGetPendingCount()
{
	UINT nTotalPendingTunnel = 0;

	for(POSITION pos = m_KeepTunnelList.GetHeadPosition(); pos; )
		if(m_KeepTunnelList.GetNext(pos).flag & TPTF_PENDING)
			++nTotalPendingTunnel;

#ifdef DEBUG
	printx("CNetworkManager::KTGetPendingCount: %d\n", nTotalPendingTunnel);
#endif

	return nTotalPendingTunnel;
}

BOOL CNetworkManager::KTUnMarkPending(HANDLE hAdapter, DWORD vip, CMapTable *pTable) // uid could be null.
{
	BOOL bFound = FALSE;
	POSITION pos, oldpos;

	ASSERT((vip && pTable) || (!vip && !pTable));

	for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
	{
		oldpos = pos;
		stKTInfo &info = m_KeepTunnelList.GetNext(pos);
		if(!(info.flag & TPTF_PENDING) || info.vip != vip)
			continue;

		if(vip)
		{
			pTable->SetTableItem(hAdapter, info.DriverMapIndex, 0, TRUE);
		}
		else
		{
			printx("Remove timer of closing pending socket!\n", bFound);
		}

		m_KeepTunnelList.RemoveAt(oldpos);
		bFound = TRUE;
	}

	printx("CNetworkManager::KTUnMarkPending (%d)\n", bFound);

	return bFound;
}

void CNetworkManager::KTRemoveAllHost()
{
//	m_csKT.EnterCriticalSection();
	m_KeepTunnelList.RemoveAll();
//	m_csKT.LeaveCriticalSection();
}

UINT CNetworkManager::KTRemoveHost(DWORD uid)
{
	UINT nCount = 0;
	POSITION oldpos, pos;

	for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
	{
		oldpos = pos;
		stKTInfo &info = m_KeepTunnelList.GetNext(pos);
		if(info.uid == uid)
		{
			m_KeepTunnelList.RemoveAt(oldpos);
			++nCount;
		}
	}

	return nCount;
}

BOOL CNetworkManager::KTTimerEntry(HANDLE hAdapter, stClientInfo *pClientInfo, CMapTable *pTable)
{
	if(!m_KeepTunnelList.GetCount())
		return FALSE;

	printx("---> CNetworkManager::KTTimerEntry().\n");

	POSITION oldpos, pos;
	DWORD data = 0x12345678;
	BOOL bNeedKeep = pClientInfo->bInNat || pClientInfo->bInFirewall;

	for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
	{
		oldpos = pos;
		stKTInfo &info = m_KeepTunnelList.GetNext(pos);

		if(!info.dwTimeoutTick && bNeedKeep)
		{
			info.dwTimeoutTick = info.dwPeriod;
			DirectSend(hAdapter, info.eip, info.eip.m_bIsIPV6, info.eip.m_port, &data, sizeof(data));

			printx("Keep tunnel for vip:%d.%d.%d.%d UID: %08x.\n", info.vip.b1, info.vip.b2, info.vip.b3, info.vip.b4, info.uid);
		}

		if((info.flag & TPTF_PENDING) && !info.nTryCount)
		{
			if(info.uid)
			{
				printx("Pending tunnel time out! vip: %d.%d.%d.%d", info.vip.b1, info.vip.b2, info.vip.b3, info.vip.b4);

				ASSERT(pTable->m_Count >= info.DriverMapIndex);
				ASSERT(pTable->Entry[info.DriverMapIndex].flag & AIF_PENDING);

				pTable->SetTableItem(hAdapter, info.DriverMapIndex, 0, TRUE);
				pTable->RecycleMapIndex(info.DriverMapIndex, __LINE__);
			}
			else
			{
				CloseTunnelSocket(hAdapter, FALSE);
				Release();
				printx("Tunnel pending time out! Tunnel closed!\n");
				return TRUE;
			}

			m_KeepTunnelList.RemoveAt(oldpos);
		}
	}

	return FALSE;
}

BOOL CNetworkManager::TickOnce()
{
	BOOL bResult = m_HostList.GetCount() + m_FirstTimeList.GetCount(); // Return true if need to handle job.
	POSITION oldpos, pos;

//	printx("CNetworkManager::TickOnce()\n");

	if(!m_KeepTunnelList.GetCount())
		return bResult;

	for(pos = m_KeepTunnelList.GetHeadPosition(); pos; )
	{
		oldpos = pos;
		stNatPunchInfo &info = m_KeepTunnelList.GetNext(pos);

		if(!--info.dwTimeoutTick)
			bResult = TRUE;

		if(info.flag & TPTF_PENDING)
			if(!--info.nTryCount)
				bResult = TRUE;
	}

	return bResult;
}

UINT CNetworkManager::ArpDeleteAddress(USHORT DriverIndex, DWORD ipv4, BOOL bDynamicOnly, BOOL bDeleteAll)
{
//	printx("---> ArpDeleteAddress.\n");

	UINT nCount = 0;
	BOOL bMalloc = FALSE;
	MIB_IPNETROW ArpEntry = {0};
	BYTE DesMac[6] = { FIMAC_VB1, FIMAC_VB2, FIMAC_VB3, FIMAC_VB4, (BYTE)(DriverIndex & 0xFF), (BYTE)(DriverIndex >> 8) };
	BYTE TableBuffer[sizeof(MIB_IPNETTABLE) + sizeof(MIB_IPNETROW) * DEFAULT_ARP_TABLE_SIZE];
	MIB_IPNETTABLE *pNetTable = (MIB_IPNETTABLE*)TableBuffer;
	ULONG nSize = sizeof(TableBuffer), IfIndex;

	DWORD dwRet = GetIpNetTable(pNetTable, &nSize, FALSE);

	if(dwRet == ERROR_INSUFFICIENT_BUFFER)
	{
		bMalloc = TRUE;
		pNetTable = (MIB_IPNETTABLE*)malloc(nSize);
		dwRet = GetIpNetTable(pNetTable, &nSize, FALSE);
	}
	if(dwRet != NO_ERROR)
		goto End;

//	if(GetVirtualAdapterInfo(0, 0, 0, &IfIndex))
	IfIndex = m_AdapterIndex;
	{
		ArpEntry.dwIndex = IfIndex;
		for(DWORD i = 0; i < pNetTable->dwNumEntries; ++i) // Loop to find target ip address.
		{
			if(pNetTable->table[i].dwIndex != IfIndex)
				continue;
			if(bDynamicOnly && pNetTable->table[i].Type != MIB_IPNET_TYPE_DYNAMIC)
				continue;
			if(!bDeleteAll && memcmp(pNetTable->table[i].bPhysAddr, DesMac, sizeof(DesMac)) && pNetTable->table[i].dwAddr != ipv4)
				continue;

			ArpEntry.dwAddr = pNetTable->table[i].dwAddr;
			dwRet = DeleteIpNetEntry(&ArpEntry);

			if(dwRet == NO_ERROR)
				nCount++;
			else
			{
				printx("DeleteIpNetEntry failed! ec: %d\n", dwRet);
				ASSERT(0);
			}
		}
	}

End:
	if(bMalloc)
		free(pNetTable);

	return nCount;
}

void CNetworkManager::AddPortMapping(BOOL bUDP, CIpAddress iip, UINT iport, UINT eport)
{
	HRESULT hr;
	BOOL bContinue = TRUE;
	IUPnPNAT* piNAT = NULL;
	IStaticPortMappingCollection* piPortMappingCollection = NULL;
	IStaticPortMapping* piStaticPortMapping = NULL;

	CoInitialize(NULL);

	if ( !bContinue || !SUCCEEDED( CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_ALL, __uuidof(IUPnPNAT), (void **)&piNAT) ) || ( piNAT == NULL ) )
		bContinue = FALSE;

	if ( !bContinue || !SUCCEEDED( piNAT->get_StaticPortMappingCollection(&piPortMappingCollection) ) || (piPortMappingCollection==NULL ) )
		bContinue = FALSE;

	if (bContinue)
	{
		CString csIIP = iip.GetString();
		printx(_T("Add port mapping. iip: %s:%d eport: %d ..."), csIIP, iport, eport);

		VARIANT_BOOL vb = (1) ? VARIANT_TRUE : VARIANT_FALSE;
		hr = piPortMappingCollection->Add(eport, bUDP ? _bstr_t("UDP") : _bstr_t("TCP"), iport,	_bstr_t(csIIP),
			vb, _bstr_t("nMatrix VPN Tunnel"), &piStaticPortMapping);

		if(FAILED(hr) || (piStaticPortMapping == NULL))
			printx("Failed! (hr: %08x)\n", hr);
		else
		{
			ASSERT(!m_UPNPPort);
			m_UPNPPort = eport;
			printx("OK\n");
		}
	}

	SAFE_RELEASE(piStaticPortMapping);
	SAFE_RELEASE(piPortMappingCollection);
	SAFE_RELEASE(piNAT);

	CoUninitialize();
}

void CNetworkManager::RemovePortMapping(BOOL bUDP, UINT eport)
{
	HRESULT hr;
	BOOL bContinue = TRUE;
	IUPnPNAT* piNAT = NULL;
	IStaticPortMappingCollection* piPortMappingCollection = NULL;

	CoInitialize(NULL);

	if ( !bContinue || !SUCCEEDED( CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_ALL, __uuidof(IUPnPNAT), (void **)&piNAT) ) || ( piNAT == NULL ) )
		bContinue = FALSE;

	if ( !bContinue || !SUCCEEDED( piNAT->get_StaticPortMappingCollection(&piPortMappingCollection) ) || (piPortMappingCollection==NULL ) )
		bContinue = FALSE;

	if (bContinue)
	{
		printx(_T("Remove port mapping. eport: %d ... "), eport);

		hr = piPortMappingCollection->Remove(eport, bUDP ? _bstr_t("UDP") : _bstr_t("TCP"));

		if(FAILED(hr))
			printx("Failed! (hr: %08x)\n", hr);
		else
			printx("OK\n");
	}

	SAFE_RELEASE(piPortMappingCollection);
	SAFE_RELEASE(piNAT);

	CoUninitialize();
	m_UPNPPort = 0;
}

BOOL CNetworkManager::OpenFirewallPort(UINT port)
{
	if(WFWSetPort(TRUE, port, m_FirewallPort))
		m_FirewallPort = port;
	else
		m_FirewallPort = 0;
	return m_FirewallPort;
}

void CNetworkManager::CloseFirewallPort()
{
	if(m_FirewallPort)
	{
		WFWSetPort(TRUE, 0, m_FirewallPort);
		m_FirewallPort = 0;
	}
}

BOOL CNetworkManager::WFWSetPort(BOOL bUDP, UINT PortOpen, UINT PortClose)
{
	BOOL bResult = TRUE;
	CoInitialize(NULL); // S_FALSE: The COM library is already initialized on this thread. So doesn't check result.

	HRESULT hr;
	INetFwMgr *pINetManager = 0;
	hr = CoCreateInstance(__uuidof(NetFwMgr), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwMgr), (void**)&pINetManager);
	// Return 0x80070005 if doesn't have adm right.
	if(SUCCEEDED(hr))
	{
		INetFwPolicy *pPolicy = 0;
		hr = pINetManager->get_LocalPolicy(&pPolicy);
		if(SUCCEEDED(hr))
		{
			INetFwProfile *pIProfile = 0;
			hr = pPolicy->get_CurrentProfile(&pIProfile);
			if(SUCCEEDED(hr))
			{
				INetFwOpenPorts *pIFirewallOpenPort = 0;
				hr = pIProfile->get_GloballyOpenPorts(&pIFirewallOpenPort);
				if(SUCCEEDED(hr))
				{
					if(PortOpen)
					{
						INetFwOpenPort *pIOpenPort;
						hr = CoCreateInstance(__uuidof(NetFwOpenPort), 0, CLSCTX_INPROC_SERVER, __uuidof(INetFwOpenPort), (void**)&pIOpenPort);
						if(SUCCEEDED(hr))
						{
							hr = pIOpenPort->put_Name(_bstr_t(_T("nMatrix VPN Tunnel")));
							hr = pIOpenPort->put_Protocol(bUDP ? NET_FW_IP_PROTOCOL_UDP : NET_FW_IP_PROTOCOL_TCP);
							hr = pIOpenPort->put_Enabled(VARIANT_TRUE);
							hr = pIOpenPort->put_Port(PortOpen);

							hr = pIFirewallOpenPort->Add(pIOpenPort);

							pIOpenPort->Release();
						}
					}
					if(PortClose)
					{
						pIFirewallOpenPort->Remove(PortClose, bUDP ? NET_FW_IP_PROTOCOL_UDP : NET_FW_IP_PROTOCOL_TCP);
					}
					pIFirewallOpenPort->Release();
				}
				pIProfile->Release();
			}
			pPolicy->Release();
		}
		pINetManager->Release();
	}

	if(FAILED(hr))
	{
		bResult = FALSE;
		printx("WFWSetPort failed! (hr: %08x)\n", hr);
	}

	CoUninitialize();
	return bResult;
}

BOOL CNetworkManager::OpenTunnelSocket(HANDLE hAdapter, DWORD ip, USHORT port)
{
	ASSERT(/*!m_TunnelOpenState && */(m_TunnelSocketMode == TSM_KERNEL_MODE || m_TunnelSocketMode == TSM_USER_MODE));
//	printx("Open tunnel address: %d.%d.%d.%d:%d\n", ((IPV4)ip).b1, ((IPV4)ip).b2, ((IPV4)ip).b3, ((IPV4)ip).b4, port);

	if(m_TunnelSocketMode == TSM_KERNEL_MODE)
	{
		ChangeByteOrder(ip);
		ASSERT(!m_IPCPort);

		if(m_TunnelSocket.CreateEx(CSocketBase::SF_IPV4 |CSocketBase::SF_UDP_SOCKET)) // Open IPC socket to receive data from kernel.
		{
			CIpAddress IpAddress;
			m_TunnelSocket.GetSockNameEx(IpAddress);
			m_IPCPort = IpAddress.m_port;
			//printx("IPC port: %d\n", m_IPCPort);

			HANDLE hEvent = CreateEvent(0, FALSE, FALSE, 0);
			if(hEvent != NULL)
			{
				if(!WSAEventSelect(m_TunnelSocket.GetSocket(), hEvent, FD_READ))
				{
					if(OpenUDP(hAdapter, ip, port, NBPort(m_IPCPort)))
					{
						ASSERT(*m_pdwEventCount == CEI_IPC_SOCKET_EVENT_INDEX && !m_pEventArray[CEI_IPC_SOCKET_EVENT_INDEX]);
						m_pEventArray[CEI_IPC_SOCKET_EVENT_INDEX] = hEvent;
						(*m_pdwEventCount)++;

						m_TunnelOpenState = TRUE;
						return TRUE;
					}
				}
				CloseHandle(hEvent);
			}
			m_IPCPort = 0;
			m_TunnelSocket.CloseSocket();
		}
	}
	else
	{
		if(m_TunnelSocket.CreateEx(CSocketBase::SF_IPV4 |CSocketBase::SF_UDP_SOCKET, ip, port))
		{
			BOOL bNewBehavior = FALSE;
			DWORD dwBytesReturned = 0;
			// Windows XP:  Controls whether UDP PORT_UNREACHABLE messages are reported. Set to TRUE to enable reporting. Set to FALSE to disable reporting.
			if(!WSAIoctl(m_TunnelSocket.GetSocket(), SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL))
			{
				printx("Create UM socket successfully!\n");
				m_TunnelOpenState = TRUE;
				return TRUE;
			}

			printx("Set SIO_UDP_CONNRESET failed! ec: %d.", WSAGetLastError());
			m_TunnelSocket.CloseSocket();
		}
	}

	return FALSE;
}

void CNetworkManager::CloseIPCSocket()
{
	if(m_TunnelSocket.GetSocket() != INVALID_SOCKET) // Close IPC socket.
	{
		ASSERT(*m_pdwEventCount == CEI_IPC_SOCKET_EVENT_INDEX + 1 && m_pEventArray[CEI_IPC_SOCKET_EVENT_INDEX] && m_IPCPort);
		m_IPCPort = 0;
		(*m_pdwEventCount)--;
		m_TunnelSocket.CloseSocket();
		CloseHandle(m_pEventArray[CEI_IPC_SOCKET_EVENT_INDEX]);
		m_pEventArray[CEI_IPC_SOCKET_EVENT_INDEX] = 0;
	}
}

void CNetworkManager::CloseTunnelSocket(HANDLE hAdapter, BOOL bForceCloseKMSocket)
{
	m_TunnelOpenState = FALSE;

	if(m_TunnelSocketMode == TSM_USER_MODE && !bForceCloseKMSocket)
	{
		m_TunnelSocket.CloseSocket();
		CloseUserModeSocketThread();
	}
	else
	{
		CloseIPCSocket();
		CloseUDP(hAdapter);
	}
}

BOOL CNetworkManager::SetTunnelSocketMode(HANDLE hAdapter, BYTE mode)
{
	BOOL bSuccess = TRUE;
	stClientInfo *pClientInfo = AppGetClientInfo();

	if(mode == TSM_KERNEL_MODE)
	{
		if(m_TunnelSocketMode != TSM_KERNEL_MODE)
		{
			printx("Switch tunnel socket from user mode to kernel mode.\n");
			m_TunnelSocketMode = mode;
			if(m_TunnelOpenState)
			{
				m_TunnelSocket.CloseSocket();
				CloseUserModeSocketThread();

				pClientInfo->MapTable.UpdateTrafficData(hAdapter);
				bSuccess = OpenTunnelSocket(hAdapter, pClientInfo->ClientInternalIP.v4, pClientInfo->UDPPort);
			}
		}
	}
	else
	{
		ASSERT(mode == TSM_USER_MODE);
		if(m_TunnelSocketMode != TSM_USER_MODE)
		{
			printx("Switch tunnel socket from kernel mode to user mode.\n");
			m_TunnelSocketMode = mode;
			if(m_TunnelOpenState)
			{
				CloseIPCSocket();
				CloseUDP(hAdapter);

				if(bSuccess = OpenTunnelSocket(hAdapter, pClientInfo->ClientInternalIP.v4, pClientInfo->UDPPort))
					InitUserModeSocketThread(hAdapter);
			}
		}
	}

	return bSuccess;
}

BOOL CNetworkManager::CheckTunnelSocketAddress(DWORD ip, USHORT &port)
{
	INT ec;
	BOOL bResult = FALSE;
	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(s != INVALID_SOCKET)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.S_un.S_addr = ip;

		if(!port)
		{
			port = DEFAULT_TUNNEL_PORT - 1;
			while(port < DEFAULT_TUNNEL_PORT + 500)
			{
				port++;
				addr.sin_port = htons(port);
				if(bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
				{
					if((ec = WSAGetLastError()) == WSAEADDRINUSE)
						continue;
					printx("Failed to bind address! ec: %d\n", ec);
				}
				else
					bResult = TRUE;
				break;
			}
		}
		else
		{
			addr.sin_port = htons(port);
			if(bind(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
			{
				printx("Failed to bind address! ec: %d\n", WSAGetLastError());
			}
			else
				bResult = TRUE;
		}
		closesocket(s);
	}

	return bResult;
}

void CNetworkManager::DirectSend(HANDLE hAdapter, IpAddress ip, BOOL bIsIPV6, USHORT port, void *pData, UINT size)
{
//	printx("---> CNetworkManager::DirectSend\n");

	if(m_TunnelSocketMode == TSM_USER_MODE)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = port;
		addr.sin_addr.S_un.S_addr = ip.v4;

		INT nResult = m_TunnelSocket.SendTo(pData, size, (SOCKADDR*)&addr, sizeof(addr), 0);
		//printx("DirectSend with UM socket! %d\n", nResult); // Show info after sending data.
		return;
	}

	::DirectSend(hAdapter, ip, bIsIPV6, port, pData, size);
}

void CNetworkManager::AdjustAdapterParameter(IPV4 vipIn, BYTE *vmacIn, DWORD TaskOffloadCaps, DWORD Caps) // Server assigned address.
{
	// Check virtual mac & ip.
	BOOL bDHCP = FALSE, bAdapterTaskOffload;
	BYTE vmac[6] = {0};
	IPV4 vip = 0, vipSys = 0;

	CStringA csVipA;
	CString csVip;
	GetVirtualAdapterInfo(&bDHCP, &csVip);
	if(csVip != _T(""))
	{
		csVipA = csVip;
		vipSys.ip = inet_addr(csVipA);
	}

	HANDLE hAdapter = OpenAdapter();
	GetAdapterParam(hAdapter, vmac, &vip.ip, &bAdapterTaskOffload); // Get vip from virtual adapter.
	printx("---> AdjustAdapterParameter.\n");
	printx("DHCP: %d. %s -> %d.%d.%d.%d -> %d.%d.%d.%d\n", bDHCP, csVipA, vip.b1, vip.b2, vip.b3, vip.b4, vipIn.b1, vipIn.b2, vipIn.b3, vipIn.b4);
	printx("TaskOffload: %d (Adapter) %d (Server)\n", bAdapterTaskOffload, TaskOffloadCaps);
//	printx("%d.%d.%d.%d\n", vipSys.b1, vipSys.b2, vipSys.b3, vipSys.b4);

	if(memcmp(vmac, vmacIn, sizeof(vmac)) || bAdapterTaskOffload != TaskOffloadCaps)
	{
		NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)1);
		SetAdapterRegInfo(vmacIn, vipIn, TaskOffloadCaps);
	//	SetAdapterParam(hAdapter, vmacIn, vipIn);

		CloseUDP(hAdapter);
		CloseAdapter(hAdapter);

		// New windows now will use last acquired ip address so must release the old ip address.
		// MSDN: DHCP (Dynamic Host Configuration Protocol) Basics - http://support.microsoft.com/kb/169289/en-us.
		if(bDHCP)
			RenewAdapter(2); // Release address before calling RessetAdapter().
		ResetAdapter();

		hAdapter = OpenAdapter(); // Must keep adapter handle for later operation. Ex: OpenUDP(), InitUserModeSocketThread().
		if(m_TunnelSocketMode == TSM_KERNEL_MODE)
		{
			stClientInfo *pClientInfo = AppGetClientInfo();
			DWORD ipv4 = pClientInfo->ClientInternalIP.v4;
			ChangeByteOrder(ipv4);
			if(!OpenUDP(hAdapter, ipv4, pClientInfo->UDPPort, NBPort(m_IPCPort)))
				AfxMessageBox(_T("AdjustAdapterParameter::OpenUDP failed!"));
		}

		NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)0);
	}
	else if(vipIn != vip) // Renew adapter.
	{
		SetAdapterRegInfo(vmacIn, vipIn, TaskOffloadCaps);
		SetAdapterParam(hAdapter, vmacIn, vipIn);
		if(bDHCP)
		{
			NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)1);
			RenewAdapter(1);
			NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)0);
		}
	}
	else if(vipIn != vipSys)
	{
		if(bDHCP)
		{
			NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)1);
			RenewAdapter(1);
			NotifyGUIMessage(GET_UPDATE_ADAPTER_CONFIG, (DWORD)0);
		}
	}

	if(m_TunnelSocketMode == TSM_USER_MODE)
		InitUserModeSocketThread(hAdapter);

	SetAdapterFunc(hAdapter, Caps);
	CloseAdapter(hAdapter);

	// Common initialization.
	m_nLastAssignedTime = 0;
	m_AssignedCount = 0;

	GetVirtualAdapterInfo(0, 0, 0, &m_AdapterIndex); // Init adapter index first.
	UINT nCount = ArpDeleteAddress(0, 0, TRUE, TRUE);

	printx("<--- AdjustAdapterParameter. %d\n", nCount);
}

void CNetworkManager::InitUserModeSocketThread(HANDLE hAdapter)
{
	if(m_TunnelSocket.GetSocket() == INVALID_SOCKET)
	{
		ASSERT(0);
		printx("User mode tunnel socket is not opened!\n");
		return;
	}

	HANDLE hRecvEvent, hSendEvent;
	stClientInfo *pClientInfo = AppGetClientInfo();

	// Make sure events have been created by driver.
	DWORD dwResult = EnableUserMode(hAdapter, 1, TRUE);
	if(!dwResult)
	{
		printx("Failed to create um events!\n");
		return;
	}
	printx("EnableUserMode: %d.\n", dwResult);

	hRecvEvent = CreateEvent(0, FALSE, TRUE, _T(DRIVER_UM_WRITE_EVENT));
	if(!hRecvEvent || (hRecvEvent && GetLastError() != ERROR_ALREADY_EXISTS))
		printx("Open kernel recv event failed! ec: %d\n", GetLastError());

	hSendEvent = CreateEvent(0, FALSE, TRUE, _T(DRIVER_UM_READ_EVENT));
	if(!hSendEvent || (hSendEvent && GetLastError() != ERROR_ALREADY_EXISTS))
		printx("Open kernel send event failed! ec: %d\n", GetLastError());

/*
	hRecvEvent = OpenEvent(EVENT_ALL_ACCESS, 0, _T("Global\\KernelWriteEvent"));
	if(!hRecvEvent)
		printx("Open kernel recv event failed! ec: %d\n", GetLastError());

	hSendEvent = OpenEvent(EVENT_ALL_ACCESS, 0, _T("Global\\KernelEventTest"));
	if(!hSendEvent)
		printx("Open kernel send event failed! ec: %d\n", GetLastError());
*/
	if(!hRecvEvent || !hSendEvent)
	{
		SAFE_CLOSE_HANDLE(hRecvEvent);
		SAFE_CLOSE_HANDLE(hSendEvent);
		return;
	}

	pClientInfo->hUMRecvEvent = hRecvEvent;
	pClientInfo->hUMSendEvent = hSendEvent;

	ASSERT(!pClientInfo->hRecvThread && !pClientInfo->hSendThread);

	pClientInfo->bEnableUM = TRUE;
	pClientInfo->hRecvThread = CreateThread(0, 0, CnMatrixCore::KernelWriteThreadEx, 0, 0, 0);
	pClientInfo->hSendThread = CreateThread(0, 0, CnMatrixCore::KernelReadThreadEx, 0, 0, 0);
//	pClientInfo->hRecvThread = CreateThread(0, 0, CnMatrixCore::KernelWriteThread, 0, 0, 0);
//	pClientInfo->hSendThread = CreateThread(0, 0, CnMatrixCore::KernelReadThread, 0, 0, 0);

//	SetThreadPriority(pClientInfo->hSendThread, THREAD_PRIORITY_ABOVE_NORMAL);
//	SetThreadPriority(pClientInfo->hRecvThread, THREAD_PRIORITY_ABOVE_NORMAL);
}

void CNetworkManager::CloseUserModeSocketThread()
{
	stClientInfo *pClientInfo = AppGetClientInfo();
	HANDLE hThread[2] = {pClientInfo->hRecvThread, pClientInfo->hSendThread};

	if(!hThread[0] && !hThread[1])
		return;

	pClientInfo->bEnableUM = FALSE;
	SetEvent(pClientInfo->hUMRecvEvent);
	SetEvent(pClientInfo->hUMSendEvent);

	DWORD dwResult = WaitForMultipleObjects(2, hThread, TRUE, 5000);

	if(dwResult == WAIT_TIMEOUT || dwResult == WAIT_FAILED)
		printx("Can't termainate user mode socket thread! ec: %d.\n", GetLastError());

	SAFE_CLOSE_HANDLE(pClientInfo->hUMRecvEvent);
	SAFE_CLOSE_HANDLE(pClientInfo->hUMSendEvent);
	SAFE_CLOSE_HANDLE(pClientInfo->hRecvThread);
	SAFE_CLOSE_HANDLE(pClientInfo->hSendThread);
}

BOOL CNetworkManager::IsValidIpAddress(IPV4 ip)
{
	PIP_ADAPTER_INFO pAdapterInfo = 0;
	PIP_ADAPTER_INFO pAdapter;
	ULONG ulOutBufLen = 0;
	BOOL bResult = FALSE;
	CStringA inIP;
	inIP.Format("%d.%d.%d.%d", ip.b1, ip.b2, ip.b3, ip.b4);

	if(GetAdaptersInfo(0, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		pAdapterInfo = (IP_ADAPTER_INFO*) MALLOC(ulOutBufLen);
		if(!pAdapterInfo)
			return 0;
	}

	if(GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while(pAdapter)
		{
			if(pAdapter->IpAddressList.IpAddress.String == inIP)
			{
				bResult = TRUE;
			//	printx("Address is found!\n");
				break;
			}
			pAdapter = pAdapter->Next;
		}
	}

	if(pAdapterInfo)
		FREE(pAdapterInfo);

	return bResult;
}


