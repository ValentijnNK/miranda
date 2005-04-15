/*
IRC plugin for Miranda IM

Copyright (C) 2003 J�rgen Persson

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

#include "irc.h"

HANDLE					OptionsInitHook = NULL;	
extern UINT_PTR			KeepAliveTimer;	
UINT_PTR				OnlineNotifTimer = 0;	


extern PREFERENCES *	prefs;
extern char *			IRCPROTONAME;
extern char *			ALTIRCPROTONAME;
extern char *			pszServerFile;
extern char *			pszIgnoreFile;
extern char *			pszPerformFile;
extern char				mirandapath[MAX_PATH];
HWND					connect_hWnd = NULL;
HWND					addserver_hWnd = NULL;
bool					ServerlistModified = false;
bool					PerformlistModified = false;
extern bool				bTempDisableCheck ;
extern bool				bTempForceCheck ;
extern int				iTempCheckTime ;
extern HMODULE			m_ssleay32;
extern HANDLE			hMenuServer;
static WNDPROC			OldProc;


static int GetPrefsString(const char *szSetting, char * prefstoset, int n, char * defaulttext)
{
	DBVARIANT dbv;
	int rv;
	rv = !DBGetContactSetting(NULL, IRCPROTONAME,szSetting, &dbv) && dbv.type==DBVT_ASCIIZ;
	if(rv)
		lstrcpyn(prefstoset, dbv.pszVal, n);
	else
		lstrcpyn(prefstoset, defaulttext, n);
	if (rv)
		DBFreeVariant(&dbv);
	return rv;
}

static int GetSetting(const char *szSetting, DBVARIANT *dbv)
{
	int rv;
	rv = !DBGetContactSetting(NULL, IRCPROTONAME,szSetting,dbv) && dbv->type==DBVT_ASCIIZ;
	return rv;
}
void InitPrefs(void)
{
	DBVARIANT dbv;

	prefs = new PREFERENCES;
	GetPrefsString("ServerName", prefs->ServerName, 101, "");
	GetPrefsString("PortStart", prefs->PortStart, 6, "");
	GetPrefsString("PortEnd", prefs->PortEnd, 6, "");
	GetPrefsString("Password", prefs->Password, 501, "");
	if(!GetPrefsString("PNick", prefs->Nick, 30, ""))
	{
		GetPrefsString("Nick", prefs->Nick, 30, "");
		if (lstrlen(prefs->Nick) > 0)
			DBWriteContactSettingString(NULL, IRCPROTONAME, "PNick", prefs->Nick);
	}
	GetPrefsString("AlernativeNick", prefs->AlternativeNick, 31, "");
	GetPrefsString("Name", prefs->Name, 199, "");
	GetPrefsString("UserID", prefs->UserID, 199, "Miranda");
	GetPrefsString("IdentSystem", prefs->IdentSystem, 10, "UNIX");
	GetPrefsString("IdentPort", prefs->IdentPort, 6, "113");
	GetPrefsString("RetryWait", prefs->RetryWait, 4, "30");
	GetPrefsString("RetryCount", prefs->RetryCount, 4, "10");
	GetPrefsString("Network", prefs->Network, 31, "");
	GetPrefsString("OnlineNotificationTime", prefs->OnlineNotificationTime, 10, "30");
	GetPrefsString("QuitMessage", prefs->QuitMessage, 399, STR_QUITMESSAGE);
	GetPrefsString("UserInfo", prefs->UserInfo, 499, STR_USERINFO);
	GetPrefsString("SpecHost", prefs->MySpecifiedHost, 499, "");
	GetPrefsString("MyLocalHost", prefs->MyLocalHost, 49, "");

	lstrcpy(prefs->MySpecifiedHostIP, "");

	if (GetSetting("Alias", &dbv)) {
		prefs->Alias = new char[lstrlen(dbv.pszVal)+1];
		lstrcpyn(prefs->Alias, dbv.pszVal, lstrlen(dbv.pszVal)+1);
		DBFreeVariant(&dbv);
	}
	else 
	{
		prefs->Alias = new char[350];
		lstrcpy(prefs->Alias, "/op /mode ## +ooo $1 $2 $3\r\n/dop /mode ## -ooo $1 $2 $3\r\n/voice /mode ## +vvv $1 $2 $3\r\n/dvoice /mode ## -vvv $1 $2 $3\r\n/j /join #$1 $2-\r\n/p /part ## $1-\r\n/w /whois $1\r\n/k /kick ## $1 $2-\r\n/q /query $1\r\n/logon /log on ##\r\n/logoff /log off ##\r\n/save /log buffer $1\r\n/slap /me slaps $1 around a bit with a large trout" );
//		DBFreeVariant(&dbv);
	}

	prefs->ForceVisible = DBGetContactSettingByte(NULL,IRCPROTONAME, "ForceVisible", 0);
	prefs->DisableErrorPopups = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisableErrorPopups", 0);
	prefs->RejoinChannels = DBGetContactSettingByte(NULL,IRCPROTONAME, "RejoinChannels", 0);
	prefs->RejoinIfKicked = DBGetContactSettingByte(NULL,IRCPROTONAME, "RejoinIfKicked", 1);
	prefs->Ident = DBGetContactSettingByte(NULL,IRCPROTONAME, "Ident", 0);
	prefs->IdentTimer = (int)DBGetContactSettingByte(NULL,IRCPROTONAME, "IdentTimer", 0);
	prefs->Retry = DBGetContactSettingByte(NULL,IRCPROTONAME, "Retry", 0);
	prefs->DisableDefaultServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisableDefaultServer", 0);
	prefs->HideServerWindow = DBGetContactSettingByte(NULL,IRCPROTONAME, "HideServerWindow", 1);
	prefs->UseServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "UseServer", 1);
	prefs->JoinOnInvite = DBGetContactSettingByte(NULL,IRCPROTONAME, "JoinOnInvite", 0);
	prefs->Perform = DBGetContactSettingByte(NULL,IRCPROTONAME, "Perform", 0);
	prefs->ShowAddresses = DBGetContactSettingByte(NULL,IRCPROTONAME, "ShowAddresses", 0);
	prefs->AutoOnlineNotification = DBGetContactSettingByte(NULL,IRCPROTONAME, "AutoOnlineNotification", 1);
	prefs->AutoOnlineNotifTempAlso = DBGetContactSettingByte(NULL,IRCPROTONAME, "AutoOnlineNotifTempAlso", 0);
	prefs->Ignore = 1;
	prefs->ServerComboSelection = DBGetContactSettingDword(NULL,IRCPROTONAME, "ServerComboSelection", -1);
	prefs->QuickComboSelection = DBGetContactSettingDword(NULL,IRCPROTONAME, "QuickComboSelection", prefs->ServerComboSelection);
	prefs->SendKeepAlive = (int)DBGetContactSettingByte(NULL,IRCPROTONAME, "SendKeepAlive", 0);
	prefs->ListSize.y = DBGetContactSettingDword(NULL,IRCPROTONAME, "SizeOfListBottom", 400);
	prefs->ListSize.x = DBGetContactSettingDword(NULL,IRCPROTONAME, "SizeOfListRight", 600);
	prefs->iSSL = DBGetContactSettingByte(NULL,IRCPROTONAME, "UseSSL", 0);
	prefs->DCCFileEnabled = DBGetContactSettingByte(NULL,IRCPROTONAME, "EnableCtcpFile", 1);
	prefs->DCCChatEnabled = DBGetContactSettingByte(NULL,IRCPROTONAME, "EnableCtcpChat", 1);
	prefs->DCCChatAccept = DBGetContactSettingByte(NULL,IRCPROTONAME, "CtcpChatAccept", 1);
	prefs->DCCChatIgnore = DBGetContactSettingByte(NULL,IRCPROTONAME, "CtcpChatIgnore", 1);
	prefs->DCCPassive = DBGetContactSettingByte(NULL,IRCPROTONAME, "DccPassive", 0);
	prefs->ManualHost = DBGetContactSettingByte(NULL,IRCPROTONAME, "ManualHost", 0);
	prefs->IPFromServer = DBGetContactSettingByte(NULL,IRCPROTONAME, "IPFromServer", 0);
	prefs->DisconnectDCCChats = DBGetContactSettingByte(NULL,IRCPROTONAME, "DisconnectDCCChats", 1);
	prefs->OldStyleModes = DBGetContactSettingByte(NULL,IRCPROTONAME, "OldStyleModes", 0);
	prefs->MyHost[0] = '\0';
	prefs->colors[0] = RGB(255,255,255);
	prefs->colors[1] = RGB(0,0,0);
	prefs->colors[2] = RGB(0,0,127);
	prefs->colors[3] = RGB(0,147,0);
	prefs->colors[4] = RGB(255,0,0);
	prefs->colors[5] = RGB(127,0,0);
	prefs->colors[6] = RGB(156,0,156);
	prefs->colors[7] = RGB(252,127,0);
	prefs->colors[8] = RGB(255,255,0);
	prefs->colors[9] = RGB(0,252,0);
	prefs->colors[10] = RGB(0,147,147);
	prefs->colors[11] = RGB(0,255,255);
	prefs->colors[12] = RGB(0,0,252);
	prefs->colors[13] = RGB(255,0,255);
	prefs->colors[14] = RGB(127,127,127); 
	prefs->colors[15] = RGB(210,210,210);

//	DBFreeVariant(&dbv);
	return;
}


// Callback for the 'Add server' dialog
BOOL CALLBACK AddServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			 TranslateDialogDefault(hwndDlg);
			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++)
			{
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}

			if(m_ssleay32)
				CheckDlgButton(hwndDlg, IDC_OFF, BST_CHECKED);
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO), FALSE);
			}

			SetWindowText(GetDlgItem(hwndDlg, IDC_ADD_PORT), "6667");
			SetWindowText(GetDlgItem(hwndDlg, IDC_ADD_PORT2), "6667");
			SetFocus(GetDlgItem(hwndDlg, IDC_ADD_COMBO));	
					
		}break;

		
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
				switch (LOWORD(wParam))
				{	
					case IDN_ADD_OK:
					{
						if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO)))
						{
							SERVER_INFO * pData = new SERVER_INFO;
							pData->iSSL = 0;
							if(IsDlgButtonChecked(hwndDlg, IDC_ON))
								pData->iSSL = 2;
							if(IsDlgButtonChecked(hwndDlg, IDC_AUTO))
								pData->iSSL = 1;
							pData->Address=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_ADDRESS, pData->Address, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1);
							pData->Group=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_COMBO, pData->Group, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1);
							pData->Name=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+3];
							lstrcpy(pData->Name, pData->Group);
							lstrcat(pData->Name, ": ");
							char temp[255];
							GetDlgItemText(hwndDlg,IDC_ADD_SERVER, temp, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+1);
							lstrcat(pData->Name, temp);			
							pData->PortEnd=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_PORT2, pData->PortEnd, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1);
							pData->PortStart=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_PORT, pData->PortStart, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1);
							int iItem = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
							SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
							SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
							SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);						
							PostMessage (hwndDlg, WM_CLOSE, 0,0);
							if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR)
							{
								int m = SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
								SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
							}
							ServerlistModified = true;
						}
						else
							MessageBox(hwndDlg, Translate(	"Please complete all fields"	), Translate(	"IRC error"	), MB_OK|MB_ICONERROR);
					}break;

					case IDN_ADD_CANCEL:
					{
						PostMessage (hwndDlg, WM_CLOSE, 0,0);
					}break;
					default:break;
				}

		}break;


		case WM_CLOSE:
		{
			EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
			DestroyWindow(hwndDlg);
			return(true);
		} break;
		
		case WM_DESTROY:
		{
			return (true);
		} break;
		default:break;
	}

	return false;
}

// Callback for the 'Edit server' dialog
BOOL CALLBACK EditServerProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
 			int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0);
			for (int index = 0; index <i; index++)
			{
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, index, 0);
				if (SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_FINDSTRINGEXACT, -1,(LPARAM) pData->Group) == CB_ERR)
					SendMessage(GetDlgItem(hwndDlg, IDC_ADD_COMBO), CB_ADDSTRING, 0, (LPARAM) pData->Group);
			}
			int j = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
			SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, j, 0);
			SetDlgItemText(hwndDlg, IDC_ADD_ADDRESS, pData->Address);
			SetDlgItemText(hwndDlg, IDC_ADD_COMBO, pData->Group);
			SetDlgItemText(hwndDlg, IDC_ADD_PORT, pData->PortStart);
			SetDlgItemText(hwndDlg, IDC_ADD_PORT2, pData->PortEnd);
			char tempchar[255];
			strcpy (tempchar, pData->Name);
			int temp = strchr(tempchar, ' ') -tempchar;
			for (int index2 = temp+1; index2 < lstrlen(tempchar)+1;index2++)
				tempchar[index2-temp-1] = tempchar[index2];
			if(m_ssleay32)
			{
				if(pData->iSSL == 0)
					CheckDlgButton(hwndDlg, IDC_OFF, BST_CHECKED);
				if(pData->iSSL == 1)
					CheckDlgButton(hwndDlg, IDC_AUTO, BST_CHECKED);
				if(pData->iSSL == 2)
					CheckDlgButton(hwndDlg, IDC_ON, BST_CHECKED);
			}
			else
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_ON), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_OFF), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_AUTO), FALSE);

			}
			

			SetDlgItemText(hwndDlg, IDC_ADD_SERVER, tempchar);
			SetFocus(GetDlgItem(hwndDlg, IDC_ADD_COMBO));	

		}break;

		
		case WM_COMMAND:
		{
			if (HIWORD(wParam) == BN_CLICKED)
				switch (LOWORD(wParam))
				{	
					case IDN_ADD_OK:
					{
						if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2)) && GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO)))
						{
							int i = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);

							SERVER_INFO * pData1 = (SERVER_INFO *)SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
							delete []pData1->Name;
							delete []pData1->Address;
							delete []pData1->PortStart;
							delete []pData1->PortEnd;
							delete []pData1->Group;
							delete pData1;	
							SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);
//							if (i >= SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0))
//								i--;
//							SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, i, 0);
//							SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
	
							
							SERVER_INFO * pData = new SERVER_INFO;
							pData->iSSL = 0;
							if(IsDlgButtonChecked(hwndDlg, IDC_ON))
								pData->iSSL = 2;
							if(IsDlgButtonChecked(hwndDlg, IDC_AUTO))
								pData->iSSL = 1;
							pData->Address=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_ADDRESS, pData->Address, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_ADDRESS))+1);
							pData->Group=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_COMBO, pData->Group, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+1);
							pData->Name=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_COMBO))+3];
							lstrcpy(pData->Name, pData->Group);
							lstrcat(pData->Name, ": ");
							char temp[255];
							GetDlgItemText(hwndDlg,IDC_ADD_SERVER, temp, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_SERVER))+1);
							lstrcat(pData->Name, temp);			
							pData->PortEnd=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_PORT2, pData->PortEnd, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT2))+1);
							pData->PortStart=new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1];
							GetDlgItemText(hwndDlg,IDC_ADD_PORT, pData->PortStart, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ADD_PORT))+1);
							int iItem = SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Name);
							SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETITEMDATA,iItem,(LPARAM) pData);
							SendMessage(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO),CB_SETCURSEL,iItem,0);
							SendMessage(connect_hWnd, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
							PostMessage (hwndDlg, WM_CLOSE, 0,0);
							if ( SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_FINDSTRINGEXACT,-1, (LPARAM)pData->Group) == CB_ERR)
							{
								int m = SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_ADDSTRING,0,(LPARAM) pData->Group);
								SendMessage(GetDlgItem(connect_hWnd, IDC_PERFORMCOMBO),CB_SETITEMDATA,m,0);
							}
							ServerlistModified = true;
						}
						else
							MessageBox(hwndDlg, Translate(	"Please complete all fields"	), Translate(	"IRC error"	), MB_OK|MB_ICONERROR);
					}break;

					case IDN_ADD_CANCEL:
					{
						PostMessage (hwndDlg, WM_CLOSE, 0,0);
					}break;
					default:break;
				}

		}break;
		case WM_CLOSE:
		{
			EnableWindow(GetDlgItem(connect_hWnd, IDC_SERVERCOMBO), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_ADDSERVER), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_EDITSERVER), true);
			EnableWindow(GetDlgItem(connect_hWnd, IDC_DELETESERVER), true);
			DestroyWindow(hwndDlg);
			return(true);
		} break;
		
		case WM_DESTROY:
		{
			return (true);
		} break;
		default:break;
	}

	return false;
}

static LRESULT CALLBACK EditSubclassProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam) 
{ 

	switch(msg) 
	{ 
		case WM_CHAR :
		{
			if (wParam == 21 || wParam == 11 || wParam == 2)
			{
				char w[2];
				w[1] = '\0';
				if (wParam == 11)
					w[0] = 3;
				if (wParam == 2)
					w[0] = 2;
				if (wParam == 21)
					w[0] = 31;
				SendMessage(hwndDlg, EM_REPLACESEL, false, (LPARAM) w);
				SendMessage(hwndDlg, EM_SCROLLCARET, 0,0);
				return 0;
			}

		} break;
		default: break;
	
	} 
	return CallWindowProc(OldProc, hwndDlg, msg, wParam, lParam); 

} 

// Callback for the 'CTCP preferences' dialog
BOOL CALLBACK CtcpPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			SetDlgItemText(hwndDlg,IDC_USERINFO,prefs->UserInfo);

			CheckDlgButton(hwndDlg, IDC_ENABLEXFER, prefs->DCCFileEnabled?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ENABLECHAT, prefs->DCCChatEnabled?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_SLOW, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==0?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_FAST, DBGetContactSettingByte(NULL, IRCPROTONAME, "DCCMode", 0)==1?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_DISC, prefs->DisconnectDCCChats?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_PASSIVE, prefs->DCCPassive?BST_CHECKED:BST_UNCHECKED);

			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "256");
			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "512");
			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "1024");
			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "2048");
			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "4096");
			SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_ADDSTRING, (WPARAM)0,(LPARAM) "8192");
			int j = DBGetContactSettingWord(NULL, IRCPROTONAME, "DCCPacketSize", 1024*4);
			char szTemp[10];
			_snprintf(szTemp, sizeof(szTemp), "%u", j);
			int i = SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) szTemp);
			if(i== CB_ERR)
				int i = SendDlgItemMessage(hwndDlg, IDC_COMBO, CB_SELECTSTRING, (WPARAM)-1,(LPARAM) "4096");


			if(prefs->DCCChatAccept == 1)
				CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);
			if(prefs->DCCChatAccept == 2)
				CheckDlgButton(hwndDlg, IDC_RADIO2, BST_CHECKED);
			if(prefs->DCCChatAccept == 3)
				CheckDlgButton(hwndDlg, IDC_RADIO3, BST_CHECKED);
			if(prefs->DCCChatIgnore == 1)
				CheckDlgButton(hwndDlg, IDC_RADIO4, BST_CHECKED);
			if(prefs->DCCChatIgnore == 2)
				CheckDlgButton(hwndDlg, IDC_RADIO5, BST_CHECKED);

			CheckDlgButton(hwndDlg, IDC_FROMSERVER, prefs->IPFromServer?BST_CHECKED:BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_ENABLEIP, prefs->ManualHost?BST_CHECKED:BST_UNCHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_IP), prefs->ManualHost);
			EnableWindow(GetDlgItem(hwndDlg, IDC_FROMSERVER), !prefs->ManualHost);
			if(prefs->ManualHost)
			{
				SetDlgItemText(hwndDlg,IDC_IP,prefs->MySpecifiedHost);
			}else
			{
				if(prefs->IPFromServer)
				{
					if(lstrlen(prefs->MyHost))
					{
						String s = (String)Translate("<Resolved IP: ") + (String)prefs->MyHost+ (String)">";
						SetDlgItemText(hwndDlg,IDC_IP,s.c_str());
					}
					else
						SetDlgItemText(hwndDlg,IDC_IP,Translate("<Automatic>"));
				}
				else
				{
					if(lstrlen(prefs->MyLocalHost))
					{
						String s = (String)Translate("<Local IP: ") + (String)prefs->MyLocalHost+ (String)">";
						SetDlgItemText(hwndDlg,IDC_IP,s.c_str());
					}
					else
						SetDlgItemText(hwndDlg,IDC_IP,Translate("<Automatic>"));

				}

			}

			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO1), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO2), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO3), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO4), prefs->DCCChatEnabled);
			EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO5), prefs->DCCChatEnabled);

		}break;

		case WM_COMMAND:
		{
			
			if((	LOWORD(wParam) == IDC_USERINFO	
				|| LOWORD(wParam) == IDC_IP)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;
				
				
			SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{	
					case ( IDC_ENABLECHAT):
					{

						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO1), IsDlgButtonChecked(hwndDlg, IDC_ENABLECHAT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO2), IsDlgButtonChecked(hwndDlg, IDC_ENABLECHAT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO3), IsDlgButtonChecked(hwndDlg, IDC_ENABLECHAT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO4), IsDlgButtonChecked(hwndDlg, IDC_ENABLECHAT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO5), IsDlgButtonChecked(hwndDlg, IDC_ENABLECHAT)== BST_CHECKED);
		
					}break;
					case ( IDC_FROMSERVER):
					case ( IDC_ENABLEIP):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_IP), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_FROMSERVER), IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_UNCHECKED);

						if(IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP)== BST_CHECKED)
						{
							SetDlgItemText(hwndDlg,IDC_IP,prefs->MySpecifiedHost);
						}else
						{
							if(IsDlgButtonChecked(hwndDlg, IDC_FROMSERVER)== BST_CHECKED)
							{
								if(lstrlen(prefs->MyHost))
								{
									String s = (String)Translate("<Resolved IP: ") + (String)prefs->MyHost+ (String)">";
									SetDlgItemText(hwndDlg,IDC_IP,s.c_str());
								}
								else
									SetDlgItemText(hwndDlg,IDC_IP,Translate("<Automatic>"));
							}
							else
							{
								if(lstrlen(prefs->MyLocalHost))
								{
									String s = (String)Translate("<Local IP: ") + (String)prefs->MyLocalHost+ (String)">";
									SetDlgItemText(hwndDlg,IDC_IP,s.c_str());
								}
								else
									SetDlgItemText(hwndDlg,IDC_IP,Translate("<Automatic>"));

							}

						}

					}break;
					default:break;
				}
			}
		}break;
		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom) 
			{
				case 0:
					switch (((LPNMHDR)lParam)->code) 
					{
						case PSN_APPLY:
						{
							
							GetDlgItemText(hwndDlg,IDC_USERINFO,prefs->UserInfo, 499);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"UserInfo",prefs->UserInfo);

							char szTemp[10];
							GetWindowText(GetDlgItem(hwndDlg, IDC_COMBO), szTemp, 10);
							DBWriteContactSettingWord(NULL,IRCPROTONAME,"DCCPacketSize", (WORD)atoi(szTemp));


							prefs->DCCFileEnabled = IsDlgButtonChecked(hwndDlg,IDC_ENABLEXFER)== BST_CHECKED?1:0;
							prefs->DCCChatEnabled = IsDlgButtonChecked(hwndDlg,IDC_ENABLECHAT)== BST_CHECKED?1:0;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"EnableCtcpFile",prefs->DCCFileEnabled);
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"EnableCtcpChat",prefs->DCCChatEnabled);

							prefs->DCCPassive = IsDlgButtonChecked(hwndDlg,IDC_PASSIVE)== BST_CHECKED?1:0;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"DccPassive",prefs->DCCPassive);
							
							if(IsDlgButtonChecked(hwndDlg,IDC_SLOW)== BST_CHECKED)
								DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",0);
							else 
								DBWriteContactSettingByte(NULL,IRCPROTONAME,"DCCMode",1);

							prefs->ManualHost = IsDlgButtonChecked(hwndDlg,IDC_ENABLEIP)== BST_CHECKED?1:0;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"ManualHost",prefs->ManualHost);

							prefs->IPFromServer = IsDlgButtonChecked(hwndDlg,IDC_FROMSERVER)== BST_CHECKED?1:0;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"IPFromServer",prefs->IPFromServer);
							
							prefs->DisconnectDCCChats = IsDlgButtonChecked(hwndDlg,IDC_DISC)== BST_CHECKED?1:0;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisconnectDCCChats",prefs->DisconnectDCCChats);

							if(IsDlgButtonChecked(hwndDlg, IDC_ENABLEIP) == BST_CHECKED)
							{
								char szTemp[500];
								GetDlgItemText(hwndDlg,IDC_IP,szTemp, 499);
								lstrcpyn(prefs->MySpecifiedHost, GetWord(szTemp, 0).c_str(), 499);
								DBWriteContactSettingString(NULL,IRCPROTONAME,"SpecHost",prefs->MySpecifiedHost);
								if(lstrlen(prefs->MySpecifiedHost))
								{
									IPRESOLVE * ipr = new IPRESOLVE;
									ipr->iType = IP_MANUAL;
									ipr->pszAdr = prefs->MySpecifiedHost;
									forkthread(ResolveIPThread, NULL, ipr);
								}

							}

							if(IsDlgButtonChecked(hwndDlg, IDC_RADIO1) == BST_CHECKED)
								prefs->DCCChatAccept = 1;
							if(IsDlgButtonChecked(hwndDlg, IDC_RADIO2) == BST_CHECKED)
								prefs->DCCChatAccept = 2;
							if(IsDlgButtonChecked(hwndDlg, IDC_RADIO3) == BST_CHECKED)
								prefs->DCCChatAccept = 3;
							if(IsDlgButtonChecked(hwndDlg, IDC_RADIO4) == BST_CHECKED)
								prefs->DCCChatIgnore = 1;
							if(IsDlgButtonChecked(hwndDlg, IDC_RADIO5) == BST_CHECKED)
								prefs->DCCChatIgnore = 2;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"CtcpChatAccept",prefs->DCCChatAccept);
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"CtcpChatIgnore",prefs->DCCChatIgnore);								
						}
						default:break;
						return TRUE;
					}
			}
		}break;

		default:break;
	}

	return false;
}

// Callback for the 'Advanced preferences' dialog
BOOL CALLBACK OtherPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);

			OldProc = (WNDPROC)SetWindowLong(GetDlgItem(hwndDlg, IDC_ALIASEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem(hwndDlg, IDC_QUITMESSAGE), GWL_WNDPROC,(LONG)EditSubclassProc); 
			SetWindowLong(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), GWL_WNDPROC,(LONG)EditSubclassProc); 

			SendDlgItemMessage(hwndDlg,IDC_ADD,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_ADD),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SendDlgItemMessage(hwndDlg,IDC_DELETE,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_DELETE),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SendMessage(GetDlgItem(hwndDlg,IDC_ADD), BUTTONADDTOOLTIP, (WPARAM)Translate("Click to set commands that will be performed for this event"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_DELETE), BUTTONADDTOOLTIP, (WPARAM)Translate("Click to delete the commands for this event"), 0);
		
			SetDlgItemText(hwndDlg,IDC_ALIASEDIT,prefs->Alias);
			SetDlgItemText(hwndDlg,IDC_QUITMESSAGE,prefs->QuitMessage);
			CheckDlgButton(hwndDlg,IDC_PERFORM, ((prefs->Perform) ? (BST_CHECKED) : (BST_UNCHECKED)));
			EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), prefs->Perform);
			EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), prefs->Perform);
			char * p1 = pszServerFile;
			char * p2 = pszServerFile;
			if (pszServerFile)
				while(strchr(p2, 'n'))
				{
					p1 = strstr(p2, "GROUP:");
					p1 =p1+ 6;
					p2 = strchr(p1, '\r');
					if (!p2)
						p2 = strchr(p1, '\n');
					if (!p2)
						p2 = strchr(p1, '\0');

					char * Group = new char[p2-p1+1];
					lstrcpyn(Group, p1, p2-p1+1);
					int i = SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)Group);
					if (i ==CB_ERR) {
						int index = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_ADDSTRING, 0, (LPARAM) Group);
//						SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_SETITEMDATA, index, 0);
					}
					delete []Group;

				}

			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 0, (LPARAM)"Event: Available"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 1, (LPARAM)"Event: Away"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 2, (LPARAM)"Event: N/A"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 3, (LPARAM)"Event: Occupied"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 4, (LPARAM)"Event: DND"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 5, (LPARAM)"Event: Free for chat"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 6, (LPARAM)"Event: On the phone"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 7, (LPARAM)"Event: Out for lunch"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 8, (LPARAM)"Event: Disconnect"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_INSERTSTRING, 9, (LPARAM)"ALL NETWORKS"	);
			SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_SETITEMDATA, -1, 0);
			p1 = pszPerformFile;
			p2 = pszPerformFile;
			if(pszPerformFile)
			while(strstr(p2, "NETWORK: ")) {
				p1 = strstr(p2, "NETWORK: ");
				p1 = p1+9;
				p2 = strchr(p1, '\n');
				char * szNetwork = new char[p2-p1];
				lstrcpyn(szNetwork, p1, p2-p1);
				p1 = p2;
				p1++;
				p2 = strstr(p1, "\nNETWORK: ");
				if (!p2)
					p2= p1 + lstrlen(p1)-1;
				if(p1 != p2) {
					int index = SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_FINDSTRINGEXACT, -1, (LPARAM) szNetwork);
					if (index != CB_ERR) {
						PERFORM_INFO * pPref = new PERFORM_INFO;
						pPref->Perform = new char[p2-p1];
						lstrcpyn(pPref->Perform, p1, p2-p1);
						SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_SETITEMDATA, index, (LPARAM)pPref);
					}
				}
				else
					break;
				delete [] szNetwork;
			}
		SendDlgItemMessage(hwndDlg, IDC_PERFORMCOMBO, CB_SETCURSEL, 0, 0);				
		SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_PERFORMCOMBO, CBN_SELCHANGE), 0);
		PerformlistModified = false;
	
		}break;

		case WM_COMMAND:
		{
			
			if((	LOWORD(wParam) == IDC_ALIASEDIT
					|| LOWORD(wParam) == IDC_PERFORMEDIT
					|| LOWORD(wParam) == IDC_QUITMESSAGE
					|| LOWORD(wParam) == IDC_PERFORMCOMBO && HIWORD(wParam) != CBN_SELCHANGE
				)
				&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;
				
				
			if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PERFORMCOMBO)
			{

				int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
				PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
				if (pPerf == 0)
					SetDlgItemText(hwndDlg, IDC_PERFORMEDIT, "");
				else
					SetDlgItemText(hwndDlg, IDC_PERFORMEDIT, pPerf->Perform);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);
				if ( GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT)) != 0)
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), true);
				}
				else
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
				}
				return false;
			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);

			if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == IDC_PERFORMEDIT)
			{
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), true);

				if ( GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT)) != 0)
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), true);
				}
				else
				{
					EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
				}
			}

			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{	
					case ( IDC_PERFORM):
					{

						EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), IsDlgButtonChecked(hwndDlg, IDC_PERFORM)== BST_CHECKED);
		
					}break;


					case ( IDC_ADD):
					{

						int j = GetWindowTextLength(GetDlgItem(hwndDlg, IDC_PERFORMEDIT));
						char * temp = new char[j+1];
						GetWindowText(GetDlgItem(hwndDlg, IDC_PERFORMEDIT), temp, j+1);

						if(my_strstri(temp, "/away"))
							MessageBox(NULL, "The usage of /AWAY in your perform buffer is restricted\n as IRC sends this command automatically.", "IRC Error", MB_OK);
						else
						{
							int i = (int) SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
							PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
							if (pPerf != 0)
							{
								delete []pPerf->Perform;
								delete pPerf;
							}
							pPerf = new PERFORM_INFO;
							pPerf->Perform = new char[j+1];
							lstrcpy(pPerf->Perform, temp);
							SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_SETITEMDATA, i, (LPARAM) pPerf);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);

							PerformlistModified = true;
						}
						delete []temp;

						
					}break;
					case ( IDC_DELETE):
					{
						int i = (int) SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCURSEL, 0, 0);
						PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, i, 0);
						if (pPerf != 0)
						{
							delete []pPerf->Perform;
							delete pPerf;
						}
						SetDlgItemText(hwndDlg, IDC_PERFORMEDIT, "");
						SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_SETITEMDATA, i, 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), false);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), false);

							
						PerformlistModified = true;
						
					}break;
					default:break;
				}
			}
		}break;
		case WM_DESTROY:
		{
			PerformlistModified = false;
			int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
			if (i != CB_ERR && i !=0)
			{
				for (int index = 0; index < i; index++)
				{
					PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
					if ( (const int)pPerf != CB_ERR && pPerf != 0)
					{
						delete []pPerf->Perform;
						delete pPerf;
					}

				}
			}
		
		} break;
		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom) 
			{
				case 0:
					switch (((LPNMHDR)lParam)->code) 
					{
						case PSN_APPLY:
						{
							
							delete [] prefs->Alias;
							prefs->Alias = new char[GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ALIASEDIT))+1];
							GetDlgItemText(hwndDlg,IDC_ALIASEDIT,prefs->Alias, GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ALIASEDIT))+1);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"Alias",prefs->Alias);

							GetDlgItemText(hwndDlg,IDC_QUITMESSAGE,prefs->QuitMessage, 399);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"QuitMessage",prefs->QuitMessage);

							prefs->Perform = IsDlgButtonChecked(hwndDlg,IDC_PERFORM)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"Perform",prefs->Perform);
							if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_ADD)))
								SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_ADD, BN_CLICKED), 0);

							if (PerformlistModified) {
								PerformlistModified = false;
								char filepath[MAX_PATH];
								_snprintf(filepath, sizeof(filepath), "%s\\%s_perform.ini", mirandapath, IRCPROTONAME);
								int i = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETCOUNT, 0, 0);
								FILE *hFile = fopen(filepath,"wb");
								if (hFile && i != CB_ERR && i !=0)
								{
									for (int index = 0; index < i; index++)
									{
										PERFORM_INFO * pPerf = (PERFORM_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETITEMDATA, index, 0);
										if ( (const int)pPerf != CB_ERR && pPerf != 0)
										{
											int k = SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETLBTEXTLEN, index, 0);
											char * temp = new char[k+1];
											SendMessage(GetDlgItem(hwndDlg, IDC_PERFORMCOMBO), CB_GETLBTEXT, index, (LPARAM)temp);
											fputs("NETWORK: ", hFile);
											fputs(temp, hFile);
											fputs("\r\n", hFile);
											fputs(pPerf->Perform, hFile);
											fputs("\r\n", hFile);
											delete []temp;

										}

									}
								fclose(hFile);
								delete [] pszPerformFile;
								pszPerformFile = IrcLoadFile(filepath);

								}
							}
								
						}
							return TRUE;
					}
			}
		}break;

		default:break;
	}

	return false;
}

// Callback for the 'Connect preferences' dialog
BOOL CALLBACK ConnectPrefsProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{


	switch (uMsg)
	{
	case WM_CHAR:
		{
			SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);
		}break;
		case WM_INITDIALOG:
		{

			TranslateDialogDefault(hwndDlg);

			SendDlgItemMessage(hwndDlg,IDC_ADDSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_ADD),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SendDlgItemMessage(hwndDlg,IDC_DELETESERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_DELETE),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SendDlgItemMessage(hwndDlg,IDC_EDITSERVER,BM_SETIMAGE,IMAGE_ICON,(LPARAM)(HICON)LoadImage(g_hInstance,MAKEINTRESOURCE(IDI_RENAME),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SendMessage(GetDlgItem(hwndDlg,IDC_ADDSERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Add a new network"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_EDITSERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Edit this network"), 0);
			SendMessage(GetDlgItem(hwndDlg,IDC_DELETESERVER), BUTTONADDTOOLTIP, (WPARAM)Translate("Delete this network"), 0);

			
			connect_hWnd = hwndDlg;
			
			//	Fill the servers combo box and create SERVER_INFO structures
			char * p1 = pszServerFile;
			char * p2 = pszServerFile;
			if(pszServerFile) 
				while (strchr(p2, 'n'))
				{
					SERVER_INFO * pData = new SERVER_INFO;
					p1 = strchr(p2, '=');
					++p1;
					p2 = strstr(p1, "SERVER:");
					pData->Name=new char[p2-p1+1];
					lstrcpyn(pData->Name, p1, p2-p1+1);
					
					p1 = strchr(p2, ':');
					++p1;
					pData->iSSL = 0;
					if(strstr(p1, "SSL") == p1)
					{
						p1 +=3;
						if(*p1 == '1')
							pData->iSSL = 1;
						else if(*p1 == '2')
							pData->iSSL = 2;
						p1++;
					}
					p2 = strchr(p1, ':');
					pData->Address=new char[p2-p1+1];
					lstrcpyn(pData->Address, p1, p2-p1+1);
					
					p1 = p2;
					p1++;
					while (*p2 !='G' && *p2 != '-')
						p2++;
					pData->PortStart = new char[p2-p1+1];
					lstrcpyn(pData->PortStart, p1, p2-p1+1);

					if (*p2 == 'G'){
						pData->PortEnd = new char[p2-p1+1];
						lstrcpy(pData->PortEnd, pData->PortStart);
					} else {
						p1 = p2;
						p1++;
						p2 = strchr(p1, 'G');
						pData->PortEnd = new char[p2-p1+1];
						lstrcpyn(pData->PortEnd, p1, p2-p1+1);
					}

					p1 = strchr(p2, ':');
					p1++;
					p2 = strchr(p1, '\r');
					if (!p2)
						p2 = strchr(p1, '\n');
					if (!p2)
						p2 = strchr(p1, '\0');
					pData->Group = new char[p2-p1+1];
					lstrcpyn(pData->Group, p1, p2-p1+1);
					int iItem = SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_ADDSTRING,0,(LPARAM) pData->Name);
					SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_SETITEMDATA, iItem,(LPARAM) pData);

				}
				
				SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_SETCURSEL, prefs->ServerComboSelection,0);				
				SetDlgItemText(hwndDlg,IDC_SERVER, prefs->ServerName);
				SetDlgItemText(hwndDlg,IDC_PORT, prefs->PortStart);
				SetDlgItemText(hwndDlg,IDC_PORT2, prefs->PortEnd);
				if(m_ssleay32)
				{
					if(prefs->iSSL == 0)
						SetDlgItemText(hwndDlg,IDC_SSL,Translate("Off") );
					if(prefs->iSSL == 1)
						SetDlgItemText(hwndDlg,IDC_SSL,Translate("Auto") );
					if(prefs->iSSL == 2)
						SetDlgItemText(hwndDlg,IDC_SSL,Translate("On") );
				}
				else
					SetDlgItemText(hwndDlg,IDC_SSL, Translate("N/A"));


				
				if (prefs->ServerComboSelection != -1)
				{
					SERVER_INFO * pData = (SERVER_INFO *)SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETITEMDATA, prefs->ServerComboSelection, 0);
					if ((int)pData != CB_ERR)
					{
						SetDlgItemText(hwndDlg,IDC_SERVER,pData->Address);
						SetDlgItemText(hwndDlg,IDC_PORT,pData->PortStart);
						SetDlgItemText(hwndDlg,IDC_PORT2,pData->PortEnd);
					}
				}

				SetDlgItemText(hwndDlg,IDC_NICK,prefs->Nick);
				SetDlgItemText(hwndDlg,IDC_ONLINETIMER,prefs->OnlineNotificationTime);
				SetDlgItemText(hwndDlg,IDC_NICK2,prefs->AlternativeNick);
				SetDlgItemText(hwndDlg,IDC_USERID,prefs->UserID);
				SetDlgItemText(hwndDlg,IDC_NAME,prefs->Name);
				SetDlgItemText(hwndDlg,IDC_PASS,prefs->Password);
				SetDlgItemText(hwndDlg,IDC_IDENTSYSTEM,prefs->IdentSystem);
				SetDlgItemText(hwndDlg,IDC_IDENTPORT,prefs->IdentPort);
				SetDlgItemText(hwndDlg,IDC_RETRYWAIT,prefs->RetryWait);
				SetDlgItemText(hwndDlg,IDC_RETRYCOUNT,prefs->RetryCount);			
				CheckDlgButton(hwndDlg,IDC_ADDRESS, ((prefs->ShowAddresses) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_OLDSTYLE, ((prefs->OldStyleModes) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_ONLINENOTIF, ((prefs->AutoOnlineNotification) ? (BST_CHECKED) : (BST_UNCHECKED)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_ONLINETIMER), prefs->AutoOnlineNotification);
				EnableWindow(GetDlgItem(hwndDlg, IDC_NOTTEMP), prefs->AutoOnlineNotification);
				CheckDlgButton(hwndDlg,IDC_NOTTEMP, ((prefs->AutoOnlineNotifTempAlso) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_IDENT, ((prefs->Ident) ? (BST_CHECKED) : (BST_UNCHECKED)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTSYSTEM), prefs->Ident);
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTPORT), prefs->Ident);
				CheckDlgButton(hwndDlg,IDC_DISABLEERROR, ((prefs->DisableErrorPopups) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_FORCEVISIBLE, ((prefs->ForceVisible) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_REJOINCHANNELS, ((prefs->RejoinChannels) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_REJOINONKICK, ((prefs->RejoinIfKicked) ? (BST_CHECKED) : (BST_UNCHECKED)));
				CheckDlgButton(hwndDlg,IDC_RETRY, ((prefs->Retry) ? (BST_CHECKED) : (BST_UNCHECKED)));
				EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYWAIT), prefs->Retry);
				EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYCOUNT), prefs->Retry);
				CheckDlgButton(hwndDlg,IDC_STARTUP, ((!prefs->DisableDefaultServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				CheckDlgButton(hwndDlg,IDC_KEEPALIVE, ((prefs->SendKeepAlive) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				CheckDlgButton(hwndDlg,IDC_IDENT_TIMED, ((prefs->IdentTimer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				CheckDlgButton(hwndDlg,IDC_USESERVER, ((prefs->UseServer) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				CheckDlgButton(hwndDlg,IDC_SHOWSERVER, ((!prefs->HideServerWindow) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				CheckDlgButton(hwndDlg,IDC_AUTOJOIN, ((prefs->JoinOnInvite) ? (BST_CHECKED) : (BST_UNCHECKED)));				
				EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSERVER), prefs->UseServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_SERVER), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PORT2), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_PASS), !prefs->DisableDefaultServer);
				EnableWindow(GetDlgItem(hwndDlg, IDC_IDENT_TIMED), prefs->Ident);
				ServerlistModified = false;
		} break;

		case WM_COMMAND:
		{
			if(	(LOWORD(wParam)		  == IDC_SERVER
					|| LOWORD(wParam) == IDC_PORT
					|| LOWORD(wParam) == IDC_PORT2
					|| LOWORD(wParam) == IDC_PASS
					|| LOWORD(wParam) == IDC_NICK
					|| LOWORD(wParam) == IDC_NICK2
					|| LOWORD(wParam) == IDC_NAME
					|| LOWORD(wParam) == IDC_USERID
					|| LOWORD(wParam) == IDC_IDENTSYSTEM
					|| LOWORD(wParam) == IDC_IDENTPORT
					|| LOWORD(wParam) == IDC_ONLINETIMER
					|| LOWORD(wParam) == IDC_RETRYWAIT
					|| LOWORD(wParam) == IDC_RETRYCOUNT
					|| LOWORD(wParam) == IDC_SSL
					|| LOWORD(wParam) == IDC_SERVERCOMBO && HIWORD(wParam) != CBN_SELCHANGE
					)
			&& (HIWORD(wParam)!=EN_CHANGE || (HWND)lParam!=GetFocus()))	return true;
			if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_SERVERCOMBO)
			{


				int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
				SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
				if (pData && (int)pData !=CB_ERR)
				{
					SetDlgItemText(hwndDlg,IDC_SERVER,pData->Address);
					SetDlgItemText(hwndDlg,IDC_PORT,pData->PortStart);
					SetDlgItemText(hwndDlg,IDC_PORT2,pData->PortEnd);
					SetDlgItemText(hwndDlg,IDC_PASS,"");
					if(m_ssleay32)
					{
						if(pData->iSSL == 0)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("Off") );
						if(pData->iSSL == 1)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("Auto") );
						if(pData->iSSL == 2)
							SetDlgItemText(hwndDlg,IDC_SSL,Translate("On") );
					}
					else
						SetDlgItemText(hwndDlg,IDC_SSL, Translate("N/A"));
					SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);
					}
				return false;

			}
			SendMessage(GetParent(hwndDlg), PSM_CHANGED,0,0);
			
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case ( IDC_DELETESERVER):
					{
						int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
						if (i != CB_ERR)
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
							SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
							char * temp = new char [lstrlen(pData->Name)+24];
							wsprintf(temp,Translate(	"Do you want to delete\r\n%s"	), pData->Name);
							if (MessageBox(hwndDlg, temp, Translate(	"Delete server"	), MB_YESNO|MB_ICONQUESTION) == IDYES)
							{
								delete []pData->Name;
								delete []pData->Address;
								delete []pData->PortStart;
								delete []pData->PortEnd;
								delete []pData->Group;
								delete pData;	
								SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_DELETESTRING, i, 0);
								if (i >= SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCOUNT, 0, 0))
									i--;
								SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_SETCURSEL, i, 0);
								SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_SERVERCOMBO,CBN_SELCHANGE), 0);
								ServerlistModified = true;
							}
							delete []temp;
							EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), true);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), true);
							EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), true);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), true);
						}
					
					}break;

					case ( IDC_ADDSERVER):
					{

							EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
							addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,AddServerProc);

					}break;
					case ( IDC_EDITSERVER):
					{
						int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
						if (i != CB_ERR)
						{
							EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), false);
							EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), false);
							addserver_hWnd = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_ADDSERVER),hwndDlg,EditServerProc);
							SetWindowText(addserver_hWnd, Translate("Edit server"));
						}

					}break;
					case ( IDC_STARTUP):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ADDSERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDITSERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_DELETESERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SERVER), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_PORT), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_PORT2), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_PASS), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_SSL), IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED);
		
					}break;	
					
					case ( IDC_IDENT):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTSYSTEM), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_IDENTPORT), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_IDENT_TIMED), IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED);
					}break;
					case ( IDC_USESERVER):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_SHOWSERVER), IsDlgButtonChecked(hwndDlg, IDC_USESERVER)== BST_CHECKED);
					}break;
					case ( IDC_ONLINENOTIF):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_NOTTEMP), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_ONLINETIMER), IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF)== BST_CHECKED);
					}break;

					case ( IDC_RETRY):
					{
						EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYWAIT), IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED);
						EnableWindow(GetDlgItem(hwndDlg, IDC_RETRYCOUNT), IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED);
					}break;

					default:break;
				}
			}
		} break;

		case WM_NOTIFY:
		{
			switch(((LPNMHDR)lParam)->idFrom) 
			{
				case 0:
					switch (((LPNMHDR)lParam)->code) 
					{
						case PSN_APPLY:
						{
							//Save the setting in the CONNECT dialog
							if(IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED)
							{
								GetDlgItemText(hwndDlg,IDC_SERVER, prefs->ServerName, 101);
								DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
								GetDlgItemText(hwndDlg,IDC_PORT, prefs->PortStart, 6);
								DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
								GetDlgItemText(hwndDlg,IDC_PORT2, prefs->PortEnd, 6);
								DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
								GetDlgItemText(hwndDlg,IDC_PASS, prefs->Password, 500);
								DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
							}
							else
							{
								lstrcpy(prefs->ServerName, "");
								DBWriteContactSettingString(NULL,IRCPROTONAME,"ServerName",prefs->ServerName);
								lstrcpy(prefs->PortStart, "");
								DBWriteContactSettingString(NULL,IRCPROTONAME,"PortStart",prefs->PortStart);
								lstrcpy(prefs->PortEnd, "");
								DBWriteContactSettingString(NULL,IRCPROTONAME,"PortEnd",prefs->PortEnd);
								lstrcpy( prefs->Password, "");
								DBWriteContactSettingString(NULL,IRCPROTONAME,"Password",prefs->Password);
							}
							GetDlgItemText(hwndDlg,IDC_ONLINETIMER, prefs->OnlineNotificationTime, 5);
							if (lstrlen(prefs->OnlineNotificationTime) == 0 ) 
								lstrcpy(prefs->OnlineNotificationTime, "30");
							if (StrToInt(prefs->OnlineNotificationTime) <10 ) 
								lstrcpy(prefs->OnlineNotificationTime, "10");
							SetDlgItemText(hwndDlg, IDC_ONLINETIMER, prefs->OnlineNotificationTime);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"OnlineNotificationTime",prefs->OnlineNotificationTime);
							GetDlgItemText(hwndDlg,IDC_NICK, prefs->Nick, 30);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"PNick",prefs->Nick);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"Nick",prefs->Nick);
							GetDlgItemText(hwndDlg,IDC_NICK2, prefs->AlternativeNick, 30);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"AlernativeNick",prefs->AlternativeNick);
							GetDlgItemText(hwndDlg,IDC_USERID, prefs->UserID, 199);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"UserID",prefs->UserID);
							GetDlgItemText(hwndDlg,IDC_NAME, prefs->Name, 199);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"Name",prefs->Name);
							GetDlgItemText(hwndDlg,IDC_IDENTSYSTEM, prefs->IdentSystem, 10);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"IdentSystem",prefs->IdentSystem);
							GetDlgItemText(hwndDlg,IDC_IDENTPORT, prefs->IdentPort, 6);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"IdentPort",prefs->IdentPort);
							GetDlgItemText(hwndDlg,IDC_RETRYWAIT, prefs->RetryWait, 4);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"RetryWait",prefs->RetryWait);
							GetDlgItemText(hwndDlg,IDC_RETRYCOUNT, prefs->RetryCount, 4);
							DBWriteContactSettingString(NULL,IRCPROTONAME,"RetryCount",prefs->RetryCount);
							prefs->DisableDefaultServer = !IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableDefaultServer",prefs->DisableDefaultServer);
							prefs->Ident = IsDlgButtonChecked(hwndDlg, IDC_IDENT)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"Ident",prefs->Ident);
							prefs->IdentTimer = IsDlgButtonChecked(hwndDlg, IDC_IDENT_TIMED)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"IdentTimer",prefs->IdentTimer);
							prefs->ForceVisible = IsDlgButtonChecked(hwndDlg, IDC_FORCEVISIBLE)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"ForceVisible",prefs->ForceVisible);
							prefs->DisableErrorPopups = IsDlgButtonChecked(hwndDlg, IDC_DISABLEERROR)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"DisableErrorPopups",prefs->DisableErrorPopups);
							prefs->RejoinChannels = IsDlgButtonChecked(hwndDlg, IDC_REJOINCHANNELS)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinChannels",prefs->RejoinChannels);
							prefs->RejoinIfKicked = IsDlgButtonChecked(hwndDlg, IDC_REJOINONKICK)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"RejoinIfKicked",prefs->RejoinIfKicked);
							prefs->Retry = IsDlgButtonChecked(hwndDlg, IDC_RETRY)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"Retry",prefs->Retry);
							prefs->ShowAddresses = IsDlgButtonChecked(hwndDlg, IDC_ADDRESS)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"ShowAddresses",prefs->ShowAddresses);
							prefs->OldStyleModes = IsDlgButtonChecked(hwndDlg, IDC_OLDSTYLE)== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"OldStyleModes",prefs->OldStyleModes);

							prefs->UseServer = IsDlgButtonChecked(hwndDlg, IDC_USESERVER )== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseServer",prefs->UseServer);

							CLISTMENUITEM clmi;
							memset( &clmi, 0, sizeof( clmi ));
							clmi.cbSize = sizeof( clmi );
							if(prefs->UseServer)
							{
								clmi.flags = CMIM_FLAGS;
								CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServer, ( LPARAM )&clmi );
							}
							else
							{
								clmi.flags = CMIM_FLAGS|CMIF_GRAYED;
								CallService( MS_CLIST_MODIFYMENUITEM, ( WPARAM )hMenuServer, ( LPARAM )&clmi );
							}

							prefs->JoinOnInvite = IsDlgButtonChecked(hwndDlg, IDC_AUTOJOIN )== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"JoinOnInvite",prefs->JoinOnInvite);
							prefs->HideServerWindow = IsDlgButtonChecked(hwndDlg, IDC_SHOWSERVER )== BST_UNCHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"HideServerWindow",prefs->HideServerWindow);
							prefs->ServerComboSelection = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
							DBWriteContactSettingDword(NULL,IRCPROTONAME,"ServerComboSelection",prefs->ServerComboSelection);
							prefs->SendKeepAlive = IsDlgButtonChecked(hwndDlg, IDC_KEEPALIVE )== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"SendKeepAlive",prefs->SendKeepAlive);
							if (prefs->SendKeepAlive)
								SetChatTimer(KeepAliveTimer, 60*1000, KeepAliveTimerProc);
							else
								KillChatTimer(KeepAliveTimer);

							prefs->AutoOnlineNotification = IsDlgButtonChecked(hwndDlg, IDC_ONLINENOTIF )== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"AutoOnlineNotification",prefs->AutoOnlineNotification);
							if (prefs->AutoOnlineNotification)
							{
								if(!bTempDisableCheck)
									SetChatTimer(OnlineNotifTimer, 1000, OnlineNotifTimerProc);
							}
							else
							{
								if (!bTempForceCheck)
									KillChatTimer(OnlineNotifTimer);
							}
							
							prefs->AutoOnlineNotifTempAlso = IsDlgButtonChecked(hwndDlg, IDC_NOTTEMP )== BST_CHECKED;
							DBWriteContactSettingByte(NULL,IRCPROTONAME,"AutoOnlineNotifTempAlso",prefs->AutoOnlineNotifTempAlso);

							int i = SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETCURSEL, 0, 0);
							SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, i, 0);
							if (pData && (int)pData !=CB_ERR)
							{
								if(IsDlgButtonChecked(hwndDlg, IDC_STARTUP)== BST_CHECKED)
									lstrcpy(prefs->Network, pData->Group); 
								else
									lstrcpy(prefs->Network, ""); 
								DBWriteContactSettingString(NULL,IRCPROTONAME,"Network",prefs->Network);
								prefs->iSSL = pData->iSSL;
								DBWriteContactSettingByte(NULL,IRCPROTONAME,"UseSSL",pData->iSSL);			
							}

							if (ServerlistModified) {
								ServerlistModified = false;
								char filepath[MAX_PATH];
								_snprintf(filepath, sizeof(filepath), "%s\\%s_servers.ini", mirandapath, IRCPROTONAME);
								FILE *hFile2 = fopen(filepath,"w");
								if (hFile2)
								{
									int j = (int) SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
									if (j !=CB_ERR && j !=0){
										for (int index2 = 0; index2 < j; index2++)
										{
											SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
											if (pData != NULL && (int)pData != CB_ERR)
											{
												char TextLine[512];
												if(pData->iSSL > 0)
													_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:SSL%u%s:%s-%sGROUP:%s\n", index2, pData->Name, pData->iSSL, pData->Address, pData->PortStart, pData->PortEnd, pData->Group);
												else
													_snprintf(TextLine, sizeof(TextLine), "n%u=%sSERVER:%s:%s-%sGROUP:%s\n", index2, pData->Name, pData->Address, pData->PortStart, pData->PortEnd, pData->Group);
												fputs(TextLine, hFile2);

											}

										}
									}
									fclose(hFile2);	
									delete [] pszServerFile;
									pszServerFile = IrcLoadFile(filepath);
									
								}
							}

					
						}
						return TRUE;
					}
			}
		}break;
		case WM_DESTROY:
		{
			ServerlistModified = false;
			int j = (int) SendDlgItemMessage(hwndDlg, IDC_SERVERCOMBO, CB_GETCOUNT, 0, 0);
			if (j !=CB_ERR && j !=0){
				for (int index2 = 0; index2 < j; index2++)
				{
					SERVER_INFO * pData = (SERVER_INFO *)SendMessage(GetDlgItem(hwndDlg, IDC_SERVERCOMBO), CB_GETITEMDATA, index2, 0);
					if (pData != NULL && (int)pData != CB_ERR)
					{
						delete []pData->Name;
						delete []pData->Address;
						delete []pData->PortStart;
						delete []pData->PortEnd;
						delete []pData->Group;
						delete pData;	
					}

				}
			}
				
		} break;
	
		default:break;
	}
return (false);
}
int InitOptionsPages(WPARAM wParam,LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };

	odp.cbSize = sizeof(odp);
	odp.hInstance = g_hInstance;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_PREFS_CONNECT);
	odp.pszTitle = ALTIRCPROTONAME;
	odp.pszGroup = Translate("Network");
	odp.flags=ODPF_BOLDGROUPS;
	odp.pfnDlgProc = ConnectPrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	char Temp[256];
	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize = sizeof(odp);
	odp.hInstance = g_hInstance;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_PREFS_CTCP);
	wsprintf(Temp, Translate("%s DCC 'n CTCP"), ALTIRCPROTONAME);
	odp.pszTitle = Temp;
	odp.pszGroup = Translate("Network");
	odp.flags=ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pfnDlgProc = CtcpPrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	ZeroMemory(&odp,sizeof(odp));
	odp.cbSize = sizeof(odp);
	odp.hInstance = g_hInstance;
	odp.pszTemplate = MAKEINTRESOURCE(IDD_PREFS_OTHER);
	wsprintf(Temp, Translate("%s Advanced"), ALTIRCPROTONAME);
	odp.pszTitle = Temp;
	odp.pszGroup = Translate("Network");
	odp.flags=ODPF_BOLDGROUPS | ODPF_EXPERTONLY;
	odp.pfnDlgProc = OtherPrefsProc;
	CallService(MS_OPT_ADDPAGE,wParam,(LPARAM)&odp);

	return 0;
}

void UnInitOptions(void)
{
	delete [] prefs->Alias;
	delete prefs;
	delete []pszServerFile;
	delete []pszPerformFile;
	delete []pszIgnoreFile;

}