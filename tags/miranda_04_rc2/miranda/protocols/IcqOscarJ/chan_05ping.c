// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern HANDLE ghServerNetlibUser;
extern HANDLE hDirectBoundPort;



void handlePingChannel(unsigned char* buf, WORD datalen)
{

	Netlib_Logf(ghServerNetlibUser, "Warning: Ignoring server packet on PING channel");

}


// Todo: This need to be redone
void __cdecl icq_keepAliveThread(void* fa)
{

	icq_packet packet;
	NETLIBSELECT nls = {0};


	nls.cbSize = sizeof(nls);
	nls.dwTimeout = 57000;
	nls.hExceptConns[0] = hDirectBoundPort;

	Netlib_Logf(ghServerNetlibUser, "Keep alive thread starting.");

	for(;;)
	{

		if (CallService(MS_NETLIB_SELECT, 0, (LPARAM)&nls) != 0)
			break;

		// Send a keep alive packet to server
		packet.wLen = 0;
		write_flap(&packet, ICQ_PING_CHAN);
		sendServPacket(&packet);

	}

	Netlib_Logf(ghServerNetlibUser, "Keep alive thread shutting down.");

	return;

}
