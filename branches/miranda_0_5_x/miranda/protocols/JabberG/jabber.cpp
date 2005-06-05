/*

Jabber Protocol Plugin for Miranda IM
Copyright ( C ) 2002-04  Santithorn Bunchua
Copyright ( C ) 2005     George Hazan

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

*/

#include "jabber.h"
#include "jabber_ssl.h"
#include "jabber_iq.h"
#include "resource.h"
#include "version.h"

HINSTANCE hInst;
PLUGINLINK *pluginLink;

PLUGININFO pluginInfo = {
	sizeof( PLUGININFO ),
	"Jabber Protocol",
	__VERSION_DWORD,
	"Jabber protocol plugin for Miranda IM ( "__DATE__" )",
	"George Hazan",
	"ghazan@postman.ru",
	"( c ) 2002-05 Santithorn Bunchua, George Hazan",
	"http://miranda-im.org/download/details.php?action=viewfile&id=437",
	0,
	0
};

HANDLE hMainThread = NULL;
DWORD jabberMainThreadId;
char* jabberProtoName;	// "JABBER"
char* jabberModuleName;	// "Jabber"
CRITICAL_SECTION mutex;
HANDLE hNetlibUser;
// Main jabber server connection thread global variables
struct ThreadData *jabberThreadInfo = NULL;
BOOL jabberConnected = FALSE;
BOOL jabberOnline = FALSE;
BOOL jabberChatDllPresent = FALSE;
int jabberStatus = ID_STATUS_OFFLINE;
int jabberDesiredStatus;
BOOL modeMsgStatusChangePending = FALSE;
BOOL jabberChangeStatusMessageOnly = FALSE;
char* jabberJID = NULL;
char* streamId = NULL;
DWORD jabberLocalIP;
UINT jabberCodePage;
JABBER_MODEMSGS modeMsgs;
//char* jabberModeMsg;
CRITICAL_SECTION modeMsgMutex;
char* jabberVcardPhotoFileName = NULL;
char* jabberVcardPhotoType = NULL;
BOOL  jabberSendKeepAlive;
HICON jabberIcon[JABBER_ICON_TOTAL];

// SSL-related global variable
HMODULE hLibSSL = NULL;
PVOID jabberSslCtx;

const char xmlnsAdmin[]  = "<query xmlns='http://jabber.org/protocol/muc#admin'>";
const char xmlnsOwner[]  = "<query xmlns='http://jabber.org/protocol/muc#owner'>";

HWND hwndJabberAgents = NULL;
HWND hwndJabberGroupchat = NULL;
HWND hwndJabberJoinGroupchat = NULL;
HWND hwndAgentReg = NULL;
HWND hwndAgentRegInput = NULL;
HWND hwndAgentManualReg = NULL;
HWND hwndRegProgress = NULL;
HWND hwndJabberVcard = NULL;
HWND hwndMucVoiceList = NULL;
HWND hwndMucMemberList = NULL;
HWND hwndMucModeratorList = NULL;
HWND hwndMucBanList = NULL;
HWND hwndMucAdminList = NULL;
HWND hwndMucOwnerList = NULL;
HWND hwndJabberChangePassword = NULL;

int JabberOptInit( WPARAM wParam, LPARAM lParam );
int JabberUserInfoInit( WPARAM wParam, LPARAM lParam );
int JabberMsgUserTyping( WPARAM wParam, LPARAM lParam );
void JabberMenuInit( void );
int JabberSvcInit( void );
int JabberSvcUninit( void );

BOOL WINAPI DllMain( HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved )
{
#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	hInst = hModule;
	return TRUE;
}

extern "C" __declspec( dllexport ) PLUGININFO *MirandaPluginInfo( DWORD mirandaVersion )
{
	if ( mirandaVersion < PLUGIN_MAKE_VERSION( 0,4,0,0 )) {
		MessageBox( NULL, "The Jabber protocol plugin cannot be loaded. It requires Miranda IM 0.4 or later.", "Jabber Protocol Plugin", MB_OK|MB_ICONWARNING|MB_SETFOREGROUND|MB_TOPMOST );
		return NULL;
	}

	return &pluginInfo;
}

///////////////////////////////////////////////////////////////////////////////
// OnPreShutdown - prepares Miranda to be shut down

static int OnPreShutdown( WPARAM wParam, LPARAM lParam )
{
	if ( hwndJabberAgents ) SendMessage( hwndJabberAgents, WM_CLOSE, 0, 0 );
	if ( hwndJabberGroupchat ) SendMessage( hwndJabberGroupchat, WM_CLOSE, 0, 0 );
	if ( hwndJabberJoinGroupchat ) SendMessage( hwndJabberJoinGroupchat, WM_CLOSE, 0, 0 );
	if ( hwndAgentReg ) SendMessage( hwndAgentReg, WM_CLOSE, 0, 0 );
	if ( hwndAgentRegInput ) SendMessage( hwndAgentRegInput, WM_CLOSE, 0, 0 );
	if ( hwndRegProgress ) SendMessage( hwndRegProgress, WM_CLOSE, 0, 0 );
	if ( hwndJabberVcard ) SendMessage( hwndJabberVcard, WM_CLOSE, 0, 0 );
	if ( hwndMucVoiceList ) SendMessage( hwndMucVoiceList, WM_CLOSE, 0, 0 );
	if ( hwndMucMemberList ) SendMessage( hwndMucMemberList, WM_CLOSE, 0, 0 );
	if ( hwndMucModeratorList ) SendMessage( hwndMucModeratorList, WM_CLOSE, 0, 0 );
	if ( hwndMucBanList ) SendMessage( hwndMucBanList, WM_CLOSE, 0, 0 );
	if ( hwndMucAdminList ) SendMessage( hwndMucAdminList, WM_CLOSE, 0, 0 );
	if ( hwndMucOwnerList ) SendMessage( hwndMucOwnerList, WM_CLOSE, 0, 0 );
	if ( hwndJabberChangePassword ) SendMessage( hwndJabberChangePassword, WM_CLOSE, 0, 0 );

	hwndJabberAgents = NULL;
	hwndJabberGroupchat = NULL;
	hwndJabberJoinGroupchat = NULL;
	hwndAgentReg = NULL;
	hwndAgentRegInput = NULL;
	hwndAgentManualReg = NULL;
	hwndRegProgress = NULL;
	hwndJabberVcard = NULL;
	hwndMucVoiceList = NULL;
	hwndMucMemberList = NULL;
	hwndMucModeratorList = NULL;
	hwndMucBanList = NULL;
	hwndMucAdminList = NULL;
	hwndMucOwnerList = NULL;
	hwndJabberChangePassword = NULL;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnModulesLoaded - execute some code when all plugins are initialized

int JabberGcEventHook( WPARAM, LPARAM );
int JabberGcMenuHook( WPARAM, LPARAM );
int JabberGcInit( WPARAM, LPARAM );

static COLORREF crCols[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
HANDLE hChatEvent = NULL, hChatMenu = NULL, hInitChat = NULL, hEvInitChat = NULL;

static int OnModulesLoaded( WPARAM wParam, LPARAM lParam )
{
	JabberWsInit();
	JabberSslInit();
	HookEvent( ME_USERINFO_INITIALISE, JabberUserInfoInit );

	if ( ServiceExists( MS_GC_REGISTER )) {
		jabberChatDllPresent = true;

		GCREGISTER gcr = {0};
		gcr.cbSize = sizeof( GCREGISTER );
		gcr.dwFlags = GC_TYPNOTIF|GC_CHANMGR;
		gcr.iMaxText = 0;
		gcr.nColors = 16;
		gcr.pColors = &crCols[0];
		gcr.pszModuleDispName = jabberProtoName;
		gcr.pszModule = jabberProtoName;
		JCallService( MS_GC_REGISTER, NULL, ( LPARAM )&gcr );

		hChatEvent = HookEvent( ME_GC_EVENT, JabberGcEventHook );
		hChatMenu = HookEvent( ME_GC_BUILDMENU, JabberGcMenuHook );

		char szEvent[ 200 ];
		mir_snprintf( szEvent, sizeof szEvent, "%s\\ChatInit", jabberProtoName );
		hInitChat = CreateHookableEvent( szEvent );
		hEvInitChat = HookEvent( szEvent, JabberGcInit );
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// OnLoad - initialize the plugin instance

static int iconList[] = {
	IDI_GCOWNER,
	IDI_GCADMIN,
	IDI_GCMODERATOR,
	IDI_GCVOICE
};

extern "C" int __declspec( dllexport ) Load( PLUGINLINK *link )
{
	pluginLink = link;

	char text[_MAX_PATH];
	char* p, *q;

	GetModuleFileName( hInst, text, sizeof( text ));
	p = strrchr( text, '\\' );
	p++;
	q = strrchr( p, '.' );
	*q = '\0';
	jabberProtoName = _strdup( p );
	_strupr( jabberProtoName );

	jabberModuleName = _strdup( jabberProtoName );
	_strlwr( jabberModuleName );
	jabberModuleName[0] = toupper( jabberModuleName[0] );

	JabberLog( "Setting protocol/module name to '%s/%s'", jabberProtoName, jabberModuleName );

	DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &hMainThread, THREAD_SET_CONTEXT, FALSE, 0 );
	jabberMainThreadId = GetCurrentThreadId();

	HookEvent( ME_OPT_INITIALISE, JabberOptInit );
	HookEvent( ME_SYSTEM_MODULESLOADED, OnModulesLoaded );
	HookEvent( ME_SYSTEM_PRESHUTDOWN, OnPreShutdown );

	// Register protocol module
	PROTOCOLDESCRIPTOR pd;
	ZeroMemory( &pd, sizeof( PROTOCOLDESCRIPTOR ));
	pd.cbSize = sizeof( PROTOCOLDESCRIPTOR );
	pd.szName = jabberProtoName;
	pd.type = PROTOTYPE_PROTOCOL;
	JCallService( MS_PROTO_REGISTERMODULE, 0, ( LPARAM )&pd );

	// Set all contacts to offline
	HANDLE hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDFIRST, 0, 0 );
	while ( hContact != NULL ) {
		char* szProto = ( char* )JCallService( MS_PROTO_GETCONTACTBASEPROTO, ( WPARAM ) hContact, 0 );
		if ( szProto != NULL && !strcmp( szProto, jabberProtoName ))
			if ( JGetWord( hContact, "Status", ID_STATUS_OFFLINE ) != ID_STATUS_OFFLINE )
				JSetWord( hContact, "Status", ID_STATUS_OFFLINE );

		hContact = ( HANDLE ) JCallService( MS_DB_CONTACT_FINDNEXT, ( WPARAM ) hContact, 0 );
	}

	memset(( char* )&modeMsgs, 0, sizeof( JABBER_MODEMSGS ));
	//jabberModeMsg = NULL;
	jabberCodePage = JGetWord( NULL, "CodePage", CP_ACP );

	InitializeCriticalSection( &mutex );
	InitializeCriticalSection( &modeMsgMutex );

	for ( int i=0; i < JABBER_ICON_TOTAL; i++ )
		jabberIcon[i] = ( HICON )LoadImage( hInst, MAKEINTRESOURCE( iconList[i] ), IMAGE_ICON, 0, 0, 0 );

	srand(( unsigned ) time( NULL ));
	JabberSerialInit();
	JabberIqInit();
	JabberListInit();
	JabberSvcInit();
	JabberMenuInit();
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Unload - destroy the plugin instance

extern "C" int __declspec( dllexport ) Unload( void )
{
	int i;

#ifdef _DEBUG
	OutputDebugString( "Unloading..." );
#endif

	if ( hChatEvent  ) UnhookEvent( hChatEvent );
	if ( hChatMenu   ) UnhookEvent( hChatMenu );
	if ( hEvInitChat ) UnhookEvent( hEvInitChat );

	if ( hInitChat )
		DestroyHookableEvent( hInitChat );

	JabberSvcUninit();
	JabberSslUninit();
	JabberListUninit();
	JabberIqUninit();
	JabberSerialUninit();
	JabberWsUninit();
	DeleteCriticalSection( &modeMsgMutex );
	DeleteCriticalSection( &mutex );
	free( modeMsgs.szOnline );
	free( modeMsgs.szAway );
	free( modeMsgs.szNa );
	free( modeMsgs.szDnd );
	free( modeMsgs.szFreechat );
	free( jabberModuleName );
	free( jabberProtoName );
	if ( jabberVcardPhotoFileName ) {
		DeleteFile( jabberVcardPhotoFileName );
		free( jabberVcardPhotoFileName );
	}
	if ( jabberVcardPhotoType ) free( jabberVcardPhotoType );
	if ( streamId ) free( streamId );

	for ( i=0; i < JABBER_ICON_TOTAL; i++ )
		DestroyIcon( jabberIcon[i] );

	if ( hMainThread ) CloseHandle( hMainThread );
	return 0;
}
