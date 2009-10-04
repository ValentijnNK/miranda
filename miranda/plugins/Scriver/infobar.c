/*
Scriver

Copyright 2000-2009 Miranda ICQ/IM project,

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
#include "commonheaders.h"
#include "infobar.h"
#include "richutil.h"

extern HINSTANCE g_hInst;


void UpdateInfoBar(InfobarWindowData* idat) {
	HWND hwnd = idat->hWnd;
	struct MessageWindowData *dat = idat->mwd;
		{
			TCHAR *szContactName = GetNickname(dat->windowData.hContact, dat->szProto);
			TCHAR *szContactStatusMsg = DBGetStringT(dat->windowData.hContact, "CList", "StatusMsg");
			SETTEXTEX  st;
			st.flags = ST_DEFAULT;
#ifdef _UNICODE
			st.codepage = 1200;
#else
			st.codepage = CP_ACP;
#endif
			SendMessage(GetDlgItem(hwnd, IDC_INFOBAR_NAME), EM_SETTEXTEX, (WPARAM) &st, (LPARAM)szContactName);
			SendMessage(GetDlgItem(hwnd, IDC_INFOBAR_STATUS), EM_SETTEXTEX, (WPARAM) &st, (LPARAM)L"I wwewewew w ew ewe  sd ada dadasdad  adfsdfsdfsdfsdf  f sdf sdf sdfdfdsfsdf  fggdfgdfgdfgdfg  gdfgdfgdg  am looking at www.onet.pl");
		}
		{
			CHARFORMAT2 cf2 = {0};
			LOGFONT lf;
			DWORD colour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INFOBARBKGCOLOUR, SRMSGDEFSET_INFOBARBKGCOLOUR);
			SendDlgItemMessage(hwnd, IDC_INFOBAR_NAME, EM_SETBKGNDCOLOR, 0, colour);
			colour = DBGetContactSettingDword(NULL, SRMMMOD, SRMSGSET_INFOBARBKGCOLOUR, SRMSGDEFSET_INFOBARBKGCOLOUR);
			SendDlgItemMessage(hwnd, IDC_INFOBAR_STATUS, EM_SETBKGNDCOLOR, 0, colour);
			//InvalidateRect(GetDlgItem(hwndDlg, IDC_MESSAGE), NULL, FALSE);
			LoadMsgDlgFont(MSGFONTID_INFOBAR_NAME, &lf, &colour);
			cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_BOLD | CFM_ITALIC;
			cf2.cbSize = sizeof(cf2);
			cf2.crTextColor = colour;
			cf2.bCharSet = lf.lfCharSet;
			_tcsncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
			cf2.dwEffects = ((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0);
			cf2.wWeight = (WORD)lf.lfWeight;
			cf2.bPitchAndFamily = lf.lfPitchAndFamily;
			cf2.yHeight = abs(lf.lfHeight) * 15;
			SendDlgItemMessageA(hwnd, IDC_INFOBAR_NAME, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);

			LoadMsgDlgFont(MSGFONTID_INFOBAR_STATUS, &lf, &colour);
			cf2.dwMask = CFM_COLOR | CFM_FACE | CFM_CHARSET | CFM_SIZE | CFM_WEIGHT | CFM_BOLD | CFM_ITALIC;
			cf2.cbSize = sizeof(cf2);
			cf2.crTextColor = colour;
			cf2.bCharSet = lf.lfCharSet;
			_tcsncpy(cf2.szFaceName, lf.lfFaceName, LF_FACESIZE);
			cf2.dwEffects = ((lf.lfWeight >= FW_BOLD) ? CFE_BOLD : 0) | (lf.lfItalic ? CFE_ITALIC : 0);
			cf2.wWeight = (WORD)lf.lfWeight;
			cf2.bPitchAndFamily = lf.lfPitchAndFamily;
			cf2.yHeight = abs(lf.lfHeight) * 15;
			SendDlgItemMessageA(hwnd, IDC_INFOBAR_STATUS, EM_SETCHARFORMAT, 0, (LPARAM)&cf2);
		
		}
}

static LRESULT CALLBACK InfobarWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	InfobarWindowData* idat = (InfobarWindowData *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!idat && msg!=WM_INITDIALOG) return FALSE;
	switch (msg) {
	case WM_INITDIALOG:
		idat = (InfobarWindowData *) lParam;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)idat);
		SendDlgItemMessage(hwnd, IDC_INFOBAR_NAME, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
		SendDlgItemMessage(hwnd, IDC_INFOBAR_STATUS, EM_AUTOURLDETECT, (WPARAM) TRUE, 0);
		SendDlgItemMessage(hwnd, IDC_INFOBAR_STATUS, EM_SETEVENTMASK, 0, ENM_MOUSEEVENTS | ENM_LINK | ENM_KEYEVENTS);
		UpdateInfoBar(idat);
		return TRUE;

	case WM_SIZE:
		{
			if (wParam==SIZE_RESTORED || wParam==SIZE_MAXIMIZED) {
				HDWP hdwp;
				RECT rc;
				int dlgWidth, dlgHeight;
				int avatarWidth = 0;
				int avatarHeight = 0;
				GetClientRect(hwnd, &rc);
				dlgWidth = rc.right - rc.left;
				dlgHeight = rc.bottom - rc.top;
				if (idat->mwd->avatarPic && (g_dat->flags&SMF_AVATAR)) {
					BITMAP bminfo;
					GetObject(idat->mwd->avatarPic, sizeof(bminfo), &bminfo);
					if ( bminfo.bmWidth != 0 && bminfo.bmHeight != 0 ) {
						avatarHeight = dlgHeight - 2;
						avatarWidth = bminfo.bmWidth * avatarHeight / bminfo.bmHeight;
						if (avatarWidth > dlgHeight) {
							avatarWidth = dlgHeight - 2;
							avatarHeight = bminfo.bmHeight * avatarWidth / bminfo.bmWidth;
						}
					}
				}
				hdwp = BeginDeferWindowPos(3);
				
				hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_INFOBAR_NAME), 0, 15, 0, dlgWidth - avatarWidth - 2 - 15, dlgHeight/2, SWP_NOZORDER);
				hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_INFOBAR_STATUS), 0, 15, dlgHeight/2, dlgWidth - avatarWidth - 2 - 15, dlgHeight/2, SWP_NOZORDER);
				hdwp = DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_AVATAR), 0, dlgWidth - avatarWidth - 2, 1, avatarWidth, avatarHeight, SWP_NOZORDER);
				//MessageDialogResize(hwndDlg, dat, dlgWidth, dlgHeight);
				EndDeferWindowPos(hdwp);
			}
			return TRUE;
		}
	case WM_CTLCOLORDLG:
      //SetTextColor((HDC)wParam,RGB(60,60,150));
      //SetBkColor((HDC)wParam,GetSysColor(COLOR_WINDOW));
		return (INT_PTR)g_dat->hInfobarBrush;
	
	case WM_NOTIFY:
		{
			LPNMHDR pNmhdr = (LPNMHDR)lParam;
			switch (pNmhdr->idFrom) {
			case IDC_INFOBAR_NAME:
			case IDC_INFOBAR_STATUS:
				switch (pNmhdr->code) {
				case EN_LINK:
					switch (((ENLINK *) lParam)->msg) {
					case WM_RBUTTONDOWN:
					case WM_LBUTTONUP:
						if (HandleLinkClick(g_hInst, hwnd, GetDlgItem(GetParent(hwnd), IDC_MESSAGE),(ENLINK*)lParam)) {
							SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
							return TRUE;
						}
						break;
					}
				}
				break;
			}
			break;
		}
	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
			if (dis->hwndItem == GetDlgItem(hwnd, IDC_AVATAR)) {
				RECT rect;
				HDC hdcMem = CreateCompatibleDC(dis->hDC);
				int avatarWidth = 0;
				int avatarHeight = 0;
				int itemWidth = dis->rcItem.right - dis->rcItem.left + 1;
				int itemHeight = dis->rcItem.bottom - dis->rcItem.top + 1;
				HBITMAP hbmMem = CreateCompatibleBitmap(dis->hDC, itemWidth, itemHeight);
				hbmMem = (HBITMAP) SelectObject(hdcMem, hbmMem);
				rect.top = 0;
				rect.left = 0;
				rect.right = itemWidth - 1;
				rect.bottom = itemHeight - 1;
				FillRect(hdcMem, &rect, g_dat->hInfobarBrush);
				if (idat->mwd->avatarPic && (g_dat->flags&SMF_AVATAR)) {
					BITMAP bminfo;
					GetObject(idat->mwd->avatarPic, sizeof(bminfo), &bminfo);
					if ( bminfo.bmWidth != 0 && bminfo.bmHeight != 0 ) {
						AVATARDRAWREQUEST adr;
						avatarHeight = itemHeight;
						avatarWidth = bminfo.bmWidth * avatarHeight / bminfo.bmHeight;
						if (avatarWidth > itemWidth) {
							avatarWidth = itemWidth;
							avatarHeight = bminfo.bmHeight * avatarWidth / bminfo.bmWidth;
						}
						ZeroMemory(&adr, sizeof(adr));
						adr.cbSize = sizeof (AVATARDRAWREQUEST);
						adr.hContact = idat->mwd->windowData.hContact;
						adr.hTargetDC = hdcMem;
						adr.rcDraw.left = 0;
						adr.rcDraw.top = 0;
						adr.rcDraw.right = avatarWidth - 1;
						adr.rcDraw.bottom = avatarHeight - 1;
						adr.dwFlags = AVDRQ_DRAWBORDER | AVDRQ_HIDEBORDERONTRANSPARENCY;
						CallService(MS_AV_DRAWAVATAR, (WPARAM)0, (LPARAM)&adr);
					}
				}
				BitBlt(dis->hDC, 0, 0, itemWidth, itemHeight, hdcMem, 0, 0, SRCCOPY);
				hbmMem = (HBITMAP) SelectObject(hdcMem, hbmMem);
				DeleteObject(hbmMem);
				DeleteDC(hdcMem);
				return TRUE;
			}
			return CallService(MS_CLIST_MENUDRAWITEM, wParam, lParam);
		}

	}
	return FALSE;
}

InfobarWindowData *CreateInfoBar(HWND hParent)
{
	struct MessageWindowData *dat = (struct MessageWindowData *) GetWindowLongPtr(hParent, GWLP_USERDATA);
	InfobarWindowData *idat = (InfobarWindowData *) mir_alloc(sizeof(InfobarWindowData));
	idat->mwd = dat;
	idat->hWnd = CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_INFOBAR), hParent, InfobarWndProc, (LPARAM)idat);
	SetWindowPos(idat->hWnd, HWND_TOP, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOREPOSITION);
	return idat;
}
