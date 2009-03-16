/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2008 Miranda ICQ/IM project, 
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

#if defined( _UNICODE )
	#define STR_VERSION_FORMAT "%s%S%S"
#else
	#define STR_VERSION_FORMAT "%s%s%s"
#endif

INT_PTR CALLBACK DlgProcAbout(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static int iState = 0;
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{	TCHAR filename[MAX_PATH], *productCopyright;
			DWORD unused;
			DWORD verInfoSize;
			UINT blockSize;
			PVOID pVerInfo;

			GetModuleFileName(NULL,filename,SIZEOF(filename));
			verInfoSize=GetFileVersionInfoSize(filename,&unused);
			pVerInfo=mir_alloc(verInfoSize);
			GetFileVersionInfo(filename,0,verInfoSize,pVerInfo);
			VerQueryValue(pVerInfo,_T("\\StringFileInfo\\000004b0\\LegalCopyright"),(LPVOID*)&productCopyright,&blockSize);
			SetDlgItemText(hwndDlg,IDC_DEVS,productCopyright);
			mir_free(pVerInfo);
		}
		{	char productVersion[56], *p;
            int isAnsi = 0;
			TCHAR str[64];
			CallService(MS_SYSTEM_GETVERSIONTEXT,SIZEOF(productVersion),(LPARAM)productVersion);
            // Hide Unicode from version text as it is assumed at this point
            p = strstr(productVersion, " Unicode"); 
			if (p)
				*p = '\0';
            else
                isAnsi = 1;
			mir_sntprintf(str,SIZEOF(str),_T(STR_VERSION_FORMAT), TranslateT("v"), productVersion, isAnsi?" ANSI":"");
            {
                TCHAR oldTitle[256], newTitle[256];
				GetWindowText( GetDlgItem(hwndDlg, IDC_HEADERBAR), oldTitle, SIZEOF( oldTitle ));
				mir_sntprintf( newTitle, SIZEOF(newTitle), oldTitle, str );
				SetWindowText( GetDlgItem(hwndDlg, IDC_HEADERBAR), newTitle );
                SendMessage(GetDlgItem(hwndDlg, IDC_HEADERBAR), WM_SETICON, 0, (WPARAM)LoadIcon(hMirandaInst, MAKEINTRESOURCE(IDI_MIRANDA)));
			}
            
			mir_sntprintf(str,SIZEOF(str),TranslateT("Built %s %s"),_T(__DATE__),_T(__TIME__));
			SetDlgItemText(hwndDlg,IDC_BUILDTIME,str);
		}
		ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
		{	char* pszMsg = ( char* )LockResource(LoadResource(hMirandaInst,FindResource(hMirandaInst,MAKEINTRESOURCE(IDR_CREDITS),_T("TEXT"))));
			#if defined( _UNICODE )
				TCHAR* ptszMsg = ( TCHAR* )alloca(2000*sizeof(TCHAR));
            MultiByteToWideChar(1252,0,pszMsg,-1,ptszMsg,2000);
            SetDlgItemText(hwndDlg,IDC_CREDITSFILE, ptszMsg);
			#else
            SetDlgItemText(hwndDlg,IDC_CREDITSFILE, pszMsg);
			#endif
		}
		Window_SetIcon_IcoLib(hwndDlg, SKINICON_OTHER_MIRANDA);
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD( wParam )) {
		case IDOK:
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			return TRUE;
		case IDC_CONTRIBLINK:
			if (iState) {
				iState = 0;
				SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("Credits >"));
				ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_SHOW);
				ShowWindow(GetDlgItem(hwndDlg, IDC_BUILDTIME), SW_SHOW);
				ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_HIDE);
			}
			else {
				iState = 1;
				SetDlgItemText(hwndDlg, IDC_CONTRIBLINK, TranslateT("< Copyright"));
				ShowWindow(GetDlgItem(hwndDlg, IDC_DEVS), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_BUILDTIME), SW_HIDE);
				ShowWindow(GetDlgItem(hwndDlg, IDC_CREDITSFILE), SW_SHOW);
			}
			break;
		}
		break;

	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		switch ( GetWindowLong(( HWND )lParam, GWL_ID )) {
		case IDC_WHITERECT:
		case IDC_BUILDTIME:
		case IDC_LOGO:
		case IDC_CREDITSFILE:
		case IDC_DEVS:
			SetTextColor((HDC)wParam,GetSysColor(COLOR_WINDOWTEXT));
			break;
		default:
			return FALSE;
      }
		SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
		return (BOOL)GetSysColorBrush(COLOR_WINDOW);

	case WM_DESTROY:
		Window_FreeIcon_IcoLib( hwndDlg );
		{	
			HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_MIRANDA,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				

			hFont=(HFONT)SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_GETFONT,0,0);
			SendDlgItemMessage(hwndDlg,IDC_VERSION,WM_SETFONT,SendDlgItemMessage(hwndDlg,IDOK,WM_GETFONT,0,0),0);
			DeleteObject(hFont);				
		}
		break;
	}
	return FALSE;
}
