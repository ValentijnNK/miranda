#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 1
#define _CRT_SECURE_NO_DEPRECATE 1
#include "connection.h"
CRITICAL_SECTION statusMutex;
CRITICAL_SECTION connectionMutex;
HANDLE aim_connect(char* server)
{
	char* server_dup=strldup(server,strlen(server));
    NETLIBOPENCONNECTION ncon = { 0 };
	char* host=strtok(server_dup,":");
	char* port=strtok(NULL,":");
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = host;
    ncon.wPort =atoi(port);
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlib, (LPARAM) & ncon);
	delete[] server_dup;
	load_extra_icons();
	return con;
}
HANDLE aim_peer_connect(char* ip,unsigned short port)
{
    NETLIBOPENCONNECTION ncon = { 0 };
    ncon.cbSize = sizeof(ncon);
    ncon.szHost = ip;
    ncon.wPort =port;
    HANDLE con = (HANDLE) CallService(MS_NETLIB_OPENCONNECTION, (WPARAM) conn.hNetlibPeer, (LPARAM) & ncon);
	return con;
}
void __cdecl aim_connection_authorization()
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	DBVARIANT dbv;
	int recvResult=0;
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_PW, &dbv))
	{
        CallService(MS_DB_CRYPT_DECODESTRING, strlen(dbv.pszVal) + 1, (LPARAM) dbv.pszVal);
        conn.password = strldup(dbv.pszVal,strlen(dbv.pszVal));
        DBFreeVariant(&dbv);
	}
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	if (!DBGetContactSetting(NULL, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
        conn.username = strldup(dbv.pszVal,strlen(dbv.pszVal));
        DBFreeVariant(&dbv);
    }
	else
	{
		LeaveCriticalSection(&connectionMutex);
		return;
	}
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	conn.hServerPacketRecver=NULL;
	conn.hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hServerConn, 2048 * 4);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	while(1)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0) {
                break;
            }
        if (recvResult == SOCKET_ERROR) {
                break;
            }
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],packetRecv.bytesAvailable-packetRecv.bytesUsed);
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
				if(flap.cmp(0x01))
				{
					if(aim_send_connection_packet(flap.val())==0)//cookie challenge
					{
						aim_authkey_request();//md5 authkey request
					}
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0017))
					{
						snac_md5_authkey(snac);
						if(snac_authorization_reply(snac)==1)
						{
							delete[] conn.username;
							delete[] conn.password;
							Netlib_CloseHandle(conn.hServerPacketRecver);
							LeaveCriticalSection(&connectionMutex);
							return;
						}
					}
				}
				if(flap.cmp(0x04))
				{
					delete[] conn.username;
					delete[] conn.password;
					conn.state=0;
					Netlib_CloseHandle(conn.hServerPacketRecver);
					broadcast_status(ID_STATUS_OFFLINE);
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	delete[] conn.username;
	delete[] conn.password;
	conn.state=0;
	Netlib_CloseHandle(conn.hServerPacketRecver);
	LeaveCriticalSection(&connectionMutex);
	broadcast_status(ID_STATUS_OFFLINE);
}
void __cdecl aim_protocol_negotiation()
{
	EnterCriticalSection(&connectionMutex);
	NETLIBPACKETRECVER packetRecv;
	int recvResult=0;
	ZeroMemory(&packetRecv, sizeof(packetRecv));
	conn.hServerPacketRecver=NULL;
	conn.hServerPacketRecver = (HANDLE) CallService(MS_NETLIB_CREATEPACKETRECVER, (WPARAM)conn.hServerConn, 2048 * 8);
	packetRecv.cbSize = sizeof(packetRecv);
	packetRecv.dwTimeout = INFINITE;
	while(1)
	{
		recvResult = CallService(MS_NETLIB_GETMOREPACKETS, (WPARAM) conn.hServerPacketRecver, (LPARAM) & packetRecv);
		if (recvResult == 0)
		{
                break;
		}
        if (recvResult == SOCKET_ERROR)
		{
                break;
		}
		if(recvResult>0)
		{
			unsigned short flap_length=0;
			for(;packetRecv.bytesUsed<packetRecv.bytesAvailable;packetRecv.bytesUsed=flap_length)
			{
				if(!packetRecv.buffer)
					break;
				FLAP flap((char*)&packetRecv.buffer[packetRecv.bytesUsed],packetRecv.bytesAvailable-packetRecv.bytesUsed);
				if(!flap.len())
					break;
				flap_length+=FLAP_SIZE+flap.len();
  				if(flap.cmp(0x01))
				{
					aim_send_cookie(COOKIE_LENGTH,COOKIE);//cookie challenge
					delete[] COOKIE;
					COOKIE=NULL;
					COOKIE_LENGTH=0;
				}
				else if(flap.cmp(0x02))
				{
					SNAC snac(flap.val(),flap.snaclen());
					if(snac.cmp(0x0001))
					{
						snac_supported_families(snac);
						snac_supported_family_versions(snac);
						snac_rate_limitations(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0002))
					{
						snac_received_info(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0003))
					{
						snac_user_online(snac);
						snac_user_offline(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0004))
					{
						snac_icbm_limitations(snac);
						snac_message_accepted(snac);
						snac_received_message(snac);
						snac_typing_notification(snac);
						snac_error(snac);
					}
					else if(snac.cmp(0x0013))
					{
						snac_contact_list(snac);
						snac_list_modification_ack(snac);
						snac_error(snac);
					}
				}
				else if(flap.cmp(0x04))
				{
					offline_contacts();
					conn.state=0;
					Netlib_CloseHandle(conn.hServerPacketRecver);
					conn.buddy_list_received=0;
					broadcast_status(ID_STATUS_OFFLINE);
					LeaveCriticalSection(&connectionMutex);
					return;
				}
			}
		}
	}
	offline_contacts();
	conn.state=0;
	Netlib_CloseHandle(conn.hServerPacketRecver);
	conn.buddy_list_received=0;
	broadcast_status(ID_STATUS_OFFLINE);
	LeaveCriticalSection(&connectionMutex);
}
