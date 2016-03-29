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
#include "resource.h"
#include "VividTreeEx.h"
#include "SetupDialog.h"


extern CFont GBoldFont;


CString UTXFormatTreeHostName(CString &HostName, DWORD vip)
{
	IPV4 IP;
	CString str;
	IP.ip = vip;

	str.Format(_T("  %s (%d.%d.%d.%d)"), HostName, IP.b1, IP.b2, IP.b3, IP.b4);

	return str;
}


void CGUINetworkManager::Release()
{
	for(POSITION npos = m_VNetList.GetHeadPosition(); npos; )
	{
		stGUIVLanInfo *pVLan = m_VNetList.GetNext(npos);
		for(POSITION mpos = pVLan->MemberList.GetHeadPosition(); mpos; )
			delete pVLan->MemberList.GetNext(mpos);
		pVLan->MemberList.RemoveAll();
		delete pVLan;
	}

	m_VNetList.RemoveAll();
}

stGUIVLanInfo* CGUINetworkManager::CreateVNet()
{
	stGUIVLanInfo* pVLanInfo = new stGUIVLanInfo;

	if(pVLanInfo)
		m_VNetList.AddTail(pVLanInfo);

	return pVLanInfo;
}

stGUIVLanInfo* CGUINetworkManager::FindVNet(DWORD_PTR VNetID, const TCHAR *pName)
{
	stGUIVLanInfo *pVLanInfo;
	POSITION pos = m_VNetList.GetHeadPosition();

	if(VNetID)
		for(; pos; )
		{
			pVLanInfo = m_VNetList.GetNext(pos);
			if(pVLanInfo->NetIDCode == VNetID)
				return pVLanInfo;
		}
	else if(pName)
		for(; pos; )
		{
			pVLanInfo = m_VNetList.GetNext(pos);
			if(pVLanInfo->NetName == pName)
				return pVLanInfo;
		}

	return 0;
}

BOOL CGUINetworkManager::FindRelayHost(DWORD_PTR VNetID, DWORD_PTR &UID, IPV4 &vip, CString &HostName)
{
	BOOL bFound = FALSE;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	pVLanInfo = FindVNet(VNetID);
	if(!pVLanInfo)
		return FALSE;

	if(pVLanInfo->Flag & VF_RELAY_HOST) // Test local host.
	{
		UID = 0;
		return TRUE;
	}

	for(POSITION pos = pVLanInfo->MemberList.GetHeadPosition(); pos; )
	{
		pMember = pVLanInfo->MemberList.GetNext(pos);
		if(pMember->Flag & VF_RELAY_HOST)
		{
			UID = pMember->dwUserID;
			vip = pMember->vip;
			HostName = pMember->HostName;
			bFound = TRUE;
			break;
		}
	}

	return bFound;
}

BOOL CGUINetworkManager::OfflineSubnet(BOOL bOnline, DWORD_PTR VNetID)
{
	BOOL bResult = FALSE;
	stGUIVLanInfo *pVLanInfo;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(pVLanInfo->NetIDCode != VNetID)
			continue;

		if(bOnline)
			pVLanInfo->Flag |= VF_IS_ONLINE;
		else
			pVLanInfo->Flag &= ~VF_IS_ONLINE;

		bResult = TRUE;
		break;
	}

	if(bResult)
		m_pTreeCtrl->RedrawWindow();

	return bResult;
}

BOOL CGUINetworkManager::OfflineNetMember(DWORD_PTR VNetID, DWORD_PTR UID, BOOL bOnline)
{
	BOOL bResult = FALSE;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(pVLanInfo->NetIDCode != VNetID)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != UID)
				continue;

			if(bOnline)
				pMember->Flag |= VF_IS_ONLINE;
			else
				pMember->Flag &= ~VF_IS_ONLINE;
			bResult = TRUE;
			break;
		}

		break;
	}

	ASSERT(bResult);
	return bResult;
}

void CGUINetworkManager::UpdateAllMemberFromVNet(stGUIVLanInfo *pVLanInfo)
{
	for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
	{
		stGUIVLanMember *pMember = pVLanInfo->MemberList.GetNext(mpos);
		if(pMember->LinkState != LS_TRYING_TPT)
			continue;
		SetMemberLinkState(pVLanInfo->NetIDCode, pMember->dwUserID, LS_TRYING_TPT, 0, 0, 0);
	}
}

BOOL CGUINetworkManager::UpdateVLanFlag(DWORD_PTR VNetID, DWORD dwFlag)
{
	BOOL bFound = FALSE;
	stGUIVLanInfo *pVLanInfo;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(pVLanInfo->NetIDCode != VNetID)
			continue;

		pVLanInfo->UpdateFlag((USHORT)dwFlag);
		bFound = TRUE;
		break;
	}

	return bFound;
}

BOOL CGUINetworkManager::UpdateMemberRole(DWORD_PTR VNetID, DWORD_PTR UserID, DWORD dwFlag, DWORD dwMask)
{
	BOOL bFound = FALSE;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;
	USHORT flag = (USHORT)dwFlag, mask =  (USHORT)dwMask; //VF_HUB | VF_RELAY_HOST;

	if(mask == VF_RELAY_HOST)
	{
		for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
		{
			pVLanInfo = m_VNetList.GetNext(npos);
			if(pVLanInfo->NetIDCode != VNetID)
				continue;

			if(!UserID)
			{
				pVLanInfo->Flag &= ~mask;
				pVLanInfo->Flag |= (flag & mask);
				pVLanInfo->UpdataToolTipString();
				bFound = TRUE;
				break;
			}

			for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
			{
				pMember = pVLanInfo->MemberList.GetNext(mpos);
				if(pMember->dwUserID != UserID)
					continue;
				pMember->Flag &= ~mask;
				pMember->Flag |= (flag & mask);
				bFound = TRUE;
				break;
			}
			break;
		}
	}
	else if(mask == VF_ALLOW_RELAY && !(flag & VF_ALLOW_RELAY))
	{
		for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
		{
			pVLanInfo = m_VNetList.GetNext(npos);
			if(pVLanInfo->NetIDCode != VNetID)
				continue;

			if(!UserID)
			{
				pVLanInfo->Flag &= ~(VF_ALLOW_RELAY | VF_RELAY_HOST);
				pVLanInfo->UpdataToolTipString();
				bFound = TRUE;
				break;
			}

			for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; ) // Must check all members of the network.
			{
				pMember = pVLanInfo->MemberList.GetNext(mpos);
				if(pMember->dwUserID != UserID)
					continue;
				pMember->Flag &= ~(VF_ALLOW_RELAY | VF_RELAY_HOST);
				bFound = TRUE;
				break;
			}
			break;
		}
	}
	else
	{
		for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
		{
			pVLanInfo = m_VNetList.GetNext(npos);
			if(pVLanInfo->NetIDCode != VNetID)
				continue;

			if(!UserID)
			{
				pVLanInfo->Flag &= ~mask;
				pVLanInfo->Flag |= (flag & mask);
				pVLanInfo->UpdataToolTipString();
				bFound = TRUE;
				break;
			}

			for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
			{
				pMember = pVLanInfo->MemberList.GetNext(mpos);
				if(pMember->dwUserID != UserID)
					continue;

				pMember->Flag &= ~mask;
				pMember->Flag |= (flag & mask);
				bFound = TRUE;
				break; // Only one host can exist in the VLAN.
			}
			break;
		}
	}

	return bFound;
}

UINT CGUINetworkManager::UpdateHostData(DWORD_PTR VNetID, DWORD_PTR UserID, DWORD type, void* pData, UINT nDataSize)
{
	UINT nFound = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	ASSERT(UserID);

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(VNetID && pVLanInfo->NetIDCode != VNetID)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != UserID)
				continue;

			switch(type)
			{
				case UDF_ONLINE_TIME:
					ASSERT(nDataSize == sizeof(time_t));
					break;
			}
			nFound++;
		}

		if(VNetID)
			break;
	}

	return nFound;
}

UINT CGUINetworkManager::UpdateMemberProfile(void *pGUI, DWORD UserID, UINT nType, void* pData, UINT nDataSize)
{
	UINT nFound = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;
	POSITION npos, mpos;
	CVividTreeEx *pTreeObj = (CVividTreeEx*)pGUI;

	for(npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);

		for(mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos;)
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != UserID)
				continue;

			switch(nType)
			{
				case CPT_HOST_NAME:
					pMember->HostName = (TCHAR*)pData;
					pTreeObj->SetItemText((HTREEITEM)pMember->GUIHandle, UTXFormatTreeHostName(pMember->HostName, pMember->vip));

					break;
			}

			++nFound;
			break;
		}
	}

	return nFound;
}

stGUIVLanInfo* CGUINetworkManager::RemoveUser(DWORD_PTR VNetID, DWORD_PTR UID)
{
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(pVLanInfo->NetIDCode != VNetID)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(), oldpos = mpos; mpos; oldpos = mpos)
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != UID)
				continue;

			if(pMember->LinkState != LS_OFFLINE)
				--pVLanInfo->nOnline;
			m_pTreeCtrl->DeleteItem((HTREEITEM)pMember->GUIHandle); // Update GUI first.
			delete pMember;
			pVLanInfo->MemberList.RemoveAt(oldpos);
			pVLanInfo->UpdataToolTipString(); // Update string after calling CList::RemoveAt.

			return pVLanInfo;
		}

		return 0;
	}

	return 0;
}

BOOL CGUINetworkManager::RemoveVLan(DWORD_PTR VNetID)
{
	BOOL bResult = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos; m_VNetList.GetNext(npos))
	{
		pVLanInfo = m_VNetList.GetAt(npos);
		if(pVLanInfo->NetIDCode != VNetID)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(), oldpos = mpos; mpos; oldpos = mpos)
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);

			m_pTreeCtrl->DeleteItem((HTREEITEM)pMember->GUIHandle); // Must delete item to prevent crashing when GUI update in the same time.
			delete pMember;
		}

		m_pTreeCtrl->DeleteItem((HTREEITEM)pVLanInfo->GUIHandle);
		m_VNetList.RemoveAt(npos);
		delete pVLanInfo;
		bResult = TRUE;
		break;
	}

	return bResult;
}

INT CGUINetworkManager::SetMemberLinkState(DWORD_PTR dwSkipNetID, DWORD_PTR uid, USHORT LinkState, USHORT *pDriverMapIndex, CIpAddress *pNewAddress, USHORT *pNewEPort, DWORD_PTR *dwRHNetID)
{
	INT nCount = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(dwSkipNetID && pVLanInfo->NetIDCode == dwSkipNetID)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != uid)
				continue;

			if(pMember->LinkState != LinkState)
				if(pMember->LinkState == LS_OFFLINE)
				{
					pVLanInfo->nOnline++;
					pVLanInfo->UpdataToolTipString();
				}
				else if(LinkState == LS_OFFLINE)
				{
					pVLanInfo->nOnline--;
					pVLanInfo->UpdataToolTipString();
				}

			pMember->LinkState = LinkState;
			if(LinkState == LS_RELAYED_TUNNEL)
				pMember->RelayVNetID = *dwRHNetID;
			if(pDriverMapIndex)
				pMember->DriverMapIndex = *pDriverMapIndex;
			if(pNewAddress)
				pMember->eip = *pNewAddress;
			if(pNewEPort)
				pMember->eip.m_port = *pNewEPort;
			++nCount;
			break;
		}
	}

	if(nCount)
		m_pTreeCtrl->RedrawWindow();

	return nCount;
}

INT CGUINetworkManager::UpdateHost(DWORD_PTR uid, stGUIVLanMember *pMemberIn)
{
	INT nCount = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;
	DWORD LinkState = pMemberIn->LinkState;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != uid)
				continue;

			if(pMember->HostName != pMemberIn->HostName)
			{
				pMember->HostName = pMemberIn->HostName; // Update host name.
				m_pTreeCtrl->SetItemText((HTREEITEM)pMember->GUIHandle, UTXFormatTreeHostName(pMember->HostName, pMember->vip));
			}

			if(pMember->LinkState != LinkState)
				if(pMember->LinkState == LS_OFFLINE)
				{
					pVLanInfo->nOnline++;
					pVLanInfo->UpdataToolTipString();
				}
				else if(LinkState == LS_OFFLINE)
				{
					pVLanInfo->nOnline--;
					pVLanInfo->UpdataToolTipString();
				}

			pMember->UpdataOnlineInfo(*pMemberIn);
			++nCount;
			break;
		}
	}

	return nCount;
}

UINT CGUINetworkManager::UpdateLinkStateFromStream(CStreamBuffer &sb)
{
	stMemberUpdateInfo Info;
	UINT i, nTotal, nFound = 0;

	sb >> nTotal;
	for(i = 0; i < nTotal; ++i)
	{
		sb.Read(&Info, sizeof(Info));
		if(SetMemberLinkState(0, Info.UserID, Info.LinkState, 0, 0, 0))
			++nFound;
	}

	printx("GUI:UpdateLinkStateFromStream %d/%d\n", nTotal, nFound);

	return nFound;
}

/*
BOOL CGUINetworkManager::UpdateSubnetRole(BOOL bHub, stGUIVLanInfo *pVLanInfo)
{
	stGUIVLanMember *pMember;
	USHORT DriverMapIndex, LinkState, us = INVALID_DM_INDEX;

	for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
	{
		pMember = pVLanInfo->MemberList.GetNext(mpos);
		if(pMember->LinkState == LS_OFFLINE)
			continue;

		UINT ExistCount = GetConnectedState(pVLanInfo, pMember->dwUserID, LinkState, DriverMapIndex);
*/
		// GUI won't update driver map index when state becomes LS_TRYING_TPT.
	//	if(!ExistCount || (DriverMapIndex == INVALID_DM_INDEX /*&& LinkState != LS_TRYING_TPT*/)) // Link state might change only in this case.
/*		{
			if((pVLanInfo->Flag & VF_HUB) && (pMember->Flag & VF_HUB))
			{
				if(pVLanInfo->IsNetOnline() && pMember->IsNetOnline())
				{
					ASSERT(pMember->LinkState != LS_NO_CONNECTION);
				//	ASSERT(pMember->DriverMapIndex != INVALID_DM_INDEX); This will failed in some complicated but valid operations.
				}
			}
			else if(!(pVLanInfo->Flag & VF_HUB) && !(pMember->Flag & VF_HUB))
			{
				if(pMember->LinkState != LS_NO_CONNECTION) // Last one that open tunnel.
				{
					ASSERT(pMember->DriverMapIndex != INVALID_DM_INDEX || pMember->LinkState == LS_TRYING_TPT);
					SetMemberLinkState(0, pMember->dwUserID, LS_NO_CONNECTION, &us, 0, 0);

					pMember->LinkState = LS_NO_CONNECTION; // Whether bHub is true or not, do the same thing.
					pMember->DriverMapIndex = INVALID_DM_INDEX;
				}
			}
			else
			{
				if(pVLanInfo->IsNetOnline() && pMember->IsNetOnline() && pMember->LinkState == LS_NO_CONNECTION)
					SetMemberLinkState(0, pMember->dwUserID, LS_TRYING_TPT, 0, 0, 0);
			}
		}
	}

	m_pTreeCtrl->RedrawWindow();

	return TRUE;
}
*/

/*
UINT CGUINetworkManager::GetConnectedState(stGUIVLanInfo *pSkipVNet, DWORD UID, USHORT &LinkState, USHORT &DriverMapIndex)
{
	INT nCount = 0;
	stGUIVLanInfo   *pVLanInfo;
	stGUIVLanMember *pMember;

	LinkState = LS_NO_CONNECTION;
	DriverMapIndex = INVALID_DM_INDEX;

	for(POSITION npos = m_VNetList.GetHeadPosition(); npos;)
	{
		pVLanInfo = m_VNetList.GetNext(npos);
		if(pVLanInfo == pSkipVNet)
			continue;

		for(POSITION mpos = pVLanInfo->MemberList.GetHeadPosition(); mpos; )
		{
			pMember = pVLanInfo->MemberList.GetNext(mpos);
			if(pMember->dwUserID != UID)
				continue;

			if(pMember->LinkState != LS_OFFLINE && pVLanInfo->IsNetOnline() && pMember->IsNetOnline() && NeedConnect(pVLanInfo, pMember))
			{
				LinkState = pMember->LinkState;
				DriverMapIndex = pMember->DriverMapIndex;
			}
			++nCount;
			break;
		}
	}

	return nCount;
}
*/


CVividTreeEx::CVividTreeEx()
{
	m_StatusIcon[LS_OFFLINE]        = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON1));
	m_StatusIcon[LS_NO_CONNECTION]  = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	m_StatusIcon[LS_RELAYED_TUNNEL] = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON3));
	m_StatusIcon[LS_CONNECTED]      = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON4));

	m_StatusIcon[LS_NO_TUNNEL]  = m_StatusIcon[LS_NO_CONNECTION];
	m_StatusIcon[LS_TRYING_TPT] = m_StatusIcon[LS_NO_CONNECTION];
	m_StatusIcon[LS_SERVER_RELAYED] = m_StatusIcon[LS_RELAYED_TUNNEL];

	m_pen1.CreatePen(PS_SOLID, 1, RGB(110, 165, 200));
	m_pen2.CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

    VERIFY( m_bmp_tree_closed.LoadBitmap( IDB_TREE_CLOSED ) );
    VERIFY( m_bmp_tree_open.LoadBitmap( IDB_TREE_OPENED ) );
    VERIFY( m_bmp_cloud.LoadBitmap( IDB_CLOUD ) );
    VERIFY( m_bmp_group.LoadBitmap( IDB_GROUP ) );
    VERIFY( m_bmp_group2.LoadBitmap( IDB_GROUP2 ) );

	m_pTable = 0;
}

CVividTreeEx::~CVividTreeEx()
{
//	Release(); // Can't call here.

    if (m_bmp_back_ground.GetSafeHandle())
        m_bmp_back_ground.DeleteObject();
    if (m_bmp_tree_closed.GetSafeHandle())
        m_bmp_tree_closed.DeleteObject();
    if (m_bmp_tree_open.GetSafeHandle())
        m_bmp_tree_open.DeleteObject();
    if (m_bmp_group.GetSafeHandle())
        m_bmp_group.DeleteObject();
    if (m_bmp_group2.GetSafeHandle())
        m_bmp_group2.DeleteObject();
}


BEGIN_MESSAGE_MAP(CVividTreeEx, VividTree)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, &CVividTreeEx::OnTvnItemexpanded)
END_MESSAGE_MAP()


void CVividTreeEx::Init(stTrafficTable *pTable)
{
	ASSERT(!m_pTable && pTable);

	m_pTable = pTable;
}

void CVividTreeEx::Release()
{
	//for(HTREEITEM hItem = GetRootItem(); hItem; hItem = GetNextSiblingItem(hItem))
	//{
	//	for(HTREEITEM hSub = GetNextItem(hItem, TVGN_CHILD); hSub; hSub = GetNextSiblingItem(hSub))
	//		SetItemData(hSub, 0); // Must set null or cause error when the contrl paints.
	//	SetItemData(hItem, 0);
	//}

	DeleteAllItems();
}

void CVividTreeEx::CacheLanguageData()
{
	m_String[LS_OFFLINE]        = GUILoadString(IDS_OFFLINE);
	m_String[LS_NO_CONNECTION]  = GUILoadString(IDS_NO_CONNECTION);
	m_String[LS_NO_TUNNEL]      = GUILoadString(IDS_NO_TUNNEL);
	m_String[LS_TRYING_TPT]     = GUILoadString(IDS_TRYING_TPT);
	m_String[LS_RELAYED_TUNNEL] = GUILoadString(IDS_RELAYED_TUNNEL);
	m_String[LS_SERVER_RELAYED] = m_String[LS_RELAYED_TUNNEL];
	m_String[LS_CONNECTED]      = GUILoadString(IDS_DIRECT_TUNNEL);

	m_String[SI_RELAY_HOST].Format(_T(" (%s)"), GUILoadString(IDS_RELAY_HOST));
	m_String[SI_HUB_CLIENT].Format(_T(" - %s"), GUILoadString(IDS_HUB_CLIENT));
	m_String[SI_SPOKE_CLIENT].Format(_T(" - %s"), GUILoadString(IDS_SPOKE_CLIENT));
	m_String[SI_SUBGROUP] = GUILoadString(IDS_SUBGROUP) + csGColon;
	m_String[SI_DEFAULT_GROUP] = GUILoadString(IDS_DEFAULT_GROUP);

	if(GetSafeHwnd())
		for(HTREEITEM hItem = GetRootItem(); hItem; hItem = GetNextSiblingItem(hItem))
			GetVLanInfo(hItem)->UpdataToolTipString();
}

CString CVividTreeEx::GetItemToolTip(HTREEITEM hItem)
{
	CString csTip;

	if(IsVLanItem(hItem))
	{
		stGUIVLanInfo *pVLanInfo = GetVLanInfo(hItem);
		if(pVLanInfo) // Must do check here.
			return pVLanInfo->ToolTipInfo;
	}
	else
	{
		stGUIObjectHeader *pObject = GetGUIObject(hItem);

		if(pObject) // Must do check here.
			if(IsVLanMember(pObject))
			{
				stGUIVLanMember *pMember = (stGUIVLanMember*)pObject;

				ASSERT(pMember->LinkState < LS_TOTAL);
				csTip = m_String[pMember->LinkState];

				stGUIVLanInfo *pVLanInfo = GetVLanInfo(GetParentItem(hItem));
				if(pVLanInfo && pVLanInfo->NetworkType == VNT_HUB_AND_SPOKE) // Must do check here.
					csTip += m_String[(pMember->Flag & VF_HUB) ? SI_HUB_CLIENT : SI_SPOKE_CLIENT];

				if(pMember->Flag & VF_RELAY_HOST)
				{
			//		printx("%S\n", pMember->HostName);
					csTip += m_String[SI_RELAY_HOST];
				}
			}
			else
			{
				stGUIVLanMember *pMember;
				stGUISubgroup *pVLanGroup = (stGUISubgroup*)pObject;

				UINT nOnline = 0, nTotal = 0;
				for(HTREEITEM hSub = GetNextItem(hItem, TVGN_CHILD); hSub; hSub = GetNextSiblingItem(hSub))
				{
					pMember = GetMemberInfo(hSub);
					if(!pMember) // Must do this check (20140407).
						continue;

					if(pMember->LinkState != LS_OFFLINE)
						++nOnline;
					++nTotal;
				}

				csTip.Format(_T("%s%s    (%d / %d)"), m_String[SI_SUBGROUP], pVLanGroup->VNetGroup.GroupName, nOnline, nTotal);
			}
	}

	return csTip;
}

CString CVividTreeEx::GetVLanName(HTREEITEM hItem)
{
	ASSERT(!GetParentItem(hItem));
	return ((stGUIVLanInfo*)GetItemData(hItem))->NetName;
}

stGUIVLanInfo* CVividTreeEx::GetVLanInfo(HTREEITEM hItem)
{
	HTREEITEM hParent = GetParentItem(hItem);
	if(hParent)
		hItem = hParent;
	return (stGUIVLanInfo*)GetItemData(hItem);
}

stGUIVLanMember* CVividTreeEx::GetMemberInfo(HTREEITEM hItem)
{
	ASSERT(GetParentItem(hItem));
	return (stGUIVLanMember*)GetItemData(hItem);
}

HTREEITEM CVividTreeEx::InsertVirtualLan(stGUIVLanInfo *pVLanInfo)
{
	HTREEITEM hItem = InsertItem(_T("                                        ")); // Used to maximize text rectangle to show tool tip string.
	pVLanInfo->GUIHandle = hItem;

	if(hItem)
	{
		SetItemData(hItem, (DWORD_PTR)pVLanInfo);

		if(pVLanInfo->m_nTotalGroup > 1)
		{
			ASSERT(pVLanInfo->m_nTotalGroup < MAX_VNET_GROUP_COUNT);
			InitDefaultGroup(pVLanInfo);
			for(UINT i = 0; i < pVLanInfo->m_nTotalGroup; ++i)
				InsertVNetGroup(&pVLanInfo->m_GroupArray[i], hItem);
		}
	}

	return hItem;
}

HTREEITEM CVividTreeEx::InsertHost(stGUIVLanMember *pMember, HTREEITEM hParentItem)
{
	ASSERT(hParentItem);

	HTREEITEM hSubItem = InsertItem(UTXFormatTreeHostName(pMember->HostName, pMember->vip), hParentItem);
	pMember->GUIHandle = hSubItem;

	if(hSubItem)
		SetItemData(hSubItem, (DWORD_PTR)pMember);

	return hSubItem;
}

HTREEITEM CVividTreeEx::InsertVNetGroup(stGUISubgroup *pGroup, HTREEITEM hParentItem, BOOL bAddHead)
{
	HTREEITEM hSubItem = InsertItem(_T("                                        "), hParentItem, bAddHead ? TVI_FIRST : TVI_LAST);
	pGroup->GUIHandle = hSubItem;

	if(hSubItem)
		SetItemData(hSubItem, (DWORD_PTR)pGroup);

	return hSubItem;
}

void CVividTreeEx::HideDefaultGroup(stGUIVLanInfo *pVNet)
{
	HTREEITEM hItem = (HTREEITEM)pVNet->m_GroupArray[0].GUIHandle;
	if(hItem)
	{
		DeleteItem(hItem);
		pVNet->m_GroupArray[0].GUIHandle = 0;
	}
}

void CVividTreeEx::DeleteVNetGroup(HTREEITEM hItem, stGUIVLanInfo *pVNet, UINT nDeletedIndex)
{
	ASSERT(IsVLanSubgroupItem(hItem));

	HTREEITEM hVNetItem = (HTREEITEM)pVNet->GUIHandle, hDefaultItem;
	if(pVNet->m_nTotalGroup == 1)
	{
		HTREEITEM hDefaultGroupItem = (HTREEITEM)pVNet->m_GroupArray[0].GUIHandle; // Delete default group.
		if(hDefaultGroupItem)
		{
			MoveAllMembers(hDefaultGroupItem, hVNetItem);
			DeleteItem(hDefaultGroupItem);
			pVNet->m_GroupArray[0].GUIHandle = 0;
		}
	}

	for(HTREEITEM hSub = GetNextItem(hVNetItem, TVGN_CHILD); hSub; hSub = GetNextSiblingItem(hSub)) // Re-link item to group object.
	{
		stGUIObjectHeader *pObject = (stGUIVLanMember*)GetItemData(hSub);
		if(!IsVLanGroup(pObject))
			break;

		UINT nIndex = pVNet->GetGroupIndex((stGUISubgroup*)pObject);
		if(nIndex > nDeletedIndex)
			SetItemData(hSub, (DWORD_PTR)&pVNet->m_GroupArray[nIndex - 1]);
	}

	hDefaultItem = (HTREEITEM)pVNet->m_GroupArray[pVNet->FindDefaultGroupIndex()].GUIHandle;
	if(!hDefaultItem)
		hDefaultItem = (pVNet->m_nTotalGroup == 1) ? hVNetItem : InsertVNetGroup(&pVNet->m_GroupArray[0], hVNetItem, TRUE);

	for(HTREEITEM hSub = GetNextItem(hItem, TVGN_CHILD); hSub; hSub = GetNextSiblingItem(hSub))
	{
		stGUIVLanMember *pMember = (stGUIVLanMember*)GetItemData(hSub);
		InsertHost(pMember, hDefaultItem);
	}

	DeleteItem(hItem);
}

UINT CVividTreeEx::MoveAllMembers(HTREEITEM hParentItem, HTREEITEM hNewParentItem)
{
	UINT nCount = 0;

	for(HTREEITEM hSub = GetNextItem(hParentItem, TVGN_CHILD), hCurrent; hSub; )
	{
		stGUIObjectHeader *pObject = (stGUIObjectHeader*)GetItemData(hSub);
		hCurrent = hSub;
		hSub = GetNextSiblingItem(hSub);

		if(!IsVLanMember(pObject))
			continue;

		DeleteItem(hCurrent);
		InsertHost((stGUIVLanMember*)pObject, hNewParentItem);
		++nCount;
	}

	return nCount;
}

void CVividTreeEx::MoveMember(stGUIVLanInfo *pVNet, stGUIVLanMember *pMember, UINT nNewGroupIndex)
{
	DeleteItem((HTREEITEM)pMember->GUIHandle);
	InsertHost(pMember, (HTREEITEM)pVNet->m_GroupArray[nNewGroupIndex].GUIHandle);
}

BOOL CVividTreeEx::HitExpandButton(CPoint &point, HTREEITEM hItem)
{
	CRect rect;
	stGUIObjectHeader *pObject = GetGUIObject(hItem);

	GetItemRect(hItem, rect, FALSE);
	GetTextRect(hItem, rect, FALSE);

	if(IsVLanItem(hItem))
	{
		rect.top += 3;
		rect.bottom -= 5;
		rect.left = rect.right + 10;
		rect.right = rect.left + 15;

		if(rect.left <= point.x && point.x <= rect.right)
			if(rect.top <= point.y && point.y <= rect.bottom)
				ToggleButton(hItem);

		return TRUE;
	}
	else if(IsVLanGroup(pObject))
	{
		rect.top += 3;
		rect.bottom -= 5;
		rect.left = 11;
		rect.right = rect.left + 16;

		if(rect.left <= point.x && point.x <= rect.right)
			if(rect.top <= point.y && point.y <= rect.bottom)
				ToggleButton(hItem);

		return TRUE;
	}

	return FALSE;
}

HICON CVividTreeEx::GetItemIcon(HTREEITEM hItem, BOOL *bUpload, BOOL *bDownload)
{
	stGUIVLanMember *pLanMember = (stGUIVLanMember*)GetItemData(hItem);

	if(!pLanMember)
		return m_StatusIcon[LS_OFFLINE];

	USHORT DriverMapIndex = pLanMember->DriverMapIndex;
	if(DriverMapIndex != INVALID_DM_INDEX)
	{
		*bUpload = m_pTable->bUpload[DriverMapIndex];
		*bDownload = m_pTable->bDownload[DriverMapIndex];
	}

	ASSERT(GetParentItem(hItem) && pLanMember->LinkState < LS_TOTAL);
	return m_StatusIcon[pLanMember->LinkState];
}

DWORD CVividTreeEx::GetItemTextColor(HTREEITEM hItem)
{
	if(GetParentItem(hItem))
	{
		stGUIVLanMember *pLanMember = (stGUIVLanMember*)GetItemData(hItem);
		if(pLanMember && pLanMember->LinkState == LS_OFFLINE)
			return RGB(125, 125, 125);
	}

	return 0;
}

void CVividTreeEx::GetVNetItemColor(HTREEITEM hItem, BOOL bSelect, CPen **pPen, DWORD &gscolor, DWORD &gecolor, DWORD &text)
{
	stGUIVLanInfo *pNetInfo = GetVLanInfo(hItem);

	if(pNetInfo && pNetInfo->IsNetOnline())
	{
		*pPen = &m_pen1;

		text = RGB(0, 0, 0);
		if(bSelect)
		{
			gscolor = RGB(220, 230, 240);
			gecolor = RGB(110, 185, 250);
		}
		else
		{
			gscolor = RGB(230, 245, 240);
			gecolor = RGB(200, 230, 250);
		}
	}
	else
	{
		*pPen = &m_pen2;
		if(bSelect)
		{
			text = RGB(100, 100, 100);
			gscolor = RGB(240, 240, 240);
			gecolor = RGB(185, 185, 185);
		}
		else
		{
			text = RGB(120, 120, 120);
			gscolor = RGB(245, 250, 245);
			gecolor = RGB(225, 225, 225);
		}
	}
}

CString CVividTreeEx::GetVNetItemName(HTREEITEM hItem)
{
	stGUIVLanInfo *pNetInfo = GetVLanInfo(hItem);
	if(pNetInfo)
		return pNetInfo->NetName;
	return 0;
}

LRESULT CVividTreeEx::OnTvmHitTest(WPARAM wParam, LPARAM lParam)
{
	TVHITTESTINFO *pHitInfo = (TVHITTESTINFO*)lParam;
//	LRESULT lr = VividTree::DefWindowProc(TVM_HITTEST, wParam, lParam);
	HTREEITEM hItem = 0;//TreeView_HitTest(GetSafeHwnd(), pHitInfo);

	if(pHitInfo && pHitInfo->flags & TVHT_ONITEMBUTTON)
	{
		pHitInfo->flags &= ~TVHT_ONITEMBUTTON;
		printx("On item button!\n");
	}

//	printx("On item button!\n");

	return (LRESULT)hItem;
}

void CVividTreeEx::OnLButtonDown(UINT nFlags, CPoint point)
{
	TVHITTESTINFO HitInfo = {0};
	HitInfo.pt = point;
	HTREEITEM hItem = TreeView_HitTest(GetSafeHwnd(), &HitInfo);

	if(HitInfo.flags & TVHT_ONITEMBUTTON) // Prevent default behavior that toggle item of the tree control.
	{
		Select(hItem, TVGN_CARET);
		return;
	}

	if(hItem)
	{
		HitExpandButton(point, hItem);
		Select(hItem, TVGN_CARET);
	}

	VividTree::OnLButtonDown(nFlags, point);
}

void CVividTreeEx::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	HTREEITEM hItem = GetSelectedItem();
	if(hItem && HitExpandButton(point, hItem))
		return;

	VividTree::OnLButtonDblClk(nFlags, point);
}

void CVividTreeEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	TVHITTESTINFO HitInfo = {0};
	HitInfo.pt = point;
	HTREEITEM hItem = TreeView_HitTest(GetSafeHwnd(), &HitInfo);
	if(hItem)
		Select(hItem, TVGN_CARET);

	VividTree::OnRButtonDown(nFlags, point);
}

void CVividTreeEx::OnTvnItemexpanded(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	Invalidate(FALSE);
	*pResult = 0;
}

void CVividTreeEx::DrawItems( CDC *pDC )
{
	HTREEITEM show_item, parent;
	CRect rc_item;
	CString name;
	COLORREF color;
	DWORD tree_style;
//	BITMAP bm;
	CDC dc_mem;
	int count = 0;
	int state;
	bool selected;
	bool has_children;
	stGUIObjectHeader *pObject;

	show_item = GetFirstVisibleItem();
	if ( show_item == NULL )
		return;

	CRect NetRect, HostRect, rect;
	GetTextRect(show_item, NetRect, FALSE);
	NetRect.left += 12;
	GetTextRect(show_item, HostRect, TRUE);

	dc_mem.CreateCompatibleDC(NULL);
	color = pDC->GetTextColor();
	tree_style = ::GetWindowLong( m_hWnd, GWL_STYLE );

	do
	{
		state = GetItemState( show_item, TVIF_STATE );
		parent = GetParentItem( show_item );
		has_children = ItemHasChildren( show_item ) || parent == NULL;
		selected = (state & TVIS_SELECTED) && ((this == GetFocus()) || (tree_style & TVS_SHOWSELALWAYS));
		pObject = (stGUIObjectHeader*)GetItemData(show_item);

		if(!pObject) // Must do this check. Sometimes OS forces GUI updating while msg handlers still work in progress.
			continue;

		if ( GetItemRect( show_item, rc_item, TRUE ) )
		{
	//		printx("Item rect %d ----> %d\n", rc_item.left, rc_item.right);
			rect.top = rc_item.top + 1;
			rect.bottom = rc_item.bottom - 1;
			rect.right = m_h_size + m_h_offset - 1;
			rect.left = m_h_offset + 2;

			if(!parent)
			{
				DWORD color_text, color_from, color_to;
				CPen *pen;
				GetVNetItemColor(show_item, selected, &pen, color_from, color_to, color_text);
				CPen *pOldPen = (CPen*)pDC->SelectObject(pen);
				pDC->RoundRect(rect.left + 4, rect.top, rect.right - 4, rect.bottom - 1, 2, 2);
				pDC->SelectObject(pOldPen);

				rect.DeflateRect(5, 2);
				rect.top -= 1;
				GradientFillRect(pDC, rect, color_from, color_to, TRUE);

				CBitmap *pOldBitmap;
				if ( state & TVIS_EXPANDED ) // Draw an Open/Close button.
					pOldBitmap = dc_mem.SelectObject(&m_bmp_tree_open);
				else
					pOldBitmap = dc_mem.SelectObject(&m_bmp_tree_closed);
				pDC->BitBlt(NetRect.right + 10, rect.top + 1, 20, 24, &dc_mem, 0, 0, SRCAND );

				dc_mem.SelectObject(&m_bmp_cloud);
			//	pDC->BitBlt(rect.left + 5, rect.top + 2, 16, 16, &dc_mem, 0, 0, SRCAND );
				pDC->TransparentBlt(rect.left + 5, rect.top + 2, 16, 16, &dc_mem, 0, 0, 16, 16, RGB(255, 0, 255));
				dc_mem.SelectObject(pOldBitmap);

				name = GetVNetItemName(show_item);

				NetRect.top = rc_item.top;
				NetRect.bottom = rc_item.bottom;
				pDC->SetTextColor(color_text);

				CFont *pOldFont = pDC->SelectObject(&GBoldFont);
				pDC->DrawText(name, NetRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP);
				pDC->SelectObject(pOldFont);
			}
			else if(IsVLanMember(pObject))
			{
				if ( selected )
				{
					rect.DeflateRect(4, 1);
					pDC->FillRect(&rect, &m_ItemSelectedBk);
				//	GradientFillRect( pDC, rect, from, from, FALSE );
				}

				CFont *f = GetFont();
				LOGFONT lf;
				f->GetLogFont(&lf);


				name = GetItemText( show_item );

				HostRect.top = rc_item.top;
				HostRect.bottom = rc_item.bottom;
				pDC->SetTextColor( GetItemTextColor(show_item) );
				pDC->DrawText( name, HostRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

				BOOL bUpload = 0, bDownload = 0;
				HICON icon = GetItemIcon(show_item, &bUpload, &bDownload);

				rc_item.left = 41; // Force to fix RDP bug.
				if( icon != NULL )
		//			DrawIconEx( pDC->m_hDC, rc_item.left - 18, rc_item.top, icon, 16, 16,0,0, DI_NORMAL );
					DrawIconEx( pDC->m_hDC, rc_item.left - 15, rc_item.top + 7, icon, 10, 10, 0, 0, DI_NORMAL ); // 2011/01/11 Mod.

				if(bUpload || bDownload)
				{
					CPen pen(PS_SOLID, 1, RGB(50, 128, 64));
					CPen *pOldPen = pDC->SelectObject(&pen);
					INT x, y;
					if(bUpload)
					{
						x = rc_item.left - 26; y = rc_item.top + 8;
						pDC->MoveTo(x, y);
						pDC->LineTo(x, y + 8);
						pDC->MoveTo(x, y);
						pDC->LineTo(x - 3, y + 3);
						pDC->MoveTo(x, y);
						pDC->LineTo(x + 3, y + 3);
					}
				//	pDC->DrawText(CString("¡ô"), CRect(rc_item.left - 32, rect.top, rect.right, rect.bottom), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
					if(bDownload)
					{
						x = rc_item.left - 22; y = rc_item.top + 6 + 9;
						pDC->MoveTo(x, y - 7);
						pDC->LineTo(x, y);
						pDC->MoveTo(x, y);
						pDC->LineTo(x - 3, y - 3);
						pDC->MoveTo(x, y);
						pDC->LineTo(x + 3, y - 3);
					}
					pDC->SelectObject(pOldPen);
				//	pDC->DrawText(CString("¡õ"), CRect(rc_item.left - 27, rect.top, rect.right, rect.bottom), DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
				}
			}
			else
			{
				if ( selected )
				{
					rect.DeflateRect(4, 1);
					pDC->FillRect(&rect, &m_ItemSelectedBk);
				//	GradientFillRect( pDC, rect, from, from, FALSE );
				}

				DWORD dwTextColor = RGB(63, 120, 178);
				CBitmap *pOldBitmap, *pIconBitmap = &m_bmp_group;
				if ( state & TVIS_EXPANDED ) // Draw an Open/Close button.
					pOldBitmap = dc_mem.SelectObject(&m_bmp_tree_open);
				else
					pOldBitmap = dc_mem.SelectObject(&m_bmp_tree_closed);
				pDC->BitBlt(12, rc_item.top + 4, 20, 24, &dc_mem, 0, 0, SRCAND );

				stGUISubgroup *pGroup = (stGUISubgroup*)pObject;
				name = pGroup->VNetGroup.GroupName;

				if(pGroup->VNetGroup.Flag & VGF_STATE_OFFLINE || !pGroup->pVNetInfo->IsSubgroupOnline(pGroup))
				{
					dwTextColor = RGB(127, 127, 127);
					pIconBitmap = &m_bmp_group2;
				}

				if(pGroup->VNetGroup.Flag & VGF_DEFAULT_GROUP)
					pDC->FillSolidRect( 36, rc_item.top + 8, 8, 8, RGB(130, 240, 160));

				pOldBitmap = dc_mem.SelectObject(pIconBitmap);
				pDC->BitBlt( 32, rc_item.top + 4, 16, 16, &dc_mem, 0, 0, SRCAND); // Draw group icon.
				dc_mem.SelectObject(pOldBitmap);

				HostRect.top = rc_item.top;
				HostRect.left += 10;
				HostRect.bottom = rc_item.bottom;
				pDC->SetTextColor( dwTextColor );
				pDC->DrawText( name, HostRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

				HostRect.left -= 10;
			}

		//	if ( selected )
		//	{
		//		if ( !has_children  )
		//			pDC->SetTextColor( GetSysColor(COLOR_HIGHLIGHTTEXT) );
		//		COLORREF col = pDC->GetBkColor();
		//		pDC->SetBkColor( GetSysColor(COLOR_HIGHLIGHT) );
		//		pDC->DrawText( name, rc_item, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
		//		pDC->SetTextColor( color );
		//		pDC->SetBkColor( col );
		//	}

			//if ( state & TVIS_BOLD )
			//	pDC->SelectObject( font );
		}
	}
	while ( (show_item = GetNextVisibleItem( show_item )) != NULL );
}


