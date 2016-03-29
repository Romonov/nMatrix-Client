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


#include "VividTree.h"
#include "CnMatrixCore.h"


class CGUINetworkManager
{
public:

	CGUINetworkManager()
	:m_pTreeCtrl(NULL)
	{
		ASSERT(sizeof(stGUIVLanMember) != sizeof(stGUIVLanInfo));
		ASSERT(sizeof(stGUIVLanMember) != sizeof(stGUISubgroup));
		ASSERT(sizeof(stGUIVLanInfo) != sizeof(stGUISubgroup));
	}
	~CGUINetworkManager()
	{
		Release();
	}


	void SetGUIObject(CTreeCtrl *pTreeCtrl) { ASSERT(!m_pTreeCtrl); m_pTreeCtrl = pTreeCtrl; }
	CList<stGUIVLanInfo*>& GetVNetList() { return m_VNetList; }
	UINT GetVNetCount() { return m_VNetList.GetCount(); }

	void Release();
	stGUIVLanInfo* CreateVNet();
	stGUIVLanInfo* FindVNet(DWORD_PTR VNetID, const TCHAR *pName = NULL);
	BOOL FindRelayHost(DWORD_PTR VNetID, DWORD_PTR &UID, IPV4 &vip, CString &HostName);

	BOOL OfflineSubnet(BOOL bOnline, DWORD_PTR VNetID);
	BOOL OfflineNetMember(DWORD_PTR VNetID, DWORD_PTR UID, BOOL bOnline);
	void UpdateAllMemberFromVNet(stGUIVLanInfo *pVLanInfo);
	BOOL UpdateVLanFlag(DWORD_PTR VNetID, DWORD dwFlag);
	BOOL UpdateMemberRole(DWORD_PTR NetID, DWORD_PTR UserID, DWORD dwFlag, DWORD dwMask);
	UINT UpdateHostData(DWORD_PTR VNetID, DWORD_PTR UserID, DWORD type, void* pData, UINT nDataSize); // VNetID can be null.
	UINT UpdateMemberProfile(void *pGUI, DWORD UserID, UINT nType, void* pData, UINT nDataSize);

	stGUIVLanInfo* RemoveUser(DWORD_PTR VNetID, DWORD_PTR UID);
	BOOL RemoveVLan(DWORD_PTR VNetID);

	INT SetMemberLinkState(DWORD_PTR dwSkipNetID, DWORD_PTR uid, USHORT LinkState, USHORT *pDriverMapIndex, CIpAddress *pNewAddress, USHORT *pNewEPort, DWORD_PTR *dwRHNetID = 0);
	INT UpdateHost(DWORD_PTR uid, stGUIVLanMember *pMemberIn);

	UINT UpdateLinkStateFromStream(CStreamBuffer &sb);

//	BOOL UpdateSubnetRole(BOOL bHub, stGUIVLanInfo *pVLanInfo);
//	UINT GetConnectedState(stGUIVLanInfo *pSkipVNet, DWORD UID, USHORT &LinkState, USHORT &DriverMapIndex);


protected:

	friend class CVPNClientDlg;

	CTreeCtrl *m_pTreeCtrl;
	CList<stGUIVLanInfo*> m_VNetList;


};


struct stTrafficTable
{
	DWORD DataIn[MAX_NETWORK_CLIENT], DataOut[MAX_NETWORK_CLIENT];
	DWORD DataInCache[MAX_NETWORK_CLIENT], DataOutCache[MAX_NETWORK_CLIENT];
	bool  bUpload[MAX_NETWORK_CLIENT], bDownload[MAX_NETWORK_CLIENT];
	UINT  nCount;
	BOOL  bFirstTimeUpdateTrafficTable;

	__forceinline void ZeroCacheTable()
	{
		ZeroMemory(DataInCache, sizeof(DataInCache));
		ZeroMemory(DataOutCache, sizeof(DataOutCache));
		ZeroMemory(bUpload, sizeof(bUpload));
		ZeroMemory(bDownload, sizeof(bDownload));
		bFirstTimeUpdateTrafficTable = TRUE;
//		printx("Bool table size: %d, %d.\n", sizeof(bUpload), sizeof(bDownload));
	}
	__forceinline void SetZero(USHORT DriverMapIndex)
	{
		ASSERT(DriverMapIndex < MAX_NETWORK_CLIENT);
		DataIn[DriverMapIndex] = DataOut[DriverMapIndex] = DataInCache[DriverMapIndex] = DataOutCache[DriverMapIndex] = 0;
		bUpload[DriverMapIndex] = bDownload[DriverMapIndex] = false;
	}

//	void ZeroInit() { ZeroMemory(this, sizeof(*this)); }
};


enum UPDATE_DATA_TYPE
{
	UDF_ONLINE_TIME,
};


//
// This is a custom class.
// Don't call SetItemData & GetItemData and any remove method directly.
//

class CVividTreeEx : public VividTree
{
public:

	CVividTreeEx();
	virtual ~CVividTreeEx();

	void TraverseSample()
	{
		for(HTREEITEM hItem = GetRootItem(); hItem; hItem = GetNextSiblingItem(hItem))
		{
			stGUIVLanInfo *pVLanInfo = (stGUIVLanInfo*)GetItemData(hItem);

			for(HTREEITEM hSub = GetNextItem(hItem, TVGN_CHILD); hSub; hSub = GetNextSiblingItem(hSub))
			{
				stGUIVLanMember *pMember = (stGUIVLanMember*)GetItemData(hSub);
			}
		}
	}


	void Init(stTrafficTable *pTable);
	void Release();
	void CacheLanguageData();
	void InitDefaultGroup(stGUIVLanInfo *pVLanInfo)	{ pVLanInfo->InitDefaultGroup(m_String[SI_DEFAULT_GROUP]); }

	CString GetItemToolTip(HTREEITEM hItem);
	CString GetVLanName(HTREEITEM hItem);

	stGUIVLanInfo* GetVLanInfo(HTREEITEM hItem);
	stGUIVLanMember* GetMemberInfo(HTREEITEM hItem);
	HTREEITEM InsertVirtualLan(stGUIVLanInfo *pVLanInfo);
	HTREEITEM InsertHost(stGUIVLanMember *pMember, HTREEITEM hParentItem);
	HTREEITEM InsertVNetGroup(stGUISubgroup *pGroup, HTREEITEM hParentItem, BOOL bAddHead = FALSE);

	void HideDefaultGroup(stGUIVLanInfo *pVNet);
	void DeleteVNetGroup(HTREEITEM hGroupItem, stGUIVLanInfo *pVNet, UINT nDeletedIndex);
	UINT MoveAllMembers(HTREEITEM hParentItem, HTREEITEM hNewParentItem);
	void MoveMember(stGUIVLanInfo *pVNet, stGUIVLanMember *pMember, UINT nNewGroupIndex);
	BOOL HitExpandButton(CPoint &point, HTREEITEM hItem);

	stGUIObjectHeader* GetGUIObject(HTREEITEM hItem) { return (stGUIObjectHeader*)GetItemData(hItem); }
	BOOL IsVLanItem(HTREEITEM hItem) { return !GetParentItem(hItem); }
	BOOL IsVLanMemberItem(HTREEITEM hItem) { return IsVLanMember(GetGUIObject(hItem)); }
	BOOL IsVLanSubgroupItem(HTREEITEM hItem) { return IsVLanGroup(GetGUIObject(hItem)); }
	BOOL IsVLanMember(stGUIObjectHeader *pObject) { return pObject->dwObjectSize == sizeof(stGUIVLanMember); }
	BOOL IsVLanGroup(stGUIObjectHeader *pObject) { return pObject->dwObjectSize == sizeof(stGUISubgroup); }

	// The tree methods is for VividTree class only.
	virtual HICON GetItemIcon(HTREEITEM hItem, BOOL *bUpload, BOOL *bDownload);
	virtual DWORD GetItemTextColor(HTREEITEM hItem);
	virtual void GetVNetItemColor(HTREEITEM hItem, BOOL bSelect, CPen **pPen, DWORD &gscolor, DWORD &gecolor, DWORD &text);
	virtual CString GetVNetItemName(HTREEITEM hItem);
	virtual void DrawItems( CDC* pDC );

	afx_msg LRESULT OnTvmHitTest(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTvnItemexpanded(NMHDR *pNMHDR, LRESULT *pResult);


	enum STRING_INDEX
	{
		SI_RELAY_HOST = LS_TOTAL,
		SI_HUB_CLIENT,
		SI_SPOKE_CLIENT,
		SI_SUBGROUP,
		SI_DEFAULT_GROUP,

		SI_TOTAL
	};


protected:

	HICON m_StatusIcon[LS_TOTAL];
	stTrafficTable *m_pTable;
	CPen m_pen1, m_pen2;
	CString m_String[SI_TOTAL];

	CBitmap m_bmp_tree_closed;
	CBitmap m_bmp_tree_open;
	CBitmap m_bmp_cloud;
	CBitmap m_bmp_group;
	CBitmap m_bmp_group2;

	DECLARE_MESSAGE_MAP()


};


