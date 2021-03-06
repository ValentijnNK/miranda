/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef _CLC_H_
#define _CLC_H_

#define CLB_TOPLEFT       0
#define CLB_STRETCHV      1
#define CLB_STRETCHH      2	 //and tile vertically
#define CLB_STRETCH       3

#define CLBM_TYPE         0x00FF

#define CLBF_TILEH        0x1000
#define CLBF_TILEV        0x2000
#define CLBF_PROPORTIONAL 0x4000
#define CLBF_SCROLL       0x8000
#define CLBF_TILEVTOROWHEIGHT        0x0100

#define FONTID_CONTACTS    0
#define FONTID_INVIS       1
#define FONTID_OFFLINE     2
#define FONTID_NOTONLIST   3
#define FONTID_GROUPS      4
#define FONTID_GROUPCOUNTS 5
#define FONTID_DIVIDERS    6
#define FONTID_OFFINVIS    7
#define FONTID_STATUSMSG   8
#define FONTID_GROUPSCLOSED   9
#define FONTID_CONTACTSHOVER	10

#define TIMERID_DELAYEDREPAINT   16
#define TIMERID_SUBEXPAND 21

#define CONTACTF_STATUSMSG 64

#define INTM_STATUSMSGCHANGED	(WM_USER+26)

#define CLS_SHOWSTATUSMESSAGES 0x800

#define CLS_EX_MULTICOLUMNALIGNLEFT		0x00001000

#define MAXSTATUSMSGLEN		256

struct ClcContact {
	BYTE type;
	BYTE flags;
	union {
		struct {
			WORD iImage;
			HANDLE hContact;
		};
		struct {
			WORD groupId;
			struct ClcGroup *group;
		};
	};
	BYTE iExtraImage[MAXEXTRACOLUMNS];
	TCHAR szText[120-MAXEXTRACOLUMNS];
	char * proto;	// MS_PROTO_GETBASEPROTO

	struct ClcContact *subcontacts;
	BYTE SubAllocated;
	BYTE SubExpanded;
	BYTE isSubcontact;
	TCHAR szStatusMsg[MAXSTATUSMSGLEN];
};

struct ClcData {
	struct ClcGroup list;
	int rowHeight;
	int yScroll;
	int selection;
	struct ClcFontInfo fontInfo[FONTID_MAX+1];
	int scrollTime;
	HIMAGELIST himlHighlight;
	int groupIndent;
	TCHAR szQuickSearch[128];
	int iconXSpace;
	HWND hwndRenameEdit;
	COLORREF bkColour,selBkColour,selTextColour,hotTextColour,quickSearchColour;
	int iDragItem,iInsertionMark;
	int dragStage;
	POINT ptDragStart;
	int dragAutoScrolling;
	int dragAutoScrollHeight;
	int leftMargin;
	int insertionMarkHitHeight;
	HBITMAP hBmpBackground;
	int backgroundBmpUse,bkChanged;
	int iHotTrack;
	int gammaCorrection;
	DWORD greyoutFlags;			  //see m_clc.h
	DWORD offlineModes;
	DWORD exStyle;
	POINT ptInfoTip;
	int infoTipTimeout;
	HANDLE hInfoTipItem;
	HIMAGELIST himlExtraColumns;
	int extraColumnsCount;
	int extraColumnSpacing;
	int checkboxSize;
	int showSelAlways;
	int showIdle;
	int noVScrollbar;
	int useWindowsColours;
	int NeedResort;

	DWORD style;
	SortedList lCLCContactsCache;
	BYTE HiLightMode;
	BYTE doubleClickExpand;
	int MetaIgnoreEmptyExtra;
};

//clcidents.c
int GetRowsPriorTo(struct ClcGroup *group,struct ClcGroup *subgroup,int contactIndex);
int FindItem(HWND hwnd,struct ClcData *dat,HANDLE hItem,struct ClcContact **contact,struct ClcGroup **subgroup,int *isVisible);
int GetRowByIndex(struct ClcData *dat,int testindex,struct ClcContact **contact,struct ClcGroup **subgroup);
void ClearRowByIndexCache();

//clcitems.c
struct ClcGroup *AddGroup(HWND hwnd,struct ClcData *dat,const TCHAR *szName,DWORD flags,int groupId,int calcTotalMembers);
void FreeGroup(struct ClcGroup *group);
int AddInfoItemToGroup(struct ClcGroup *group,int flags,const TCHAR *pszText);
void FreeContact(struct ClcContact *p);
void RebuildEntireList(HWND hwnd,struct ClcData *dat);
struct ClcGroup *RemoveItemFromGroup(HWND hwnd,struct ClcGroup *group,struct ClcContact *contact,int updateTotalCount);
void DeleteItemFromTree(HWND hwnd,HANDLE hItem);
void AddContactToTree(HWND hwnd,struct ClcData *dat,HANDLE hContact,int updateTotalCount,int checkHideOffline);
void SortCLC(HWND hwnd,struct ClcData *dat,int useInsertionSort);
int GetGroupContentsCount(struct ClcGroup *group,int visibleOnly);
int GetNewSelection(struct ClcGroup *group,int selection, int direction);
void SaveStateAndRebuildList(HWND hwnd,struct ClcData *dat);

//clcmsgs.c
LRESULT ProcessExternalMessages(HWND hwnd,struct ClcData *dat,UINT msg,WPARAM wParam,LPARAM lParam);

//clcutils.c
void RecalcScrollBar(HWND hwnd,struct ClcData *dat);
void BeginRenameSelection(HWND hwnd,struct ClcData *dat);
int HitTest(HWND hwnd,struct ClcData *dat,int testx,int testy,struct ClcContact **contact,struct ClcGroup **group,DWORD *flags);
void ScrollTo(HWND hwnd,struct ClcData *dat,int desty,int noSmooth);
void LoadClcOptions(HWND hwnd,struct ClcData *dat);

//clcpaint.c
void PaintClc(HWND hwnd,struct ClcData *dat,HDC hdc,RECT *rcPaint);

//clcopts.c
int ClcOptInit(WPARAM wParam,LPARAM lParam);

//clcfiledrop.c
void InitFileDropping(void);
void FreeFileDropping(void);
void RegisterFileDropping(HWND hwnd);
void UnregisterFileDropping(HWND hwnd);

//cluiservices.c
void FreeProtocolData( void );

int GetContactCachedStatus(HANDLE hContact);
char *GetContactCachedProtocol(HANDLE hContact);

#define CLUI_SetDrawerService "CLUI/SETDRAWERSERVICE"
typedef struct {
	int cbSize;
	char *PluginName;
	char *Comments;
	char *GetDrawFuncsServiceName;

} DrawerServiceStruct,*pDrawerServiceStruct ;

#define CLUI_EXT_FUNC_PAINTCLC	1

typedef struct {
	int cbSize;
	void (*PaintClc)(HWND,struct ClcData *,HDC,RECT *,int ,ClcProtoStatus *,HIMAGELIST);

} ExternDrawer,*pExternDrawer ;

ExternDrawer SED;

#define CLM_SETEXTRACOLUMNSSPACE   (CLM_FIRST+73)   //wParam=extra space between icons

void FreeAndNil( void** p );

#endif /* _CLC_H_ */
