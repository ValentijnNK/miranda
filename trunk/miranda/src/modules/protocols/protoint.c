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

#include "m_protoint.h"

int Proto_CallContactService(WPARAM wParam,LPARAM lParam);

static int MyCallProtoService( const char *szModule, const char *szService, WPARAM wParam, LPARAM lParam )
{
	char str[MAXMODULELABELLENGTH];
	mir_snprintf( str, sizeof(str), "%s%s", szModule, szService );
	return CallService(str,wParam,lParam);
}

static HANDLE fnAddToList( PROTO_INTERFACE* ppi, int flags, PROTOSEARCHRESULT* psr )
{	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_ADDTOLIST, flags, (LPARAM)psr );
}

static HANDLE fnAddToListByEvent( PROTO_INTERFACE* ppi, int flags, int iContact, HANDLE hDbEvent )
{	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_ADDTOLISTBYEVENT, MAKELONG(flags, iContact), (LPARAM)hDbEvent );
}

static int fnAuthorize( PROTO_INTERFACE* ppi, HANDLE hContact )
{	return MyCallProtoService( ppi->szPhysName, PS_AUTHALLOW, (WPARAM)hContact, 0 );
}

static int fnAuthDeny( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szReason )
{	return MyCallProtoService( ppi->szPhysName, PS_AUTHDENY, (WPARAM)hContact, (LPARAM)szReason );
}

static int fnAuthRecv( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSS_ADDED, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnAuthRequest( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szMessage )
{	CCSDATA ccs = { hContact, PSS_AUTHREQUEST, 0, (LPARAM)szMessage };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static HANDLE fnChangeInfo( PROTO_INTERFACE* ppi, int iInfoType, void* pInfoData )
{	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_CHANGEINFO, iInfoType, ( LPARAM )pInfoData );
}

static int fnFileAllow( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer, const char* szPath )
{	CCSDATA ccs = { hContact, PSS_FILEALLOW, (WPARAM)hTransfer, (LPARAM)szPath };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileCancel( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer )
{	CCSDATA ccs = { hContact, PSS_FILECANCEL, (WPARAM)hTransfer, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileDeny( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hTransfer, const char* szReason )
{	CCSDATA ccs = { hContact, PSS_FILEDENY, (WPARAM)hTransfer, (LPARAM)szReason };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnFileResume( PROTO_INTERFACE* ppi, HANDLE hTransfer, int* action, const char** szFilename )
{	PROTOFILERESUME pfr = { *action, *szFilename };
	int res = MyCallProtoService( ppi->szPhysName, PS_FILERESUME, ( WPARAM )hTransfer, ( LPARAM )&pfr );
	*action = pfr.action; *szFilename = pfr.szFilename;
	return res;
}

static int fnGetCaps( PROTO_INTERFACE* ppi, int type )
{	return MyCallProtoService( ppi->szPhysName, PS_GETCAPS, type, 0 );
}

static HICON fnGetIcon( PROTO_INTERFACE* ppi, int iconIndex )
{	return ( HICON )MyCallProtoService( ppi->szPhysName, PS_LOADICON, iconIndex, 0 );
}

static int fnGetInfo( PROTO_INTERFACE* ppi, HANDLE hContact, int flags )
{	CCSDATA ccs = { hContact, PSS_GETINFO, flags, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static HANDLE fnSearchBasic( PROTO_INTERFACE* ppi, const char* id )
{	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_BASICSEARCH, 0, ( LPARAM )id );
}

static HANDLE fnSearchByEmail( PROTO_INTERFACE* ppi, const char* email )
{	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_SEARCHBYEMAIL, 0, ( LPARAM )email );
}

static HANDLE fnSearchByName( PROTO_INTERFACE* ppi, const char* nick, const char* firstName, const char* lastName )
{	PROTOSEARCHBYNAME psn;
	psn.pszNick = ( char* )nick;
	psn.pszFirstName = ( char* )firstName;
	psn.pszLastName = ( char* )lastName;
	return ( HANDLE )MyCallProtoService( ppi->szPhysName, PS_SEARCHBYNAME, 0, ( LPARAM )&psn );
}

static HWND fnSearchAdvanced( PROTO_INTERFACE* ppi, HWND owner )
{	return ( HWND )MyCallProtoService( ppi->szPhysName, PS_SEARCHBYADVANCED, 0, ( LPARAM )owner );
}

static HWND fnCreateExtendedSearchUI( PROTO_INTERFACE* ppi, HWND owner )
{	return ( HWND )MyCallProtoService( ppi->szPhysName, PS_CREATEADVSEARCHUI, 0, ( LPARAM )owner );
}

static int fnRecvContacts( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_CONTACTS, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvFile( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVFILE* evt )
{	CCSDATA ccs = { hContact, PSR_FILE, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvMessage( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_MESSAGE, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvUrl( PROTO_INTERFACE* ppi, HANDLE hContact, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_URL, 0, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendContacts( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, int nContacts, HANDLE* hContactsList )
{	CCSDATA ccs = { hContact, PSS_CONTACTS, MAKEWPARAM(flags,nContacts), (LPARAM)hContactsList };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendFile( PROTO_INTERFACE* ppi, HANDLE hContact, const char* szDescription, char** ppszFiles )
{	CCSDATA ccs = { hContact, PSS_FILE, (WPARAM)szDescription, (LPARAM)ppszFiles };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendMessage( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, const char* msg )
{	CCSDATA ccs = { hContact, PSS_MESSAGE, flags, (LPARAM)msg };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendUrl( PROTO_INTERFACE* ppi, HANDLE hContact, int flags, const char* url )
{	CCSDATA ccs = { hContact, PSS_URL, flags, (LPARAM)url };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetApparentMode( PROTO_INTERFACE* ppi, HANDLE hContact, int mode )
{	CCSDATA ccs = { hContact, PSS_SETAPPARENTMODE, mode, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetStatus( PROTO_INTERFACE* ppi, int iNewStatus )
{	return MyCallProtoService( ppi->szPhysName, PS_SETSTATUS, iNewStatus, 0 );
}

static int fnGetAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact )
{	CCSDATA ccs = { hContact, PSS_GETAWAYMSG, 0, 0 };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnRecvAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact, int statusMode, PROTORECVEVENT* evt )
{	CCSDATA ccs = { hContact, PSR_AWAYMSG, statusMode, (LPARAM)evt };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSendAwayMsg( PROTO_INTERFACE* ppi, HANDLE hContact, HANDLE hProcess, const char* msg )
{	CCSDATA ccs = { hContact, PSS_AWAYMSG, (WPARAM)hProcess, (LPARAM)msg };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

static int fnSetAwayMsg( PROTO_INTERFACE* ppi, int iStatus, const char* msg )
{	return MyCallProtoService( ppi->szPhysName, PS_SETAWAYMSG, iStatus, ( LPARAM )msg );
}

static int fnUserIsTyping( PROTO_INTERFACE* ppi, HANDLE hContact, int type )
{	CCSDATA ccs = { hContact, PSS_USERISTYPING, (WPARAM)hContact, type };
	return Proto_CallContactService( 0, (LPARAM)&ccs );
}

// creates the default protocol container for compatibility with the old plugins

PROTO_INTERFACE* AddDefaultAccount( const char* szProtoName )
{
	PROTO_INTERFACE* ppi = ( PROTO_INTERFACE* )mir_alloc( sizeof( PROTO_INTERFACE ));
	if ( ppi != NULL ) {
		memset( ppi, 0, sizeof( PROTO_INTERFACE ));
		ppi->iVersion = 1;
		ppi->bOldProto = TRUE;
		ppi->szPhysName = mir_strdup( szProtoName );
		ppi->szProtoName = mir_strdup( szProtoName );
		ppi->tszUserName = mir_a2t( szProtoName );

		ppi->AddToList              = fnAddToList;
		ppi->AddToListByEvent       = fnAddToListByEvent;
		ppi->Authorize              = fnAuthorize;
		ppi->AuthDeny               = fnAuthDeny;
		ppi->AuthRecv               = fnAuthRecv;
		ppi->AuthRequest            = fnAuthRequest;
		ppi->ChangeInfo             = fnChangeInfo;
		ppi->FileAllow              = fnFileAllow;
		ppi->FileCancel             = fnFileCancel;
		ppi->FileDeny               = fnFileDeny;
		ppi->FileResume             = fnFileResume;
		ppi->GetCaps                = fnGetCaps;
		ppi->GetIcon                = fnGetIcon;
		ppi->GetInfo                = fnGetInfo;
		ppi->SearchBasic            = fnSearchBasic;
		ppi->SearchByEmail          = fnSearchByEmail;
		ppi->SearchByName           = fnSearchByName;
		ppi->SearchAdvanced         = fnSearchAdvanced;
		ppi->CreateExtendedSearchUI = fnCreateExtendedSearchUI;
		ppi->RecvContacts           = fnRecvContacts;
		ppi->RecvFile               = fnRecvFile;
		ppi->RecvMessage            = fnRecvMessage;
		ppi->RecvUrl                = fnRecvUrl;
		ppi->SendContacts           = fnSendContacts;
		ppi->SendFile               = fnSendFile;
		ppi->SendMessage            = fnSendMessage;
		ppi->SendUrl                = fnSendUrl;
		ppi->SetApparentMode        = fnSetApparentMode;
		ppi->SetStatus              = fnSetStatus;
		ppi->GetAwayMsg             = fnGetAwayMsg;
		ppi->RecvAwayMsg            = fnRecvAwayMsg;
		ppi->SendAwayMsg            = fnSendAwayMsg;
		ppi->SetAwayMsg             = fnSetAwayMsg;
		ppi->UserIsTyping           = fnUserIsTyping;
	}
	return ppi;
}
