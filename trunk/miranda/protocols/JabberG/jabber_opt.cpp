/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005-06  George Hazan

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or ( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

File name      : $Source: /cvsroot/miranda/miranda/protocols/JabberG/jabber_opt.cpp,v $
Revision       : $Revision$
Last change on : $Date$
Last change by : $Author$

*/

#include "jabber.h"
#include "jabber_list.h"
#include <commctrl.h>
#include "resource.h"

extern BOOL jabberSendKeepAlive;
extern UINT jabberCodePage;

/////////////////////////////////////////////////////////////////////////////////////////
// JabberRegisterDlgProc - the dialog proc for registering new account

#if defined( _UNICODE )
	#define STR_FORMAT _T("%s %s@%S:%d?")
#else
	#define STR_FORMAT _T("%s %s@%s:%d?")
#endif

static BOOL CALLBACK JabberRegisterDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	struct ThreadData *thread, *regInfo;

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );
		regInfo = ( struct ThreadData * ) lParam;
		TCHAR text[256];
		mir_sntprintf( text, SIZEOF(text), STR_FORMAT, TranslateT( "Register" ), regInfo->username, regInfo->server, regInfo->port );
		SetDlgItemText( hwndDlg, IDC_REG_STATUS, text );
		SetWindowLong( hwndDlg, GWL_USERDATA, ( LONG )regInfo );
		return TRUE;
	}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDOK:
			ShowWindow( GetDlgItem( hwndDlg, IDOK ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), SW_SHOW );
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_SHOW );
			regInfo = ( struct ThreadData * ) GetWindowLong( hwndDlg, GWL_USERDATA );
			thread = ( struct ThreadData * ) mir_alloc( sizeof( struct ThreadData ));
			thread->type = JABBER_SESSION_REGISTER;
			_tcsncpy( thread->username, regInfo->username, SIZEOF( thread->username ));
			strncpy( thread->password, regInfo->password, SIZEOF( thread->password ));
			strncpy( thread->server, regInfo->server, SIZEOF( thread->server ));
			strncpy( thread->manualHost, regInfo->manualHost, SIZEOF( thread->manualHost ));
			thread->port = regInfo->port;
			thread->useSSL = regInfo->useSSL;
			thread->reg_hwndDlg = hwndDlg;
			JabberForkThread(( JABBER_THREAD_FUNC )JabberServerThread, 0, thread );
			return TRUE;
		case IDCANCEL:
		case IDOK2:
			EndDialog( hwndDlg, 0 );
			return TRUE;
		}
		break;
	case WM_JABBER_REGDLG_UPDATE:	// wParam=progress ( 0-100 ), lparam=status string
		if (( TCHAR* )lParam == NULL )
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, TranslateT( "No message" ));
		else
			SetDlgItemText( hwndDlg, IDC_REG_STATUS, ( TCHAR* )lParam );
		if ( wParam >= 0 )
			SendMessage( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ), PBM_SETPOS, wParam, 0 );
		if ( wParam >= 100 ) {
			ShowWindow( GetDlgItem( hwndDlg, IDCANCEL2 ), SW_HIDE );
			ShowWindow( GetDlgItem( hwndDlg, IDOK2 ), SW_SHOW );
		}
		else SetFocus( GetDlgItem( hwndDlg, IDC_PROGRESS_REG ));
		return TRUE;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptDlgProc - main options dialog procedure

static HWND msgLangListBox;
static BOOL CALLBACK JabberMsgLangAdd( LPSTR str )
{
	int i, count, index;
	UINT cp;
	static struct { UINT cpId; TCHAR* cpName; } cpTable[] = {
		{	874,	_T("Thai") },
		{	932,	_T("Japanese") },
		{	936,	_T("Simplified Chinese") },
		{	949,	_T("Korean") },
		{	950,	_T("Traditional Chinese") },
		{	1250,	_T("Central European") },
		{	1251,	_T("Cyrillic") },
		{	1252,	_T("Latin I") },
		{	1253,	_T("Greek") },
		{	1254,	_T("Turkish") },
		{	1255,	_T("Hebrew") },
		{	1256,	_T("Arabic") },
		{	1257,	_T("Baltic") },
		{	1258,	_T("Vietnamese") },
		{	1361,	_T("Korean ( Johab )") }
	};

	cp = atoi( str );
	count = sizeof( cpTable )/sizeof( cpTable[0] );
	for ( i=0; i<count && cpTable[i].cpId!=cp; i++ );
	if ( i < count ) {
		if (( index=SendMessage( msgLangListBox, CB_ADDSTRING, 0, ( LPARAM )TranslateTS( cpTable[i].cpName )) ) >= 0 ) {
			SendMessage( msgLangListBox, CB_SETITEMDATA, ( WPARAM ) index, ( LPARAM )cp );
			if ( jabberCodePage == cp )
				SendMessage( msgLangListBox, CB_SETCURSEL, ( WPARAM ) index, 0 );
	}	}

	return TRUE;
}

static LRESULT CALLBACK JabberValidateUsernameWndProc( HWND hwndEdit, UINT msg, WPARAM wParam, LPARAM lParam )
{
	WNDPROC oldProc = ( WNDPROC ) GetWindowLong( hwndEdit, GWL_USERDATA );

	switch ( msg ) {
	case WM_CHAR:
		if ( strchr( "\"&'/:<>@", wParam&0xff ) != NULL )
			return 0;
		break;
	}
	return CallWindowProc( oldProc, hwndEdit, msg, wParam, lParam );
}

static BOOL CALLBACK JabberOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg ) {
	case WM_INITDIALOG:
		{
			DBVARIANT dbv;
			BOOL enableRegister = TRUE;

			TranslateDialogDefault( hwndDlg );
			SetDlgItemTextA( hwndDlg, IDC_SIMPLE, jabberModuleName );
			if ( !DBGetContactSetting( NULL, jabberProtoName, "LoginName", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_EDIT_USERNAME, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "Password", &dbv )) {
				JCallService( MS_DB_CRYPT_DECODESTRING, strlen( dbv.pszVal )+1, ( LPARAM )dbv.pszVal );
				SetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "Resource", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_EDIT_RESOURCE, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_EDIT_RESOURCE, "Miranda" );

			SendMessage( GetDlgItem( hwndDlg, IDC_PRIORITY_SPIN ), UDM_SETRANGE, 0, ( LPARAM )MAKELONG( 100, 0 ));

			char text[256];
			sprintf( text, "%d", JGetWord( NULL, "Priority", 0 ));
			SetDlgItemTextA( hwndDlg, IDC_PRIORITY, text );
			CheckDlgButton( hwndDlg, IDC_SAVEPASSWORD, JGetByte( "SavePassword", TRUE ));
			if ( !DBGetContactSetting( NULL, jabberProtoName, "LoginServer", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, dbv.pszVal );
				if ( !dbv.pszVal[0] ) enableRegister = FALSE;
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, "jabber.org" );

			WORD port = ( WORD )JGetWord( NULL, "Port", JABBER_DEFAULT_PORT );
			SetDlgItemInt( hwndDlg, IDC_PORT, port, FALSE );
			if ( port <= 0 ) enableRegister = FALSE;

			CheckDlgButton( hwndDlg, IDC_USE_SSL, JGetByte( "UseSSL", FALSE ));
			CheckDlgButton( hwndDlg, IDC_USE_TLS, JGetByte( "UseTLS", FALSE ));
			EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), !JGetByte( "UseSSL", FALSE ));

			EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), enableRegister );

			if ( JGetByte( "ManualConnect", FALSE ) == TRUE ) {
				CheckDlgButton( hwndDlg, IDC_MANUAL, TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), TRUE );
				EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), FALSE );
			}
			if ( !DBGetContactSetting( NULL, jabberProtoName, "ManualHost", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_HOST, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			SetDlgItemInt( hwndDlg, IDC_HOSTPORT, JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT ), FALSE );

			CheckDlgButton( hwndDlg, IDC_KEEPALIVE, JGetByte( "KeepAlive", TRUE ));
			CheckDlgButton( hwndDlg, IDC_ROSTER_SYNC, JGetByte( "RosterSync", FALSE ));

			if ( !DBGetContactSetting( NULL, jabberProtoName, "Jud", &dbv )) {
				SetDlgItemTextA( hwndDlg, IDC_JUD, dbv.pszVal );
				JFreeVariant( &dbv );
			}
			else SetDlgItemTextA( hwndDlg, IDC_JUD, "users.jabber.org" );

			msgLangListBox = GetDlgItem( hwndDlg, IDC_MSGLANG );
			TCHAR str[ 256 ];
			mir_sntprintf( str, SIZEOF(str), _T("== %s =="), TranslateT( "System default" ));
			SendMessage( msgLangListBox, CB_ADDSTRING, 0, ( LPARAM )str );
			SendMessage( msgLangListBox, CB_SETITEMDATA, 0, CP_ACP );
			SendMessage( msgLangListBox, CB_SETCURSEL, 0, 0 );
			EnumSystemCodePagesA( JabberMsgLangAdd, CP_INSTALLED );

			WNDPROC oldProc = ( WNDPROC ) GetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_WNDPROC );
			SetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_USERDATA, ( LONG ) oldProc );
			SetWindowLong( GetDlgItem( hwndDlg, IDC_EDIT_USERNAME ), GWL_WNDPROC, ( LONG ) JabberValidateUsernameWndProc );

			return TRUE;
		}
	case WM_COMMAND:
		switch ( LOWORD( wParam )) {
		case IDC_EDIT_USERNAME:
		case IDC_EDIT_PASSWORD:
		case IDC_EDIT_RESOURCE:
		case IDC_EDIT_LOGIN_SERVER:
		case IDC_PORT:
		case IDC_MANUAL:
		case IDC_HOST:
		case IDC_HOSTPORT:
		case IDC_JUD:
		case IDC_PRIORITY:
			if ( LOWORD( wParam ) == IDC_MANUAL ) {
				if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), TRUE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), TRUE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), FALSE );
				}
				else {
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOST ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_HOSTPORT ), FALSE );
					EnableWindow( GetDlgItem( hwndDlg, IDC_PORT ), TRUE );
				}
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}
			else {
				if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
					SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			}

			ThreadData regInfo;
			GetDlgItemText( hwndDlg, IDC_EDIT_USERNAME, regInfo.username, SIZEOF( regInfo.username ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, SIZEOF( regInfo.password ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, SIZEOF( regInfo.server ));
			if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
				GetDlgItemTextA( hwndDlg, IDC_HOST, regInfo.manualHost, SIZEOF( regInfo.manualHost ));
			}
			else {
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
				regInfo.manualHost[0] = '\0';
			}
			if ( regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL ) || regInfo.manualHost[0] ))
				EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), TRUE );
			else
				EnableWindow( GetDlgItem( hwndDlg, IDC_BUTTON_REGISTER ), FALSE );
			break;
		case IDC_LINK_PUBLIC_SERVER:
			ShellExecuteA( hwndDlg, "open", "http://www.jabber.org/network", "", "", SW_SHOW );
			return TRUE;
		case IDC_BUTTON_REGISTER:
			GetDlgItemText( hwndDlg, IDC_EDIT_USERNAME, regInfo.username, SIZEOF( regInfo.username ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, regInfo.password, SIZEOF( regInfo.password ));
			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, regInfo.server, SIZEOF( regInfo.server ));
			if ( IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				GetDlgItemTextA( hwndDlg, IDC_HOST, regInfo.manualHost, SIZEOF( regInfo.manualHost ));
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
			}
			else {
				regInfo.manualHost[0] = '\0';
				regInfo.port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
			}
			regInfo.useSSL = IsDlgButtonChecked( hwndDlg, IDC_USE_SSL );

			if ( regInfo.username[0] && regInfo.password[0] && regInfo.server[0] && regInfo.port>0 && ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL ) || regInfo.manualHost[0] ))
				DialogBoxParam( hInst, MAKEINTRESOURCE( IDD_OPT_REGISTER ), hwndDlg, JabberRegisterDlgProc, ( LPARAM )&regInfo );

			return TRUE;
		case IDC_MSGLANG:
			if ( HIWORD( wParam ) == CBN_SELCHANGE )
				SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		case IDC_USE_SSL:
			if ( !IsDlgButtonChecked( hwndDlg, IDC_MANUAL )) {
				if ( IsDlgButtonChecked( hwndDlg, IDC_USE_SSL )) {
					EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), FALSE );
					SetDlgItemInt( hwndDlg, IDC_PORT, 5223, FALSE );
				}
				else {
					EnableWindow(GetDlgItem( hwndDlg, IDC_USE_TLS ), TRUE );
					SetDlgItemInt( hwndDlg, IDC_PORT, 5222, FALSE );
			}	}
			// Fall through
		case IDC_SAVEPASSWORD:
		case IDC_KEEPALIVE:
		case IDC_ROSTER_SYNC:
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		default:
			return 0;
			break;
		}
		break;
	case WM_NOTIFY:
		if (( ( LPNMHDR ) lParam )->code == PSN_APPLY ) {
			BOOL reconnectRequired = FALSE;
			DBVARIANT dbv;

			char text[256];
			GetDlgItemTextA( hwndDlg, IDC_EDIT_USERNAME, text, sizeof( text ));
			if ( DBGetContactSetting( NULL, jabberProtoName, "LoginName", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "LoginName", text );

			if ( IsDlgButtonChecked( hwndDlg, IDC_SAVEPASSWORD )) {
				GetDlgItemTextA( hwndDlg, IDC_EDIT_PASSWORD, text, sizeof( text ));
				JCallService( MS_DB_CRYPT_ENCODESTRING, sizeof( text ), ( LPARAM )text );
				if ( DBGetContactSetting( NULL, jabberProtoName, "Password", &dbv ) || strcmp( text, dbv.pszVal ))
					reconnectRequired = TRUE;
				if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
				JSetString( NULL, "Password", text );
			}
			else JDeleteSetting( NULL, "Password" );

			GetDlgItemTextA( hwndDlg, IDC_EDIT_RESOURCE, text, sizeof( text ));
			if ( DBGetContactSetting( NULL, jabberProtoName, "Resource", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "Resource", text );

			GetDlgItemTextA( hwndDlg, IDC_PRIORITY, text, sizeof( text ));
			WORD port = ( WORD )atoi( text );
			if ( port > 100 ) port = 100;
			if ( port < 0 ) port = 0;
			if ( JGetWord( NULL, "Priority", 0 ) != port )
				reconnectRequired = TRUE;
			JSetWord( NULL, "Priority", ( WORD )port );

			JSetByte( "SavePassword", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_SAVEPASSWORD ));

			GetDlgItemTextA( hwndDlg, IDC_EDIT_LOGIN_SERVER, text, sizeof( text ));
			if ( DBGetContactSetting( NULL, jabberProtoName, "LoginServer", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "LoginServer", text );

			port = ( WORD )GetDlgItemInt( hwndDlg, IDC_PORT, NULL, FALSE );
			if ( JGetWord( NULL, "Port", JABBER_DEFAULT_PORT ) != port )
				reconnectRequired = TRUE;
			JSetWord( NULL, "Port", port );

			JSetByte( "UseSSL", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_USE_SSL ));
			JSetByte( "UseTLS", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_USE_TLS ));

			JSetByte( "ManualConnect", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_MANUAL ));

			GetDlgItemTextA( hwndDlg, IDC_HOST, text, sizeof( text ));
			if ( DBGetContactSetting( NULL, jabberProtoName, "ManualHost", &dbv ) || strcmp( text, dbv.pszVal ))
				reconnectRequired = TRUE;
			if ( dbv.pszVal != NULL )	JFreeVariant( &dbv );
			JSetString( NULL, "ManualHost", text );

			port = ( WORD )GetDlgItemInt( hwndDlg, IDC_HOSTPORT, NULL, FALSE );
			if ( JGetWord( NULL, "ManualPort", JABBER_DEFAULT_PORT ) != port )
				reconnectRequired = TRUE;
			JSetWord( NULL, "ManualPort", port );

			JSetByte( "KeepAlive", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE ));
			jabberSendKeepAlive = IsDlgButtonChecked( hwndDlg, IDC_KEEPALIVE );

			JSetByte( "RosterSync", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_ROSTER_SYNC ));

			GetDlgItemTextA( hwndDlg, IDC_JUD, text, sizeof( text ));
			JSetString( NULL, "Jud", text );

			int index = SendMessage( GetDlgItem( hwndDlg, IDC_MSGLANG ), CB_GETCURSEL, 0, 0 );
			if ( index >= 0 ) {
				jabberCodePage = SendMessage( GetDlgItem( hwndDlg, IDC_MSGLANG ), CB_GETITEMDATA, ( WPARAM ) index, 0 );
				JSetWord( NULL, "CodePage", ( WORD )jabberCodePage );
			}

			if ( reconnectRequired && jabberConnected )
				MessageBox( hwndDlg, TranslateT( "These changes will take effect the next time you connect to the Jabber network." ), TranslateT( "Jabber Protocol Option" ), MB_OK|MB_SETFOREGROUND );

			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberAdvOptDlgProc - advanced options dialog procedure

static BOOL CALLBACK JabberAdvOptDlgProc( HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	char text[256];
	BOOL bChecked;

	switch ( msg ) {
	case WM_INITDIALOG:
	{
		TranslateDialogDefault( hwndDlg );

		// File transfer options
		BOOL bDirect = JGetByte( "BsDirect", TRUE );
		BOOL bManualDirect = JGetByte( "BsDirectManual", FALSE );
		CheckDlgButton( hwndDlg, IDC_DIRECT, bDirect );
		CheckDlgButton( hwndDlg, IDC_DIRECT_MANUAL, bManualDirect );

		DBVARIANT dbv;
		if ( !DBGetContactSetting( NULL, jabberProtoName, "BsDirectAddr", &dbv )) {
			SetDlgItemTextA( hwndDlg, IDC_DIRECT_ADDR, dbv.pszVal );
			JFreeVariant( &dbv );
		}
		if ( !bDirect )
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_MANUAL ), FALSE );
		if ( !bDirect || !bManualDirect )
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), FALSE );

		BOOL bProxy = JGetByte( "BsProxy", FALSE );
		BOOL bManualProxy = JGetByte( "BsProxyManual", FALSE );
		CheckDlgButton( hwndDlg, IDC_PROXY, bProxy );
		CheckDlgButton( hwndDlg, IDC_PROXY_MANUAL, bManualProxy );
		if ( !DBGetContactSetting( NULL, jabberProtoName, "BsProxyServer", &dbv )) {
			SetDlgItemTextA( hwndDlg, IDC_PROXY_ADDR, dbv.pszVal );
			JFreeVariant( &dbv );
		}
		if ( !bProxy )
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_MANUAL ), FALSE );
		if ( !bProxy || !bManualProxy )
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_ADDR ), FALSE );

		// Miscellaneous options
		CheckDlgButton( hwndDlg, IDC_SHOW_TRANSPORT, JGetByte( "ShowTransport", TRUE ));
		CheckDlgButton( hwndDlg, IDC_AUTO_ADD, JGetByte( "AutoAdd", TRUE ));
		CheckDlgButton( hwndDlg, IDC_MSG_ACK, JGetByte( "MsgAck", FALSE ));
		CheckDlgButton( hwndDlg, IDC_DISABLE_MAINMENU, JGetByte( "DisableMainMenu", FALSE ));
		CheckDlgButton( hwndDlg, IDC_ENABLE_AVATARS, JGetByte( "EnableAvatars", TRUE ));
		CheckDlgButton( hwndDlg, IDC_AUTO_ACCEPT_MUC, JGetByte( "AutoAcceptMUC", FALSE ));
		CheckDlgButton( hwndDlg, IDC_AUTOJOIN, JGetByte( "AutoJoinConferences", FALSE ));
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch ( LOWORD( wParam )) {
		case IDC_DIRECT_ADDR:
		case IDC_PROXY_ADDR:
			if (( HWND )lParam==GetFocus() && HIWORD( wParam )==EN_CHANGE )
				goto LBL_Apply;
			break;
		case IDC_DIRECT:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_DIRECT );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_MANUAL ), bChecked );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), ( bChecked && IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL )) );
			goto LBL_Apply;
		case IDC_DIRECT_MANUAL:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL );
			EnableWindow( GetDlgItem( hwndDlg, IDC_DIRECT_ADDR ), bChecked );
			goto LBL_Apply;
		case IDC_PROXY:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_PROXY );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_MANUAL ), bChecked );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_ADDR ), ( bChecked && IsDlgButtonChecked( hwndDlg, IDC_PROXY_MANUAL )) );
			goto LBL_Apply;
		case IDC_PROXY_MANUAL:
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_PROXY_MANUAL );
			EnableWindow( GetDlgItem( hwndDlg, IDC_PROXY_ADDR ), bChecked );
		default:
		LBL_Apply:
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		}
		break;
	}
	case WM_NOTIFY:
		if (( ( LPNMHDR ) lParam )->code == PSN_APPLY ) {
			// File transfer options
			JSetByte( "BsDirect", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_DIRECT ));
			JSetByte( "BsDirectManual", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_DIRECT_MANUAL ));
			GetDlgItemTextA( hwndDlg, IDC_DIRECT_ADDR, text, sizeof( text ));
			JSetString( NULL, "BsDirectAddr", text );
			JSetByte( "BsProxy", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_PROXY ));
			JSetByte( "BsProxyManual", ( BYTE ) IsDlgButtonChecked( hwndDlg, IDC_PROXY_MANUAL ));
			GetDlgItemTextA( hwndDlg, IDC_PROXY_ADDR, text, sizeof( text ));
			JSetString( NULL, "BsProxyAddr", text );

			// Miscellaneous options
			bChecked = IsDlgButtonChecked( hwndDlg, IDC_SHOW_TRANSPORT );
			JSetByte( "ShowTransport", ( BYTE ) bChecked );
			int index = 0;
			while (( index=JabberListFindNext( LIST_ROSTER, index )) >= 0 ) {
				JABBER_LIST_ITEM* item = JabberListGetItemPtrFromIndex( index );
				if ( item != NULL ) {
					if ( _tcschr( item->jid, '@' ) == NULL ) {
						HANDLE hContact = JabberHContactFromJID( item->jid );
						if ( hContact != NULL ) {
							if ( bChecked ) {
								if ( item->status != JGetWord( hContact, "Status", ID_STATUS_OFFLINE )) {
									JSetWord( hContact, "Status", ( WORD )item->status );
							}	}
							else if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
								JSetWord( hContact, "Status", ID_STATUS_OFFLINE );
				}	}	}
				index++;
			}

			JSetByte( "AutoAdd",             ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AUTO_ADD ));
			JSetByte( "MsgAck",              ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_MSG_ACK ));
			JSetByte( "DisableMainMenu",     ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_DISABLE_MAINMENU ));
			JSetByte( "EnableAvatars",       ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_ENABLE_AVATARS ));
			JSetByte( "AutoAcceptMUC",       ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AUTO_ACCEPT_MUC ));
			JSetByte( "AutoJoinConferences", ( BYTE )IsDlgButtonChecked( hwndDlg, IDC_AUTOJOIN ));
			return TRUE;
		}
		break;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// JabberOptInit - initializes all options dialogs

static HWND hwndAcc = 0, hwndAdv = 0; 

static void SetOptionsDlgToType(HWND hwnd, int iExpert)
{
	TCITEM tci;
	RECT rcClient;
	HWND hwndTab = GetDlgItem(hwnd, IDC_OPTIONSTAB), hwndEnum;
	int iPages = 0;

	if(!hwndAcc)
		hwndAcc = CreateDialog(hInst,MAKEINTRESOURCE(IDD_OPT_JABBER), hwnd, JabberOptDlgProc);

	hwndEnum = GetWindow(hwndAcc, GW_CHILD);
	
	while(hwndEnum) {
		ShowWindow(hwndEnum, iExpert ? SW_SHOW : SW_HIDE);
		hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
	}
	if(!iExpert) {
		hwndEnum = GetDlgItem(hwndAcc, IDC_SIMPLE);
		ShowWindow(hwndEnum, SW_SHOW);
		hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
		do {
			ShowWindow(hwndEnum, SW_SHOW);
			hwndEnum = GetWindow(hwndEnum, GW_HWNDNEXT);
		} while(hwndEnum && hwndEnum != GetDlgItem(hwndAcc, IDC_LINK_PUBLIC_SERVER));
	}
	ShowWindow(hwndEnum, SW_SHOW);
	GetClientRect(hwnd, &rcClient);
	TabCtrl_DeleteAllItems(hwndTab);

	tci.mask = TCIF_PARAM|TCIF_TEXT;
	tci.lParam = (LPARAM)hwndAcc;
	tci.pszText = TranslateT("Account");
	TabCtrl_InsertItem(hwndTab, 0, &tci);
	MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
	iPages++;

	if(!hwndAdv)
		hwndAdv = CreateDialog(hInst,MAKEINTRESOURCE(IDD_OPT_JABBER2),hwnd,JabberAdvOptDlgProc);

	ShowWindow(hwndAdv, SW_HIDE);
	ShowWindow(hwndAcc, SW_SHOW);

	if(iExpert) {
		tci.lParam = (LPARAM)hwndAdv;
		tci.pszText = TranslateT("Advanced");
		TabCtrl_InsertItem(hwndTab, iPages++, &tci);
		MoveWindow((HWND)tci.lParam,5,26,rcClient.right-8,rcClient.bottom-29,1);
	}
	TabCtrl_SetCurSel(hwndTab, 0);
}

static BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   static int iInit = TRUE;
   
   switch(msg) {
   case WM_INITDIALOG:
   {
		iInit = TRUE;
		int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
		SetOptionsDlgToType(hwnd, iExpert);
      iInit = FALSE;
      return FALSE;
   }
	case WM_DESTROY:
		hwndAcc = hwndAdv = 0;
		break;
   case PSM_CHANGED: // used so tabs dont have to call SendMessage(GetParent(GetParent(hwnd)), PSM_CHANGED, 0, 0);
      if(!iInit)
			SendMessage(GetParent(hwnd), PSM_CHANGED, 0, 0);
      break;
   case WM_NOTIFY:
      switch(((LPNMHDR)lParam)->idFrom) {
      case 0:
         switch (((LPNMHDR)lParam)->code) {
         case PSN_APPLY:
            {
               TCITEM tci;
               int i,count = TabCtrl_GetItemCount(GetDlgItem(hwnd,IDC_OPTIONSTAB));
               tci.mask = TCIF_PARAM;
               for (i=0;i<count;i++) {
                  TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),i,&tci);
                  SendMessage((HWND)tci.lParam,WM_NOTIFY,0,lParam);
               }
		         break;
            }
			case PSN_EXPERTCHANGED:
				{
					int iExpert = SendMessage(GetParent(hwnd), PSM_ISEXPERT, 0, 0);
					SetOptionsDlgToType(hwnd, iExpert);
					break;
			}	}
	      break;
      case IDC_OPTIONSTAB:
         switch (((LPNMHDR)lParam)->code) {
         case TCN_SELCHANGING:
            {
               TCITEM tci;
               tci.mask = TCIF_PARAM;
               TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
               ShowWindow((HWND)tci.lParam,SW_HIDE);                     
            }
				break;
         case TCN_SELCHANGE:
            {
               TCITEM tci;
               tci.mask = TCIF_PARAM;
               TabCtrl_GetItem(GetDlgItem(hwnd,IDC_OPTIONSTAB),TabCtrl_GetCurSel(GetDlgItem(hwnd,IDC_OPTIONSTAB)),&tci);
               ShowWindow((HWND)tci.lParam,SW_SHOW);                     
			      break;
			}	}
         break;
      }
      break;
   }
   return FALSE;
}

int JabberOptInit( WPARAM wParam, LPARAM lParam )
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize      = sizeof( odp );
	odp.hInstance   = hInst;
	odp.pszGroup    = "Network";
	odp.pszTemplate = MAKEINTRESOURCEA( IDD_OPT_JABBERMAIN );
	odp.pszTitle    = jabberModuleName;
	odp.pfnDlgProc  = OptionsDlgProc;
	odp.flags       = ODPF_BOLDGROUPS;
	JCallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	return 0;
}
