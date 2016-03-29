///////////////////////////////////////////////////////////////////////////
// VividTree.h : Definition of Class VividTree
///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2005 Jim Alsup  All rights reserved
//              email: ja.alsup@gmail.com
//
// License: This code is provided "as is" with no expressed or implied 
//          warranty. You may use, or derive works from this file without
//          any restrictions except those listed below.
//
//        - This original header must be kept in the derived work.
//
//        - If your derived work is distributed in any form, you must
//          notify the author via the email address above and provide a 
//          short description of the product and intended audience.  
//
//        - You may not sell this code or a derived version of it as part of 
//          a comercial code library. 
//
//        - Offering the author a free licensed copy of any end product 
//          using this code is not required, but does endear you with a 
//          bounty of good karma.
//
///////////////////////////////////////////////////////////////////////////


#pragma once


#define VT_TREE_ITEMHEIGHT 25


extern void GradientFillRect( CDC *pDC, CRect &rect, COLORREF col_from, COLORREF col_to, bool vert_grad );


class VividTree : public CTreeCtrl
{
public:
	VividTree();
	virtual ~VividTree();

	void SetBkGradients( COLORREF from, COLORREF to ){	m_gradient_bkgd_from = from; m_gradient_bkgd_to = to; }
	COLORREF GetBkGradientFrom( ){	return m_gradient_bkgd_from; }
	COLORREF GetBkGradientTo( ){ return m_gradient_bkgd_to; }

	enum BkMode { BK_MODE_BMP = 0, BK_MODE_GRADIENT, BK_MODE_FILL };
	BkMode GetBkMode( ) { return m_bkgd_mode; }
	void SetBkMode( BkMode bkgd_mode ) { m_bkgd_mode = bkgd_mode; }

	bool GetBitmapTiledMode() { return m_bmp_tiled_mode; }
	void SetBitmapTiledMode( bool tiled ) {	m_bmp_tiled_mode = tiled; }
	void SetBitmapID( UINT id );

	bool GetGradientHorz() { return m_gradient_horz; }
	void SetGradientHorz( bool horz ) { m_gradient_horz = horz; }

//	virtual HICON GetItemIcon( HTREEITEM item );
	virtual HICON GetItemIcon(HTREEITEM hItem, BOOL *bUpload = 0, BOOL *bDownload = 0);
	virtual DWORD GetItemTextColor(HTREEITEM item) { return 0; }
	virtual void GetVNetItemColor(HTREEITEM hItem, BOOL bSelect, CPen **pPen, DWORD &gscolor, DWORD &gecolor, DWORD &text) {}
	virtual CString GetVNetItemName(HTREEITEM hItem) { return 0; }

	bool ItemIsVisible(HTREEITEM item);
	void ToggleButton(HTREEITEM hItem);
	void GetTextRect(HTREEITEM hItem, CRect &rect, BOOL bMember);


protected:

	COLORREF m_gradient_bkgd_from;		// Gradient variables
	COLORREF m_gradient_bkgd_to;
	COLORREF m_gradient_bkgd_sel;
	bool     m_gradient_horz;			// horz or vertical gradient

	BkMode  m_bkgd_mode;				// Current Background mode
    CBitmap m_bmp_back_ground;			// Background bitmap image

//	CFont  m_BoldFont;
	CBrush m_ItemSelectedBk;
	bool   m_bmp_tiled_mode;			// Tile background image

	CRect m_rect;						// The client rect when drawing
	int   m_h_offset;					// 0... -x (scroll offset)
	int   m_h_size;						// width of unclipped window
	int   m_v_offset;					// 0... -y (scroll offset)
	int   m_v_size;						// height of unclipped window

	HICON m_icon;

	DECLARE_MESSAGE_MAP()

	void DrawBackGround( CDC* pDC );
	virtual void DrawItems( CDC* pDC );

	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);


};


