/*
Plugin of Miranda IM for communicating with users of the MSN Messenger protocol.
Copyright (c) 2003-5 George Hazan.
Copyright (c) 2002-3 Richard Hughes (original version).

Miranda IM: the free icq client for MS Windows
Copyright (C) 2000-2002 Richard Hughes, Roland Rabien & Tristan Van de Vreede

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

#include "msn_global.h"

#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

struct p2p_threadParams
{
	HANDLE s;
	filetransfer* ft;
};

static char sttP2Pheader[] =
	"Content-Type: application/x-msnmsgrp2p\r\n"
	"P2P-Dest: %s\r\n\r\n";

static char sttVoidNonce[] = "{00000000-0000-0000-0000-000000000000}";

static void sttLogHeader( P2P_Header* hdrdata )
{
	MSN_DebugLog( "--- Printing message header" );
	MSN_DebugLog( "    SessionID = %lu", hdrdata->mSessionID );
	MSN_DebugLog( "    MessageID = %lu", hdrdata->mID );
	MSN_DebugLog( "    Offset of data = %I64u", hdrdata->mOffset );
	MSN_DebugLog( "    Total amount of data = %I64u", hdrdata->mTotalSize );
	MSN_DebugLog( "    Data in packet = %lu bytes", hdrdata->mPacketLen );
	MSN_DebugLog( "    Flags = %08X", hdrdata->mFlags );
	MSN_DebugLog( "    Acknowledged session ID: %lu", hdrdata->mAckSessionID );
	MSN_DebugLog( "    Acknowledged message ID: %lu", hdrdata->mAckUniqueID );
	MSN_DebugLog( "    Acknowledged data size: %I64u", hdrdata->mAckDataSize );
	MSN_DebugLog( "------------------------" );
}

static bool sttIsCancelCommand( const BYTE* p )
{
	if ( !memcmp( p, "BYE MSNMSGR:", 12 ))
		return true;

	return ( memcmp( p, "MSNSLP/1.0 603 ", 15 ) == 0 );
}

static char* getNewUuid()
{
	BYTE* p;
	UUID id;

	UuidCreate( &id );
	UuidToStringA( &id, &p );
	int len = strlen(( const char* )p );
	char* result = ( char* )malloc( len+3 );
	result[0]='{';
	memcpy( result+1, p, len );
	result[ len+1 ] = '}';
	result[ len+2 ] = 0;
	strupr( result );
	RpcStringFreeA( &p );
	return result;
}

static int sttCreateListener(
	ThreadData* info,
	filetransfer* ft,
	pThreadFunc thrdFunc,
	char* szBody, size_t cbBody )
{
	char ipaddr[256];
	if ( MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ))) {
		MSN_DebugLog( "Cannot detect my host address" );
		return 0;
	}

	NETLIBBIND nlb = {0};
	nlb.cbSize = sizeof( nlb );
	nlb.pfnNewConnectionV2 = MSN_ConnectionProc;
	nlb.wPort = 0;	// Use user-specified incoming port ranges, if available
	if (( ft->mIncomingBoundPort = (HANDLE) MSN_CallService(MS_NETLIB_BINDPORT, (WPARAM) hNetlibUser, ( LPARAM )&nlb)) == NULL ) {
		MSN_DebugLog( "Unable to bind the port for incoming transfers" );
		return 0;
	}

	ft->mIncomingPort = nlb.wPort;

	char* szUuid = getNewUuid();
	{
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_P2P_DIRECT;
		newThread->mCaller = 3;
		newThread->mP2pSession = ft;
		newThread->mParentThread = info;
		strncpy( newThread->mCookie, ( char* )szUuid, sizeof( newThread->mCookie ));
		ft->hWaitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

		newThread->startThread( thrdFunc );
	}

	char hostname[256];

	gethostname( hostname, sizeof( hostname ));
	PHOSTENT he = gethostbyname( hostname );

	hostname[0] = 0;
	for( unsigned i=0; i<sizeof( hostname )/16 && he->h_addr_list[i] ; ++i ) {
		if ( i != 0 ) strcat( hostname, " " );
		strcat( hostname, inet_ntoa( *( PIN_ADDR )he->h_addr_list[i] ));
	}

	int cbBodyLen = mir_snprintf( szBody, cbBody,
		"Bridge: TCPv1\r\n"
		"Listening: true\r\n"
		"Nonce: %s\r\n"
		"IPv4Internal-Addrs: %s\r\n"
		"IPv4Internal-Port: %u\r\n"
		"SessionID: %lu\r\n"
		"SChannelState: 0\r\n\r\n%c",
		szUuid, 
		hostname, ft->mIncomingPort, 
		ft->p2p_sessionid, 0 );
	free( szUuid );

	return cbBodyLen;
}

/////////////////////////////////////////////////////////////////////////////////////////
// sttSavePicture2disk - final handler for avatars downloading

static void sttSavePicture2disk( ThreadData* info, filetransfer* ft )
{
	if ( !ft->inmemTransfer )
		return;

	//---- Save temporary PNG image to disk --------------------
	#if defined( _DEBUG )
		char* Path = getenv( "TEMP" );
		if ( Path == NULL )
			Path = getenv( "TMP" );

		char tPathName[ MAX_PATH ];
		if ( Path == NULL ) {
			MSN_DebugLog( "Temporary file is created in the current directory: %s",
			getcwd( tPathName, sizeof( tPathName )));
		}
		else {
			MSN_DebugLog( "Temporary path found: %s", Path );
			strcpy( tPathName, Path );
		}

		strcat( tPathName, "\\avatar.png" );
		MSN_DebugLog( "Opening temporary file '%s'", tPathName );
		{	FILE* out = fopen( tPathName, "wb" );
			if ( out ) {
				fwrite( ft->fileBuffer, ft->std.totalBytes, 1, out );
				fclose( out );
		}	}
	#endif

	//---- Converting memory buffer to bitmap and saving it to disk
	if ( !MSN_LoadPngModule() )
		return;

	BITMAPINFOHEADER* pDib;
	PNG2DIB convert;
	convert.pSource = (BYTE*)ft->fileBuffer;
	convert.cbSourceSize = ft->std.totalBytes;
	convert.pResult = &pDib;
	if ( !CallService( MS_PNG2DIB, 0, (LPARAM)&convert ))
		return;

	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof( AI );
	AI.format = PA_FORMAT_BMP;
	AI.hContact = info->mJoinedContacts[0];
	MSN_GetAvatarFileName( info->mJoinedContacts[0], AI.filename, sizeof( AI.filename ));
	FILE* out = fopen( AI.filename, "wb" );
	if ( out != NULL ) {
		BITMAPFILEHEADER tHeader = { 0 };
		tHeader.bfType = 0x4d42;
		tHeader.bfOffBits = sizeof( tHeader ) + sizeof( BITMAPINFOHEADER );
		tHeader.bfSize = tHeader.bfOffBits + pDib->biSizeImage;
		fwrite( &tHeader, sizeof( tHeader ), 1, out );
		fwrite( pDib, sizeof( BITMAPINFOHEADER ), 1, out );
		fwrite( pDib+1, pDib->biSizeImage, 1, out );
		fclose( out );

		MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_AVATAR, ACKRESULT_SUCCESS, HANDLE( &AI ), NULL );
	}
	else MSN_SendBroadcast( info->mJoinedContacts[0], ACKTYPE_AVATAR, ACKRESULT_FAILED, HANDLE( &AI ), NULL );

   GlobalFree( pDib );
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendAck - sends MSN P2P acknowledgement to the received message

static char sttVoidSession[] = "ACHTUNG!!! an attempt made to send a message via the empty session";

void __stdcall p2p_sendAck( filetransfer* ft, ThreadData* info, P2P_Header* hdrdata )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	char* buf = ( char* )alloca( 1000 + strlen( ft->p2p_dest ));
	char* p = buf + sprintf( buf, sttP2Pheader, ft->p2p_dest );

	P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
	memset( tHdr, 0, sizeof( P2P_Header ));
	tHdr->mSessionID = hdrdata->mSessionID;
	tHdr->mID = ++ft->p2p_msgid;
	tHdr->mAckDataSize = hdrdata->mTotalSize;
	tHdr->mTotalSize = hdrdata->mTotalSize;
	tHdr->mFlags = 2;
	tHdr->mAckSessionID = hdrdata->mID;
	tHdr->mAckUniqueID = hdrdata->mAckSessionID;
	*( long* )p = 0; p += sizeof( long );

	if ( info->mType == SERVER_P2P_DIRECT ) {
		DWORD *p2pPacket = (DWORD*)tHdr-1;
		*p2pPacket = sizeof( P2P_Header );
		info->send(( char* )p2pPacket, sizeof( P2P_Header ) + sizeof( DWORD ));
	}
	else info->sendRawMessage( 'D', buf, int( p - buf ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendEndSession - sends MSN P2P file transfer end packet

static void __stdcall p2p_sendEndSession( ThreadData* info, filetransfer* ft )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	char* buf = ( char* )alloca( 1000 + strlen( ft->p2p_dest ));
	char* p = buf + sprintf( buf, sttP2Pheader, ft->p2p_dest );

	P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
	memset( tHdr, 0, sizeof( P2P_Header ));
	tHdr->mSessionID = ft->p2p_sessionid;

    tHdr->mAckSessionID = ft->p2p_sendmsgid;

	if ( ft->std.sending)
	{
		tHdr->mFlags = 0x40;
		tHdr->mID = ++ft->p2p_msgid;
		tHdr->mAckSessionID = tHdr->mID - 2;
	}
	else
	{
		tHdr->mID = ++ft->p2p_msgid;
		tHdr->mAckUniqueID = 0x8200000f;
		tHdr->mFlags = 0x80;
        tHdr->mAckDataSize = ft->std.currentFileSize;
	}

	if ( info->mType == SERVER_P2P_DIRECT ) {
		DWORD *p2pPacket = (DWORD*)tHdr-1;
		*p2pPacket = sizeof( P2P_Header );
		info->send(( char* )p2pPacket, sizeof( P2P_Header ) + sizeof( DWORD ));
	}
	else info->sendRawMessage( 'D', buf, int( p - buf ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendSlp - send MSN P2P SLP packet

void __stdcall p2p_sendSlp(
	ThreadData*		info,
	filetransfer*	ft,
	MimeHeaders&	pHeaders,
	int				iKind,
	const char*		szContent,
	size_t			szContLen )
{
	if ( ft == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	char szMyEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", NULL, szMyEmail, sizeof( szMyEmail ));

	char* buf = ( char* )alloca( 1000 + szContLen + pHeaders.getLength());
	char* p = buf;

	if ( info == NULL || info->mType != SERVER_P2P_DIRECT )
		p += sprintf( p, sttP2Pheader, ft->p2p_dest );
	else 
		p += sizeof( DWORD );

	P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
	memset( tHdr, 0, sizeof( P2P_Header ));

	char* pktStart = p;

	switch ( iKind ) {
		case 200: p += sprintf( p, "MSNSLP/1.0 200 OK" );	break;
		case 603: p += sprintf( p, "MSNSLP/1.0 603 DECLINE" ); break;
		case -1:  p += sprintf( p, "BYE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		case -2:  p += sprintf( p, "INVITE MSNMSGR:%s MSNSLP/1.0", ft->p2p_dest ); break;
		default: return;
	}

	p += sprintf( p,
		"\r\nTo: <msnmsgr:%s>\r\n"
		"From: <msnmsgr:%s>\r\n"
		"Via: MSNSLP/1.0/TLP ;branch=%s\r\n", ft->p2p_dest, szMyEmail, ft->p2p_branch );

	p = pHeaders.writeToBuffer( p );

	p += sprintf( p, "Content-Length: %d\r\n\r\n", szContLen );
	memcpy( p, szContent, szContLen );
	p += szContLen;
	tHdr->mID = ++ft->p2p_msgid;
	tHdr->mAckSessionID = ft->p2p_acksessid;
	tHdr->mTotalSize = tHdr->mPacketLen = int( p - pktStart );

	if (iKind == 603)
		ft->p2p_byemsgid = ft->p2p_msgid;

	*( DWORD* )p = 0; p += sizeof( DWORD );

	if ( info == NULL ) {
		MsgQueue_Add( ft->std.hContact, 'D', buf, int( p - buf ), NULL );
		msnNsThread->sendPacket( "XFR", "SB" );
	}
	else if ( info->mType == SERVER_P2P_DIRECT ) {
		DWORD *p2pPacket = (DWORD*)tHdr-1;
		*p2pPacket = p - buf - 2 * sizeof( DWORD );
		info->send(( char* )p2pPacket, *p2pPacket + sizeof( DWORD ));
	}
	else info->sendRawMessage( 'D', buf, int( p - buf ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendBye - closes P2P session

void __stdcall p2p_sendBye( ThreadData* info, filetransfer* ft )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders(5);
	tHeaders.addString( "CSeq", "0 " );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionclosebody" );

	char szContents[ 50 ];
	p2p_sendSlp( info, ft, tHeaders, -1, szContents,
		mir_snprintf( szContents, sizeof( szContents ), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c", 
		ft->p2p_sessionid, 0 ));
    ft->p2p_byemsgid = ft->p2p_msgid;
}

void __stdcall p2p_sendCancel( ThreadData* info, filetransfer* ft )
{
	p2p_sendBye(info, ft);
	p2p_sendEndSession(info, ft);
	ft->bCanceled = true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendStatus - send MSN P2P status and its description

void __stdcall p2p_sendStatus( filetransfer* ft, ThreadData* info, long lStatus )
{
	if ( ft == NULL || info == NULL ) {
		MSN_DebugLog( sttVoidSession );
		return;
	}

	MimeHeaders tHeaders( 5 );
	tHeaders.addString( "CSeq", "1 " );
	tHeaders.addString( "Call-ID", ft->p2p_callID );
	tHeaders.addLong( "Max-Forwards", 0 );
	tHeaders.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	char szContents[ 50 ];
	p2p_sendSlp( info, ft, tHeaders, lStatus, szContents,
		mir_snprintf( szContents, sizeof( szContents ), "SessionID: %lu\r\nSChannelState: 0\r\n\r\n%c", 
		ft->p2p_sessionid, 0 ));
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_connectTo - connects to a remote P2P server

static char p2p_greeting[8] = { 4, 0, 0, 0, 'f', 'o', 'o', 0  };

static void sttSendPacket( ThreadData* T, P2P_Header& hdr )
{
	DWORD len = sizeof( P2P_Header );
	T->send(( char* )&len, sizeof( DWORD ));
	T->send(( char* )&hdr, sizeof( P2P_Header ));
}

bool p2p_connectTo( ThreadData* info )
{
	filetransfer* ft = info->mP2pSession;

	NETLIBOPENCONNECTION tConn = { 0 };
	tConn.cbSize = sizeof( tConn );
	tConn.flags = NLOCF_V2;
	tConn.szHost = info->mServer;
	tConn.timeout = 5;
	{
 		char* tPortDelim = strrchr( info->mServer, ':' );
		if ( tPortDelim != NULL ) {
			*tPortDelim = '\0';
			tConn.wPort = ( WORD )atol( tPortDelim+1 );
	}	}

	while( true ) {
		char* pSpace = strchr( info->mServer, ' ' );
		if ( pSpace != NULL )
			*pSpace = 0;

		MSN_DebugLog( "Connecting to %s:%d", info->mServer, tConn.wPort );

		HANDLE h = ( HANDLE )MSN_CallService( MS_NETLIB_OPENCONNECTION, ( WPARAM )hNetlibUser, ( LPARAM )&tConn );
		if ( h != NULL ) {
			info->s = h;
			ft->mThreadId = info->mUniqueID;
			break;
		}
		{	TWinErrorCode err;
			MSN_DebugLog( "Connection Failed (%d): %s", err.mErrorCode, err.getText() );
		}

		if ( pSpace == NULL ) {
			if ( ft->std.sending )
				MSN_PingParentThread( info->mParentThread, ft );
			return false;
		}
			
		strdel( info->mServer, int( pSpace - info->mServer )+1 );
	}

	info->send( p2p_greeting, sizeof( p2p_greeting ));

	P2P_Header reply;
	memset( &reply, 0, sizeof( P2P_Header ));
	reply.mID = ++ft->p2p_msgid;
	reply.mFlags = 0x100;

	strdel( info->mCookie, 1 );
	info->mCookie[ strlen( info->mCookie )-1 ] = 0;
	UuidFromStringA(( BYTE* )info->mCookie, ( UUID* )&reply.mAckSessionID );
	sttSendPacket( info, reply );

	long cbPacketLen;
	HReadBuffer buf( info, 0 );
	BYTE* p;
	if (( p = buf.surelyRead( 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		return false;
	}

	cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( cbPacketLen )) == NULL )
		return false;

	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_listen - acts like a local P2P server

bool p2p_listen( ThreadData* info )
{
	filetransfer* ft = info->mP2pSession; 
	DWORD ftID = ft->p2p_sessionid;

	switch( WaitForSingleObject( ft->hWaitEvent, 5000 )) {
	case WAIT_TIMEOUT:
	case WAIT_FAILED:
		MSN_DebugLog( "Incoming connection timed out, closing file transfer" );
		if (( ft = p2p_getSessionByID( ftID )) != NULL )
			if ( ft->std.sending )
				MSN_PingParentThread( info->mParentThread, ft );
LBL_Error:
		MSN_DebugLog( "File transfer failed" );
		return false;
	}

	HReadBuffer buf( info, 0 );
	BYTE* p;

	ft->mThreadId = info->mUniqueID;
	
	if (( p = buf.surelyRead( 8 )) == NULL )
		goto LBL_Error;

	if ( memcmp( p, p2p_greeting, 8 ) != NULL ) {
		MSN_DebugLog( "Invalid input data, exiting" );
		goto LBL_Error;
	}

	if (( p = buf.surelyRead( 4 )) == NULL ) {
		MSN_DebugLog( "Error reading data, closing filetransfer" );
		goto LBL_Error;
	}

	long cbPacketLen = *( long* )p;
	if (( p = buf.surelyRead( cbPacketLen )) == NULL )
		goto LBL_Error;

	UUID uuidCookie;
	strdel( info->mCookie, 1 );
	info->mCookie[ strlen(info->mCookie)-1 ] = 0;
	UuidFromStringA(( BYTE* )info->mCookie, &uuidCookie );

	P2P_Header* pCookie = ( P2P_Header* )p;
	if ( memcmp( &pCookie->mAckSessionID, &uuidCookie, sizeof( UUID ))) {
		MSN_DebugLog( "Invalid input cookie, exiting" );
		goto LBL_Error;
	}

	pCookie->mID = ++ft->p2p_msgid;
	sttSendPacket( info, *pCookie );
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFeedThread - sends a file via server

void __cdecl p2p_sendFeedThread( ThreadData* info )
{
	HANDLE s = info->s; info->s = NULL;
	filetransfer* ft = info->mP2pSession;

	if ( ft->p2p_sendmsgid == 0 )
		ft->p2p_sendmsgid = ++ft->p2p_msgid;

	while ( ft->std.currentFileProgress < ft->std.currentFileSize )
	{
		if ( ft->bCanceled ) {
			MSN_DebugLog( "File transfer canceled" );
			break;
		}

		ThreadData* T = MSN_GetThreadByConnection( s );
		if ( T == NULL || p2p_sendPortion( ft, T ) == 0 ) {
			MSN_DebugLog( "File transfer failed" );
			break;
}	}	}

void __stdcall p2p_sendFeedStart( filetransfer* ft, ThreadData* T )
{
	if ( ft->std.sending )
	{
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_FILETRANS;
		newThread->mP2pSession = ft;
		newThread->s = T->s;
		newThread->startThread(( pThreadFunc )p2p_sendFeedThread );
}	}

LONG __stdcall p2p_sendPortion( filetransfer* ft, ThreadData* T )
{
	LONG trid;
	char databuf[ 1500 ], *p = databuf;

	// Compute the amount of data to send
	const long fportion = T->mType == SERVER_P2P_DIRECT ? 1352 : 1202;
	const unsigned long portion = 
		( fportion + ft->std.currentFileProgress > ft->std.currentFileSize ) ?
		ft->std.currentFileSize - ft->std.currentFileProgress : fportion;

	// Fill data size for direct transfer

	if ( T->mType != SERVER_P2P_DIRECT )
		p += sprintf( p, sttP2Pheader, ft->p2p_dest );
	else
	{
		*( unsigned long* )p = portion + sizeof( P2P_Header );
		p += sizeof( unsigned long );
	}

	// Fill P2P header
	P2P_Header* H = ( P2P_Header* ) p;
	p += sizeof( P2P_Header );

	memset( H, 0, sizeof( P2P_Header ));
	H->mSessionID = ft->p2p_sessionid;
	H->mID = ft->p2p_sendmsgid;
	H->mFlags = ft->p2p_appID == 2 ? 0x01000030 : 0x20;
	H->mTotalSize = ft->std.currentFileSize;
	H->mOffset = ft->std.currentFileProgress;
	H->mPacketLen = portion;
	H->mAckSessionID = ft->p2p_acksessid;

	// Fill data (payload) for transfer
	::read( ft->fileId, p, portion );
	p += portion;

	if ( T->mType == SERVER_P2P_DIRECT )
		trid = T->send( databuf, p - databuf);
	else
	{
		// Define packet footer for server transfer
		*( unsigned long * )p = htonl(ft->p2p_appID); 
		p += sizeof( unsigned long );

		trid = T->sendRawMessage( 'D', ( char * )databuf, p - databuf);
	}

	ft->std.totalProgress += portion;
	ft->std.currentFileProgress += portion;
	if ( ft->p2p_appID == 2 )
		MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );

	return trid;
}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_sendFileDirectly - sends a file via MSN P2P protocol

void p2p_sendRecvFileDirectly( ThreadData* info )
{
	BYTE* p;

	p2p_sendFeedStart( info->mP2pSession, info );

	HReadBuffer buf( info, 0 );
	for ( ;; ) {
		if (( p = buf.surelyRead( 4 )) == NULL ) {
			info->mP2pSession->bCanceled = true;
 			MSN_DebugLog( "File transfer failed" );
			break;
		}

		long cbPacketLen = *( long* )p;
		if (( p = buf.surelyRead( cbPacketLen )) == NULL ) {
			info->mP2pSession->bCanceled = true;
 			MSN_DebugLog( "File transfer failed" );
			break;
		}

		p2p_processMsg( info, (char*)p );
		
		if ( !p2p_sessionRegistered( info->mP2pSession ))
			break;
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// bunch of thread functions to cover all variants of P2P file transfers

void __cdecl p2p_fileActiveThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_fileActiveThread() started: connecting to '%s'", info->mServer );

	if ( p2p_connectTo( info ))
		p2p_sendRecvFileDirectly( info );
}

void __cdecl p2p_filePassiveThread( ThreadData* info )
{
	MSN_DebugLog( "p2p_filePassiveThread() started: listening" );

	if ( p2p_listen( info ))
		p2p_sendRecvFileDirectly( info );
}


/////////////////////////////////////////////////////////////////////////////////////////
// p2p_processMsg - processes all MSN P2P incoming messages

static void sttInitFileTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2,
	const char*		msgbody )
{
	char szMyEmail[ MSN_MAX_EMAIL_LEN ], szContactEmail[ MSN_MAX_EMAIL_LEN ];
	if ( MSN_GetStaticString( "e-mail", info->mJoinedContacts[0], szContactEmail, sizeof( szContactEmail )))
		return;

	MSN_GetStaticString( "e-mail", NULL, szMyEmail, sizeof( szMyEmail ));

	const char	*szCallID = tFileInfo[ "Call-ID" ],
					*szBranch = tFileInfo[ "Via" ];

	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}
	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
		return;
	}

	char* szContext = NULL;
	long dwAppID = -1;

	const char	*szSessionID = tFileInfo2[ "SessionID" ],
				*szEufGuid   = tFileInfo2[ "EUF-GUID" ];
	{	const char* p = tFileInfo2[ "AppID" ];
		if ( p )
			dwAppID = atol( p );
		if ( dwAppID == 12 )
			dwAppID = 1;
	}
	{	const char* p = tFileInfo2[ "Context" ];
		if ( p ) {
			int cbLen = strlen( p );
			szContext = ( char* )alloca( cbLen+1 );

			NETLIBBASE64 nlb = { ( char* )p, cbLen, ( PBYTE )szContext, cbLen };
			MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	}	}

	if ( szContext == NULL && memcmp( msgbody, "Context: ", 9 ) == 0 ) {
		msgbody += 9;
		int cbLen = strlen( msgbody );
		if ( cbLen > 252 )
			cbLen = 252;
		szContext = ( char* )alloca( cbLen+1 );

		NETLIBBASE64 nlb = { ( char* )msgbody, cbLen, ( PBYTE )szContext, cbLen };
		MSN_CallService( MS_NETLIB_BASE64DECODE, 0, LPARAM( &nlb ));
	}

	if ( szSessionID == NULL || dwAppID == -1 || szEufGuid == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: SessionID='%s', AppID=%ld, Branch='%s'", szSessionID, dwAppID, szEufGuid );
		return;
	}

	srand( time( NULL ));

	filetransfer* ft = new filetransfer();
	ft->p2p_appID = dwAppID;
	ft->p2p_acksessid = 0x024F0000 + rand();
	ft->p2p_sessionid = strtoul( szSessionID, NULL, 10 );
	ft->p2p_msgid = ft->p2p_acksessid + 5;
	ft->p2p_ackID = ft->p2p_appID * 1000;
	replaceStr( ft->p2p_callID, szCallID );
	replaceStr( ft->p2p_branch, szBranch );
	ft->p2p_dest = strdup( szContactEmail );
	ft->mThreadId = info->mUniqueID;
	ft->mOwnsThread = info->mMessageCount == 0;

	p2p_sendAck( ft, info, hdrdata );

	if ( dwAppID == 1 && !strcmp( szEufGuid, "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}" )) {
		char szFileName[ MAX_PATH ];
		MSN_GetAvatarFileName( NULL, szFileName, sizeof( szFileName ));
		ft->fileId = _open( szFileName, O_RDONLY | _O_BINARY, _S_IREAD );
		if ( ft->fileId == -1 ) {
			p2p_sendStatus( ft, info, 603 );
			MSN_DebugLog( "Unable to open avatar file '%s', error %d", szFileName, errno );
			delete ft;
			return;
		}
		MSN_DebugLog( "My avatar file opened for %s as %08p::%d", szContactEmail, ft, ft->fileId );
		ft->std.totalBytes = ft->std.currentFileSize = filelength( ft->fileId );
		ft->std.sending = true;

		ft->p2p_msgid -= 3;

		//---- send 200 OK Message
		p2p_sendStatus( ft, info, 200 );
		p2p_registerSession( ft );
		return;
	}

	if ( dwAppID == 2 && !strcmp( szEufGuid, "{5D3E02AB-6190-11D3-BBBB-00C04F795683}" )) {
		WCHAR* wszFileName = ( WCHAR* )&szContext[ 20 ];
		{	for ( WCHAR* p = wszFileName; *p != 0; p++ )
			{	switch( *p ) {
				case ':': case '?': case '/': case '\\': case '*':
					*p = '_';
		}	}	}

		#if defined( _UNICODE ) 
			ft->wszFileName = _wcsdup( wszFileName );
		#endif

		char szFileName[ MAX_PATH ];
		char cDefaultChar = '_';
		WideCharToMultiByte( CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, 
			wszFileName, -1, szFileName, MAX_PATH, &cDefaultChar, 0 );
		MSN_DebugLog( "File name: '%s'", szFileName );

		ft->std.hContact = info->mJoinedContacts[0];
		replaceStr( ft->std.currentFile, szFileName );
		ft->std.totalBytes =	ft->std.currentFileSize = *( long* )&szContext[ 8 ];
		ft->std.totalFiles = 1;

		p2p_registerSession( ft );

		if ( !ft->mIsFirst ) {
			filetransfer* parentFt = p2p_getFirstSession( ft->std.hContact );
			if ( parentFt != NULL )
				ft->p2p_acksessid = parentFt->p2p_acksessid;
		}

		ft->p2p_msgid -= 3;

		int tFileNameLen = strlen( ft->std.currentFile );
		char tComment[ 40 ];
		int tCommentLen = mir_snprintf( tComment, sizeof( tComment ), "%ld bytes", ft->std.currentFileSize );
		char* szBlob = ( char* )alloca( sizeof( DWORD ) + tFileNameLen + tCommentLen + 2 );
		*( PDWORD )szBlob = ( DWORD )ft;
		strcpy( szBlob + sizeof( DWORD ), ft->std.currentFile );
		strcpy( szBlob + sizeof( DWORD ) + tFileNameLen + 1, tComment );

		PROTORECVEVENT pre;
		pre.flags = 0;
		pre.timestamp = ( DWORD )time( NULL );
		pre.szMessage = ( char* )szBlob;
		pre.lParam = ( LPARAM )( char* )"";

		CCSDATA ccs;
		ccs.hContact = info->mJoinedContacts[0];
		ccs.szProtoService = PSR_FILE;
		ccs.wParam = 0;
		ccs.lParam = ( LPARAM )&pre;
		MSN_CallService( MS_PROTO_CHAINRECV, 0, ( LPARAM )&ccs );
		return;
	}

	if ( dwAppID == 4 ) {
		if ( !strcmp( szEufGuid, "{4BD96FC0-AB17-4425-A14A-439185962DC8}" )) {
			MSN_ShowPopup( MSN_GetContactName( info->mJoinedContacts[0] ), 
				MSN_Translate( "Contact tried to send its webcam data (currently not supported)" ), MSN_ALLOW_MSGBOX );
			return;
		}
		if ( !strcmp( szEufGuid, "{1C9AA97E-9C05-4583-A3BD-908A196F1E92}" )) {
			MSN_ShowPopup( MSN_GetContactName( info->mJoinedContacts[0] ), 
				MSN_Translate( "Contact tried to view our webcam data (currently not supported)" ), MSN_ALLOW_MSGBOX );
			return;
	}	}

	delete ft;
	MSN_DebugLog( "Invalid or unknown AppID/EUF-GUID combination: %ld/%s", dwAppID, szEufGuid );
}

static void sttInitDirectTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2 )
{
	const char	*szCallID = tFileInfo[ "Call-ID" ],
					*szBranch = tFileInfo[ "Via" ];

	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}
	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', Branch='%s', SessionID=%s", szCallID, szBranch );
		return;
	}

	filetransfer* ft = p2p_getSessionByCallID( szCallID );
	if ( ft == NULL )
		return;

	++ft->p2p_msgid;
	p2p_sendAck( ft, info, hdrdata );

	replaceStr( ft->p2p_callID, szCallID );
	replaceStr( ft->p2p_branch, szBranch );
	ft->p2p_acksessid = 0x024B0000 + rand();

	const char	*szConnType = tFileInfo2[ "Conn-Type" ],
					*szUPnPNat = tFileInfo2[ "UPnPNat" ],
					*szNetID = tFileInfo2[ "NetID" ],
					*szICF = tFileInfo2[ "ICF" ];

	if ( szConnType == NULL || szUPnPNat == NULL || szICF == NULL || szNetID == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: ConnType='%s', UPnPNat='%s', ICF='%s', NetID='%s'",
			szConnType, szUPnPNat, szICF, szNetID );
		return;
	}

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);

	bool bUseDirect = false, bActAsServer = false;
	if ( atol( szNetID ) == 0 ) {
		if ( !strcmp( szConnType, "Direct-Connect" ) || !strcmp( szConnType, "Firewall" ))
			bUseDirect = true;
	}

	if ( MSN_GetByte( "NLSpecifyIncomingPorts", 0 )) {
		MSN_DebugLog( "My machine can accept incoming connections" );
		bUseDirect = bActAsServer = true;
	}

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "1 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );
	if ( bUseDirect )
		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	else
		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );

	char szBody[ 512 ];
	int  cbBodyLen = 0;
	if ( bActAsServer )
		cbBodyLen = sttCreateListener( info, ft, ( pThreadFunc )p2p_filePassiveThread, szBody, sizeof( szBody ));

	if ( !cbBodyLen )
		cbBodyLen = mir_snprintf( szBody, sizeof( szBody ),
			"Bridge: TCPv1\r\n"
			"Listening: false\r\n"
			"Nonce: %s\r\n\r\n%c", sttVoidNonce, 0 );

	ft->p2p_msgid -= 2;
	p2p_sendSlp( info, ft, tResult, 200, szBody, cbBodyLen );
	++ft->p2p_msgid;
}

static void sttInitDirectTransfer2(
	P2P_Header*  hdrdata,
	ThreadData*  info,
	MimeHeaders& tFileInfo,
	MimeHeaders& tFileInfo2 )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	const char	*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ],
				*szInternalPort = tFileInfo2[ "IPv4Internal-Port" ],
				*szExternalAddress = tFileInfo2[ "IPv4External-Addrs" ],
				*szExternalPort = tFileInfo2[ "IPv4External-Port" ],
				*szNonce = tFileInfo2[ "Nonce" ],
				*szListening = tFileInfo2[ "Listening" ];

	if ( szNonce == NULL || szListening == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: Listening='%s', Nonce=%s", szNonce, szListening );
		return;
	}

	MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_INITIALISING, ft, 0);
	p2p_sendAck( ft, info, hdrdata );

	if ( !strcmp( szListening, "true" ) && strcmp( szNonce, sttVoidNonce )) {
		ThreadData* newThread = new ThreadData;
		newThread->mType = SERVER_P2P_DIRECT;
		newThread->mP2pSession = ft;
		newThread->mParentThread = info;
		strncpy( newThread->mCookie, szNonce, sizeof( newThread->mCookie ));
		mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szInternalAddress, szInternalPort );
		newThread->startThread(( pThreadFunc )p2p_fileActiveThread );
	}
	else p2p_sendStatus( ft, info, 603 );
}

static void sttAcceptTransfer(
	P2P_Header*		hdrdata,
	ThreadData*		info,
	MimeHeaders&	tFileInfo,
	MimeHeaders&	tFileInfo2 )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	ft->mThreadId = info->mUniqueID;

    ++ft->p2p_msgid;
	p2p_sendAck( ft, info, hdrdata );

	const char *szCallID = tFileInfo[ "Call-ID" ], *szBranch = tFileInfo[ "Via" ];
	if ( szBranch != NULL ) {
		szBranch = strstr( szBranch, "branch=" );
		if ( szBranch != NULL )
			szBranch += 7;
	}

	if ( szCallID == NULL || szBranch == NULL ) {
		MSN_DebugLog( "Ignoring invalid invitation: CallID='%s', szBranch='%s'", szCallID, szBranch );
LBL_Close:
		p2p_sendBye( info, ft );
		return;
	}

	if ( !ft->std.sending ) {
		replaceStr( ft->p2p_branch, szBranch );
		replaceStr( ft->p2p_callID, szCallID );
		return;
	}

	const char* szOldContentType = tFileInfo[ "Content-Type" ];
	if ( szOldContentType == NULL )
		goto LBL_Close;

	bool bAllowIncoming = ( MSN_GetByte( "NLSpecifyIncomingPorts", 0 ) != 0 );

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "0 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );

	char* szBody = ( char* )alloca( 1024 );
	int   cbBody = 0;
	if ( !strcmp( szOldContentType, "application/x-msnmsgr-sessionreqbody" )) {
		tResult.addString( "Content-Type", "application/x-msnmsgr-transreqbody" );
		cbBody = mir_snprintf( szBody, 1024,
			"Bridges: TCPv1\r\nNetID: 0\r\nConn-Type: %s\r\nUPnPNat: false\r\nICF: false\r\n\r\n%c",
//            "Nonce: %s\r\nSessionID: %lu\r\nSChannelState: 0\r\n\r\n%c",
			( bAllowIncoming ) ? "Direct-Connect" : "Unknown-Connect", 0 );
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transrespbody" )) {
		const char	*szListening       = tFileInfo2[ "Listening" ],
						*szNonce           = tFileInfo2[ "Nonce" ],
						*szExternalAddress = tFileInfo2[ "IPv4External-Addrs" ],
						*szExternalPort    = tFileInfo2[ "IPv4External-Port"  ],
						*szInternalAddress = tFileInfo2[ "IPv4Internal-Addrs" ],
						*szInternalPort    = tFileInfo2[ "IPv4Internal-Port"  ];
		if ( szListening == NULL || szNonce == NULL ) {
			MSN_DebugLog( "Invalid data packet, exiting..." );
			goto LBL_Close;
		}

		// another side reported that it will be a server.
		if ( !strcmp( szListening, "true" ) && strcmp( szNonce, sttVoidNonce )) {
			bool extOk = szExternalAddress != NULL && szExternalPort != NULL;
			bool intOk = szInternalAddress != NULL && szInternalPort != NULL;

			ThreadData* newThread = new ThreadData;

			char ipaddr[256] = "";
			MSN_GetMyHostAsString( ipaddr, sizeof( ipaddr ));

			if ( extOk && ( strcmp( szExternalAddress, ipaddr ) || !intOk ))
				mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szExternalAddress, szExternalPort );
			else if ( intOk )
				mir_snprintf( newThread->mServer, sizeof( newThread->mServer ), "%s:%s", szInternalAddress, szInternalPort );
			else {
				MSN_DebugLog( "Invalid data packet, exiting..." );
				delete newThread;
				goto LBL_Close;
			}

			newThread->mType = SERVER_P2P_DIRECT;
			newThread->mP2pSession = ft;
			newThread->mParentThread = info;
			strncpy( newThread->mCookie, szNonce, sizeof( newThread->mCookie ));
			newThread->startThread(( pThreadFunc )p2p_fileActiveThread );
			return;
		}

		// can we be a server?
		if ( bAllowIncoming )
			cbBody = sttCreateListener( info, ft, ( pThreadFunc )p2p_filePassiveThread, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
			p2p_sendFeedStart( ft, info );
			return;
		}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else if ( !strcmp( szOldContentType, "application/x-msnmsgr-transreqbody" )) {
		// can we be a server?
		if ( bAllowIncoming )
			cbBody = sttCreateListener( info, ft, ( pThreadFunc )p2p_filePassiveThread, szBody, 1024 );

		// no, send a file via server
		if ( cbBody == 0 ) {
			p2p_sendFeedStart( ft, info );
			return;
		}

		tResult.addString( "Content-Type", "application/x-msnmsgr-transrespbody" );
	}
	else goto LBL_Close;

	ft->p2p_msgid -= 2;
	p2p_sendSlp( info, ft, tResult, -2, szBody, cbBody );
	++ft->p2p_msgid;
}

static void sttCloseTransfer( P2P_Header* hdrdata, ThreadData* info, MimeHeaders& tFileInfo )
{
	filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
	if ( ft == NULL )
		return;

	p2p_sendAck( ft, info, hdrdata );
	p2p_unregisterSession( ft );
}

void __stdcall p2p_processMsg( ThreadData* info, const char* msgbody )
{
	P2P_Header* hdrdata = ( P2P_Header* )msgbody; msgbody += sizeof( P2P_Header );
	sttLogHeader( hdrdata );

	//---- if we got a message
	if ( hdrdata->mFlags == 0 )
	{
		int iMsgType = 0;
		if ( !memcmp( msgbody, "INVITE MSNMSGR:", 15 ))
			iMsgType = 1;
		else if ( !memcmp( msgbody, "MSNSLP/1.0 200 ", 15 ))
			iMsgType = 2;
		else if ( !memcmp( msgbody, "BYE MSNMSGR:", 12 ))
			iMsgType = 3;
		else if ( !memcmp( msgbody, "MSNSLP/1.0 603 ", 15 ))
			iMsgType = 4;

		if ( iMsgType ) {
			const char* peol = strstr( msgbody, "\r\n" );
			if ( peol != NULL )
				msgbody = peol+2;

			MimeHeaders tFileInfo, tFileInfo2;
			msgbody = tFileInfo.readFromBuffer( msgbody );
			msgbody = tFileInfo2.readFromBuffer( msgbody );

			const char* szContentType = tFileInfo[ "Content-Type" ];
			if ( szContentType == NULL ) {
				MSN_DebugLog( "Invalid or missing Content-Type field, exiting" );
				return;
			}

			switch( iMsgType ) {
			case 1:
				if ( info->mType == SERVER_SWITCHBOARD) {
					if ( !strcmp( szContentType, "application/x-msnmsgr-sessionreqbody" ))
						sttInitFileTransfer( hdrdata, info, tFileInfo, tFileInfo2, msgbody );
					else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transreqbody" ))
						sttInitDirectTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
					else if ( iMsgType == 1 && !strcmp( szContentType, "application/x-msnmsgr-transrespbody" ))
						sttInitDirectTransfer2( hdrdata, info, tFileInfo, tFileInfo2 );
				}
				break;

			case 2:
				if ( info->mType == SERVER_SWITCHBOARD)
					sttAcceptTransfer( hdrdata, info, tFileInfo, tFileInfo2 );
				break;

			case 3:
				if ( !strcmp( szContentType, "application/x-msnmsgr-sessionclosebody" )) 
				{
					filetransfer* ft = p2p_getSessionByCallID( tFileInfo[ "Call-ID" ] );
					if ( ft != NULL )
					{
						p2p_sendAck( ft, info, hdrdata );
						if ( ft->std.currentFileProgress < ft->std.currentFileSize )
						{
							p2p_sendEndSession(info, ft);
							ft->bCanceled = true;
						}
						else if ( !ft->std.sending ) 
							ft->complete();

						p2p_unregisterSession( ft );
				}	}
				break;

			case 4:
				sttCloseTransfer( hdrdata, info, tFileInfo );
				break;
			}
			return;
	}	}

	//---- receiving ack -----------
	if ( hdrdata->mFlags == 0x02 ) {
		filetransfer* ft = p2p_getSessionByID( hdrdata->mSessionID );
		if ( ft == NULL )
			if ((ft = p2p_getSessionByMsgID( hdrdata->mAckSessionID )) == NULL )
				return;

		if ( hdrdata->mAckSessionID == ft->p2p_sendmsgid )
		{
			if ( ft->p2p_appID == 2 )
			{
				ft->complete();
				p2p_sendBye( info, ft );
			}
			return;
		}

		if ( hdrdata->mAckSessionID == ft->p2p_byemsgid )
		{
			if ( ft->p2p_appID == 1 ) {
				sttSavePicture2disk( info, ft );

				char tContext[ 256 ];
				if ( !MSN_GetStaticString( "PictContext", ft->std.hContact, tContext, sizeof( tContext )))
					MSN_SetString( ft->std.hContact, "PictSavedContext", tContext );
			}

			p2p_unregisterSession( ft );
			if ( ft->mOwnsThread )
				info->sendPacket( "OUT", NULL );
			return;
		}

		switch( ft->p2p_ackID ) {
		case 1000:
			{
				//---- send Data Preparation Message
				char* buf = ( char* )alloca( 2000 + 3*strlen( ft->p2p_dest ));
				char* p = buf + sprintf( buf, sttP2Pheader, ft->p2p_dest );
				P2P_Header* tHdr = ( P2P_Header* )p; p += sizeof( P2P_Header );
				memset( tHdr, 0, sizeof( P2P_Header ));
				tHdr->mSessionID = ft->p2p_sessionid;
				tHdr->mID = ++ft->p2p_msgid;
				tHdr->mTotalSize = tHdr->mPacketLen = 4;
				tHdr->mAckSessionID = ft->p2p_acksessid;
				*( long* )p = 0; p += sizeof( long );
				*( long* )p = htonl(ft->p2p_appID); p += sizeof( long );
				info->sendRawMessage( 'D', buf, int( p - buf ));
			}
			break;

		case 1001:
			//---- send Data Messages
			p2p_sendFeedStart( ft, info );
			break;
		}

		ft->p2p_ackID++;
		return;
	}

	filetransfer* ft = p2p_getSessionByID( hdrdata->mSessionID );
	if ( ft == NULL )
		return;

	if ( hdrdata->mFlags == 0 ) {
		//---- accept the data preparation message ------
		long* pLongs = ( long* )msgbody;
		if ( pLongs[0] == 0 && pLongs[1] == 0x01000000 && hdrdata->mPacketLen == 4 ) {
			p2p_sendAck( ft, info, hdrdata );
			return;
	}	}

	//---- receiving data -----------
	if ( hdrdata->mFlags == 0x01000030 || hdrdata->mFlags == 0x20 ) {

		if ( hdrdata->mOffset + hdrdata->mPacketLen > hdrdata->mTotalSize )
			hdrdata->mPacketLen = DWORD( hdrdata->mTotalSize - hdrdata->mOffset );

		ft->p2p_sendmsgid = hdrdata->mID;
		ft->std.totalBytes = ft->std.currentFileSize = ( long )hdrdata->mTotalSize;

		if ( ft->inmemTransfer )
			memcpy( ft->fileBuffer + hdrdata->mOffset, msgbody, hdrdata->mPacketLen );
		else {
			::lseek( ft->fileId, long( hdrdata->mOffset ), SEEK_SET );
			::_write( ft->fileId, msgbody, hdrdata->mPacketLen );
		}

		ft->std.totalProgress += hdrdata->mPacketLen;
		ft->std.currentFileProgress += hdrdata->mPacketLen;

		if ( ft->p2p_appID == 2 )
			MSN_SendBroadcast( ft->std.hContact, ACKTYPE_FILE, ACKRESULT_DATA, ft, ( LPARAM )&ft->std );

		//---- send an ack: body was transferred correctly
		MSN_DebugLog( "Transferred %lu bytes out of %lu", ft->std.currentFileProgress, hdrdata->mTotalSize );

		if ( ft->std.currentFileProgress == hdrdata->mTotalSize ) {
			p2p_sendAck( ft, info, hdrdata );
			if ( ft->p2p_appID == 2 )
				ft->complete();
			else 
				p2p_sendBye( info, ft );
	}	}

	if ( hdrdata->mFlags == 0x40 || hdrdata->mFlags == 0x80 ) {
		if ( ft->mOwnsThread )
			info->sendPacket( "OUT", NULL );
		p2p_unregisterSession( ft );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
// p2p_invite - invite another side to transfer an avatar

struct HFileContext
{
	DWORD len, dw1, dwSize, dw2, dw3;
	WCHAR wszFileName[ MAX_PATH ];
};

void __stdcall p2p_invite( HANDLE hContact, int iAppID, filetransfer* ft )
{
	const char* szAppID;
	switch( iAppID ) {
	case MSN_APPID_FILE:	szAppID = "{5D3E02AB-6190-11D3-BBBB-00C04F795683}";	break;
	case MSN_APPID_AVATAR:	szAppID = "{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}";	break;
	default:
		return;
	}

	char szEmail[ MSN_MAX_EMAIL_LEN ];
	MSN_GetStaticString( "e-mail", hContact, szEmail, sizeof( szEmail ));

	ThreadData *thread = MSN_GetThreadByContact( hContact );

	srand( (unsigned)time( NULL ) );
	long sessionID = rand();

	if ( ft == NULL ) {
		ft = new filetransfer();
		ft->std.hContact = hContact;
	}
	ft->p2p_appID = iAppID;
	ft->p2p_msgid = rand();
	ft->p2p_acksessid = rand();
	ft->p2p_sessionid = sessionID;
	ft->p2p_dest = strdup( szEmail );
	ft->p2p_branch = getNewUuid();
	ft->p2p_callID = getNewUuid();
	p2p_registerSession( ft );

	BYTE* pContext;
	int   cbContext;

	if ( iAppID == MSN_APPID_AVATAR ) {
		ft->inmemTransfer = true;
		ft->fileBuffer = NULL;
		ft->std.sending = false;

		char tBuffer[ 256 ] = "";
		MSN_GetStaticString( "PictContext", hContact, tBuffer, sizeof( tBuffer ));

		char* p = strstr( tBuffer, "Size=\"" );
		if ( p != NULL )
			ft->std.totalBytes = ft->std.currentFileSize = atol( p+6 );

		if (ft->create() == -1) return;

		pContext = ( BYTE* )tBuffer;
		cbContext = strlen( tBuffer )+1;
	}
	else {
		HFileContext ctx;
		memset( &ctx, 0, sizeof( ctx ));
		ctx.len = sizeof( ctx );
		ctx.dwSize = ft->std.totalBytes;
		if ( ft->wszFileName != NULL )
			wcsncpy( ctx.wszFileName, ft->wszFileName, sizeof( ctx.wszFileName ));
		else {
			char* pszFiles = strrchr( ft->std.currentFile, '\\' );
			if ( pszFiles )
				pszFiles++;
			else
				pszFiles = ft->std.currentFile;

			MultiByteToWideChar( CP_ACP, 0, pszFiles, -1, ctx.wszFileName, sizeof( ctx.wszFileName ));
		}

		pContext = ( BYTE* )&ctx;
		cbContext = sizeof( ctx );
	}

	char* body = ( char* )alloca( 2000 );
	int tBytes = mir_snprintf( body, 2000,
		"EUF-GUID: %s\r\n"
		"SessionID: %lu\r\n"
		"AppID: %d\r\n"
		"Context: ",
		szAppID, sessionID, iAppID );

	NETLIBBASE64 nlb = { body+tBytes, 2000-tBytes, pContext, cbContext };
	MSN_CallService( MS_NETLIB_BASE64ENCODE, 0, LPARAM( &nlb ));
	strcat( body, "\r\n\r\n" );
	int cbBody = strlen( body );
	body[ cbBody++ ] = 0;

	MimeHeaders tResult(20);
	tResult.addString( "CSeq", "0 " );
	tResult.addString( "Call-ID", ft->p2p_callID );
	tResult.addLong( "Max-Forwards", 0 );
	tResult.addString( "Content-Type", "application/x-msnmsgr-sessionreqbody" );

	p2p_sendSlp( MSN_GetThreadByContact( hContact ), ft, tResult, -2, body, cbBody );
}
