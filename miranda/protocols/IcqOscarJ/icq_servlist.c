// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004,2005 Joe Kucera
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
//  Functions that handles list of used server IDs, sends low-level packets for SSI information
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



extern BOOL bIsSyncingCL;

static HANDLE hHookSettingChanged = NULL;
static HANDLE hHookContactDeleted = NULL;
static DWORD* pwIDList = NULL;
static int nIDListCount = 0;
static int nIDListSize = 0;


//typedef void (*PENDINGCALLBACK)(const char *szGroupPath, GROUPADDCALLBACK ofCallBack, servlistcookie* ack);

// cookie struct for pending records
typedef struct ssipendingitem_t
{
  HANDLE hContact;
  char* szGroupPath;
  GROUPADDCALLBACK ofCallback;
  servlistcookie* pCookie;
} ssipendingitem;

static CRITICAL_SECTION servlistMutex;
static int nPendingCount = 0;
static int nPendingSize = 0;
static ssipendingitem** pdwPendingList = NULL;
static int nJustAddedCount = 0;
static int nJustAddedSize = 0;
static HANDLE* pdwJustAddedList = NULL;
static WORD* pwGroupRenameList = NULL;
static int nGroupRenameCount = 0;
static int nGroupRenameSize = 0;


// Add running group rename operation
void AddGroupRename(WORD wGroupID)
{
  EnterCriticalSection(&servlistMutex);
	if (nGroupRenameCount >= nGroupRenameSize)
	{
		nGroupRenameSize += 10;
		pwGroupRenameList = (WORD*)realloc(pwGroupRenameList, nGroupRenameSize * sizeof(WORD));
	}

	pwGroupRenameList[nGroupRenameCount] = wGroupID;
	nGroupRenameCount++;	
  LeaveCriticalSection(&servlistMutex);
}


// Remove running group rename operation
void RemoveGroupRename(WORD wGroupID)
{
  int i, j;

  EnterCriticalSection(&servlistMutex);
  if (pwGroupRenameList)
  {
    for (i = 0; i<nGroupRenameCount; i++)
    {
      if (pwGroupRenameList[i] == wGroupID)
      { // we found it, so remove
        for (j = i+1; j<nGroupRenameCount; j++)
        {
          pwGroupRenameList[j-1] = pwGroupRenameList[j];
        }
        nGroupRenameCount--;
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);
}


// Returns true if dwID is reserved
BOOL IsGroupRenamed(WORD wGroupID)
{
  int i;

  EnterCriticalSection(&servlistMutex);
  if (pwGroupRenameList)
  {
    for (i = 0; i<nGroupRenameCount; i++)
    {
      if (pwGroupRenameList[i] == wGroupID)
      {
        LeaveCriticalSection(&servlistMutex);
        return TRUE;
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);

  return FALSE;
}


void FlushGroupRenames()
{
  EnterCriticalSection(&servlistMutex);
  SAFE_FREE(&pwGroupRenameList);
  nGroupRenameCount = 0;
  nGroupRenameSize = 0;
  LeaveCriticalSection(&servlistMutex);
}


// used for adding new contacts to list - sync with visible items
void AddJustAddedContact(HANDLE hContact)
{
  EnterCriticalSection(&servlistMutex);
	if (nJustAddedCount >= nJustAddedSize)
	{
		nJustAddedSize += 10;
		pdwJustAddedList = (HANDLE*)realloc(pdwJustAddedList, nJustAddedSize * sizeof(HANDLE));
	}

	pdwJustAddedList[nJustAddedCount] = hContact;
	nJustAddedCount++;	
  LeaveCriticalSection(&servlistMutex);
}


// was the contact added during this serv-list load
BOOL IsContactJustAdded(HANDLE hContact)
{
  int i;

  EnterCriticalSection(&servlistMutex);
  if (pdwJustAddedList)
  {
    for (i = 0; i<nJustAddedCount; i++)
    {
      if (pdwJustAddedList[i] == hContact)
      {
        LeaveCriticalSection(&servlistMutex);
        return TRUE;
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);

  return FALSE;
}


void FlushJustAddedContacts()
{
  EnterCriticalSection(&servlistMutex);
  SAFE_FREE((void*)&pdwJustAddedList);
  nJustAddedSize = 0;
  nJustAddedCount = 0;
  LeaveCriticalSection(&servlistMutex);
}



// Used for event-driven adding of contacts, before it is completed this is used
static BOOL AddPendingOperation(HANDLE hContact, const char* szGroup, servlistcookie* cookie, GROUPADDCALLBACK ofEvent)
{
  BOOL bRes = TRUE;
  ssipendingitem* pItem = NULL;
  int i;

  EnterCriticalSection(&servlistMutex);
  if (pdwPendingList)
  {
    for (i = 0; i<nPendingCount; i++)
    {
      if (pdwPendingList[i]->hContact == hContact)
      { // we need the last item for this contact
        pItem = pdwPendingList[i];
      }
    }
  }

  if (pItem) // we found a pending operation, so link our data
  {
    pItem->ofCallback = ofEvent;
    pItem->pCookie = cookie;
    pItem->szGroupPath = szGroup?_strdup(szGroup):NULL; // we need to duplicate the string
    bRes = FALSE;

    NetLog_Server("Operation postponed.");
  }
  
  if (nPendingCount >= nPendingSize) // add new
	{
		nPendingSize += 10;
		pdwPendingList = (ssipendingitem**)realloc(pdwPendingList, nPendingSize * sizeof(ssipendingitem*));
	}

	pdwPendingList[nPendingCount] = (ssipendingitem*)calloc(sizeof(ssipendingitem),1);
  pdwPendingList[nPendingCount]->hContact = hContact;

	nPendingCount++;
  LeaveCriticalSection(&servlistMutex);

  return bRes;
}



// Check if any pending operation is in progress
// If yes, get its data and remove it from queue
void RemovePendingOperation(HANDLE hContact, int nResult)
{
  int i, j;
  ssipendingitem* pItem = NULL;

  EnterCriticalSection(&servlistMutex);
  if (pdwPendingList)
  {
    for (i = 0; i<nPendingCount; i++)
    {
      if (pdwPendingList[i]->hContact == hContact)
      {
        pItem = pdwPendingList[i];
        for (j = i+1; j<nPendingCount; j++)
        {
          pdwPendingList[j-1] = pdwPendingList[j];
        }
        nPendingCount--;
        if (nResult) // we succeded, go on, resume operation
        {
          LeaveCriticalSection(&servlistMutex);

          if (pItem->ofCallback)
          {
            NetLog_Server("Resuming postponed operation.");

            makeGroupId(pItem->szGroupPath, pItem->ofCallback, pItem->pCookie);
          }
          else if ((int)pItem->pCookie == 1)
          {
            NetLog_Server("Resuming postponed rename.");

            renameServContact(hContact, pItem->szGroupPath);
          }

          SAFE_FREE(&pItem->szGroupPath); // free the string
          SAFE_FREE(&pItem);
          return;
        } // else remove all pending operations for this contact
        if ((pItem->pCookie) && ((int)pItem->pCookie != 1))
          SAFE_FREE(&pItem->pCookie->szGroupName); // do not leak nick name on error
        SAFE_FREE(&pItem->szGroupPath);
        SAFE_FREE(&pItem);
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);
  return;
}



// Remove All pending operations
void FlushPendingOperations()
{
  int i;

  EnterCriticalSection(&servlistMutex);

  for (i = 0; i<nPendingCount; i++)
  {
    SAFE_FREE(&(void*)pdwPendingList[i]);
  }
  SAFE_FREE(&(void*)pdwPendingList);
  nPendingCount = 0;
  nPendingSize = 0;

  LeaveCriticalSection(&servlistMutex);
}



// Add a server ID to the list of reserved IDs.
// To speed up the process, no checks is done, if
// you try to reserve an ID twice, it will be added again.
// You should call CheckServerID before reserving an ID.
void ReserveServerID(WORD wID, int bGroupId)
{
  EnterCriticalSection(&servlistMutex);
	if (nIDListCount >= nIDListSize)
	{
		nIDListSize += 100;
		pwIDList = (DWORD*)realloc(pwIDList, nIDListSize * sizeof(DWORD));
	}

	pwIDList[nIDListCount] = wID | bGroupId << 0x18;
	nIDListCount++;	
  LeaveCriticalSection(&servlistMutex);
}



// Remove a server ID from the list of reserved IDs.
// Used for deleting contacts and other modifications.
void FreeServerID(WORD wID, int bGroupId)
{
  int i, j;
  DWORD dwId = wID | bGroupId << 0x18;

  EnterCriticalSection(&servlistMutex);
  if (pwIDList)
  {
    for (i = 0; i<nIDListCount; i++)
    {
      if (pwIDList[i] == dwId)
      { // we found it, so remove
        for (j = i+1; j<nIDListCount; j++)
        {
          pwIDList[j-1] = pwIDList[j];
        }
        nIDListCount--;
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);
}


// Returns true if dwID is reserved
BOOL CheckServerID(WORD wID, unsigned int wCount)
{
  int i;

  EnterCriticalSection(&servlistMutex);
  if (pwIDList)
  {
    for (i = 0; i<nIDListCount; i++)
    {
      if (((pwIDList[i] & 0xFFFF) >= wID) && ((pwIDList[i] & 0xFFFF) <= wID + wCount))
      {
        LeaveCriticalSection(&servlistMutex);
        return TRUE;
      }
    }
  }
  LeaveCriticalSection(&servlistMutex);

  return FALSE;
}



void FlushServerIDs()
{
  EnterCriticalSection(&servlistMutex);
  SAFE_FREE(&pwIDList);
  nIDListCount = 0;
  nIDListSize = 0;
  LeaveCriticalSection(&servlistMutex);
}



static int GroupReserveIdsEnumProc(const char *szSetting,LPARAM lParam)
{ 
  if (szSetting && strlen(szSetting)<5)
  { // it is probably server group
    char val[MAX_PATH+2]; // dummy
    DBVARIANT dbv;
    DBCONTACTGETSETTING cgs;

    dbv.type = DBVT_ASCIIZ;
    dbv.pszVal = val;
    dbv.cchVal = MAX_PATH;

    cgs.szModule=(char*)lParam;
    cgs.szSetting=szSetting;
    cgs.pValue=&dbv;
    if(CallService(MS_DB_CONTACT_GETSETTINGSTATIC,(WPARAM)NULL,(LPARAM)&cgs))
      return 0;
    if(dbv.type!=DBVT_ASCIIZ)
    { // it is not a cached server-group name
      return 0;
    }
    ReserveServerID((WORD)strtoul(szSetting, NULL, 0x10), SSIT_GROUP);
  }
  return 0;
}



void ReserveServerGroups()
{
  DBCONTACTENUMSETTINGS dbces;

  char szModule[MAX_PATH+9];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");

  dbces.pfnEnumProc = &GroupReserveIdsEnumProc;
  dbces.szModule = szModule;
  dbces.lParam = (LPARAM)szModule;

  CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces);
}



// Load all known server IDs from DB to list
void LoadServerIDs()
{
  HANDLE hContact;
  WORD wSrvID;
  char* szProto;

  EnterCriticalSection(&servlistMutex);
  if (wSrvID = ICQGetContactSettingWord(NULL, "SrvAvatarID", 0))
    ReserveServerID(wSrvID, SSIT_ITEM);
  if (wSrvID = ICQGetContactSettingWord(NULL, "SrvVisibilityID", 0))
    ReserveServerID(wSrvID, SSIT_ITEM);

  ReserveServerGroups();
  
  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

  while (hContact)
  { // search all our contacts, reserve their server IDs
    szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
    if (szProto && !lstrcmp(szProto, gpszICQProtoName))
    {
      if (wSrvID = ICQGetContactSettingWord(hContact, "SrvContactId", 0))
        ReserveServerID(wSrvID, SSIT_ITEM);
      if (wSrvID = ICQGetContactSettingWord(hContact, "SrvDenyId", 0))
        ReserveServerID(wSrvID, SSIT_ITEM);
      if (wSrvID = ICQGetContactSettingWord(hContact, "SrvPermitId", 0))
        ReserveServerID(wSrvID, SSIT_ITEM);
      if (wSrvID = ICQGetContactSettingWord(hContact, "SrvIgnoreId", 0))
        ReserveServerID(wSrvID, SSIT_ITEM);
    }
    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }
  LeaveCriticalSection(&servlistMutex);

  return;
}



WORD GenerateServerId(int bGroupId)
{
  WORD wId;

  while (TRUE)
  {
    // Randomize a new ID
    // Max value is probably 0x7FFF, lowest value is probably 0x0001 (generated by Icq2Go)
    // We use range 0x1000-0x7FFF.
    wId = (WORD)RandRange(0x1000, 0x7FFF);

    if (!CheckServerID(wId, 0))
      break;
  }

  ReserveServerID(wId, bGroupId);

  return wId;
}



// Generate server ID with wCount IDs free after it, for sub-groups.
WORD GenerateServerIdPair(int bGroupId, int wCount)
{
  WORD wId;

  while (TRUE)
  {
    // Randomize a new ID
    // Max value is probably 0x7FFF, lowest value is probably 0x0001 (generated by Icq2Go)
    // We use range 0x1000-0x7FFF.
    wId = (WORD)RandRange(0x1000, 0x7FFF);

    if (!CheckServerID(wId, wCount))
      break;
	}
	
	ReserveServerID(wId, bGroupId);

	return wId;
}


/***********************************************
 *
 *  --- Low-level packet sending functions ---
 *
 */



DWORD icq_sendBuddy(DWORD dwCookie, WORD wAction, DWORD dwUin, char* szUID, WORD wGroupId, WORD wContactId, const char *szNick, const char*szNote, int authRequired, WORD wItemType)
{
  icq_packet packet;
  char szUin[10];
  int nUinLen;
  int nNickLen;
  int nNoteLen;
  char* szUtfNick = NULL;
  char* szUtfNote = NULL;
  WORD wTLVlen;

  // Prepare UIN
  if (dwUin)
  {
    _itoa(dwUin, szUin, 10);
    nUinLen = strlennull(szUin);
  }
  else 
    nUinLen = strlennull(szUID);

  if (!nUinLen)
  {
    NetLog_Server("Buddy upload failed (UID missing).");
    return 0;
  }

  // Prepare custom utf-8 nick name
  if (szNick && (strlen(szNick) > 0))
  {
    int nResult;

    nResult = utf8_encode(szNick, &szUtfNick);
    nNickLen = strlennull(szUtfNick);
  }
  else
  {
    nNickLen = 0;
  }

  // Prepare custom utf-8 note
  if (szNote && (strlen(szNote) > 0))
  {
    int nResult;

    nResult = utf8_encode(szNote, &szUtfNote);
    nNoteLen = strlennull(szUtfNote);
  }
  else
  {
    nNoteLen = 0;
  }

  // Build the packet
  packet.wLen = nUinLen + 20;	
  if (nNickLen > 0)
    packet.wLen += nNickLen + 4;
  if (nNoteLen > 0)
    packet.wLen += nNoteLen + 4;
  if (authRequired)
    packet.wLen += 4;

  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, wAction, 0, dwCookie);
  packWord(&packet, (WORD)nUinLen);
  if (dwUin)
    packBuffer(&packet, szUin, (WORD)nUinLen);
  else
    packBuffer(&packet, szUID, (WORD)nUinLen);
  packWord(&packet, wGroupId);
  packWord(&packet, wContactId);
  packWord(&packet, wItemType);

  wTLVlen = ((nNickLen>0) ? 4+nNickLen : 0) + ((nNoteLen>0) ? 4+nNoteLen : 0) + (authRequired?4:0);
  packWord(&packet, wTLVlen);
  if (authRequired)
    packDWord(&packet, 0x00660000);  // "Still waiting for auth" TLV
  if (nNickLen > 0)
  {
    packWord(&packet, 0x0131);	// Nickname TLV
    packWord(&packet, (WORD)nNickLen);
    packBuffer(&packet, szUtfNick, (WORD)nNickLen);
  }
  if (nNoteLen > 0)
  {
    packWord(&packet, 0x013C);	// Comment TLV
    packWord(&packet, (WORD)nNoteLen);
    packBuffer(&packet, szUtfNote, (WORD)nNoteLen);
  }

	// Send the packet and return the cookie
  sendServPacket(&packet);

  SAFE_FREE(&szUtfNick);
  SAFE_FREE(&szUtfNote);

  return dwCookie;
}



DWORD icq_sendGroup(DWORD dwCookie, WORD wAction, WORD wGroupId, const char *szName, void *pContent, int cbContent)
{
  icq_packet packet;
  int nNameLen;
  char* szUtfName = NULL;
  WORD wTLVlen;

  // Prepare custom utf-8 group name
  if (szName && (strlen(szName) > 0))
  {
    int nResult;

    nResult = utf8_encode(szName, &szUtfName);
    nNameLen = strlennull(szUtfName);
  }
  else
  {
    nNameLen = 0;
  }
  if (nNameLen == 0 && wGroupId != 0)
  {
    NetLog_Server("Group upload failed (GroupName missing).");
    return 0; // without name we could not change the group
  }

  // Build the packet
  packet.wLen = nNameLen + 20;	
  if (cbContent > 0)
    packet.wLen += cbContent + 4;

  write_flap(&packet, ICQ_DATA_CHAN);
  packFNACHeader(&packet, ICQ_LISTS_FAMILY, wAction, 0, dwCookie);
  packWord(&packet, (WORD)nNameLen);
  if (nNameLen) packBuffer(&packet, szUtfName, (WORD)nNameLen);
  packWord(&packet, wGroupId);
  packWord(&packet, 0); // ItemId is always 0 for groups
  packWord(&packet, 1); // ItemType 1 = group

  wTLVlen = ((cbContent>0) ? 4+cbContent : 0);
  packWord(&packet, wTLVlen);
  if (cbContent > 0)
  {
    packWord(&packet, 0x0C8);	// Groups TLV
    packWord(&packet, (WORD)cbContent);
    packBuffer(&packet, pContent, (WORD)cbContent);
  }

	// Send the packet and return the cookie
  sendServPacket(&packet);

  SAFE_FREE(&szUtfName);

  return dwCookie;
}


/*****************************************
 *
 *     --- Contact DB Utilities ---
 *
 */

static int GroupNamesEnumProc(const char *szSetting,LPARAM lParam)
{ // if we got pointer, store setting name, return zero
  if (lParam)
  {
    char** block = (char**)malloc(2*sizeof(char*));
    block[1] = _strdup(szSetting);
    block[0] = ((char**)lParam)[0];
    ((char**)lParam)[0] = (char*)block;
  }
  return 0;
}



int IsServerGroupsDefined()
{
  DBCONTACTENUMSETTINGS dbces;

  char szModule[MAX_PATH+9];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "Groups");

  dbces.pfnEnumProc = &GroupNamesEnumProc;
  dbces.szModule = szModule;
  dbces.lParam = 0;

  if (CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces))
    return 0; // no groups defined

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");

  if (CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces))
  { // old version of groups, remove old settings and force to reload all info
    char** list = NULL;
    strcpy(szModule, gpszICQProtoName);
    strcat(szModule, "Groups");
    dbces.lParam = (LPARAM)&list;
    CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces);
    while (list)
    {
      void* bet;

      DBDeleteContactSetting(NULL, szModule, list[1]);
      SAFE_FREE(&list[1]);
      bet = list;
      list = (char**)list[0];
      SAFE_FREE(&bet);
    }
    return 0; // no groups defined
  }
  else 
    return 1;
}



// Look thru DB and collect all ContactIDs from a group
void* collectBuddyGroup(WORD wGroupID, int *count)
{
  WORD* buf = NULL;
  int cnt = 0;
  HANDLE hContact;
  WORD wItemID;

  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

  while (hContact)
  { // search all contacts
    if (wGroupID == ICQGetContactSettingWord(hContact, "SrvGroupId", 0))
    { // add only buddys from specified group
      wItemID = ICQGetContactSettingWord(hContact, "ServerId", 0);

      if (wItemID)
      { // valid ID, add
        cnt++;
        buf = (WORD*)realloc(buf, cnt*sizeof(WORD));
        buf[cnt-1] = wItemID;
      }
    }

    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }

  *count = cnt<<1; // we return size in bytes
  return buf;
}



// Look thru DB and collect all GroupIDs
void* collectGroups(int *count)
{
  WORD* buf = NULL;
  int cnt = 0;
  int i;
  HANDLE hContact;
  WORD wGroupID;

  hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);

  while (hContact)
  { // search all contacts
    if (wGroupID = ICQGetContactSettingWord(hContact, "SrvGroupId", 0))
    { // add only valid IDs
      for (i = 0; i<cnt; i++)
      { // check for already added ids
        if (buf[i] == wGroupID) break;
      }

      if (i == cnt)
      { // not preset, add
        cnt++;
        buf = (WORD*)realloc(buf, cnt*sizeof(WORD));
        buf[i] = wGroupID;
      }
    }

    hContact = (HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
  }

  *count = cnt<<1;
  return buf;
}



static int GroupLinksEnumProc(const char *szSetting,LPARAM lParam)
{ // check link target, add if match
  if (DBGetContactSettingWord(NULL, ((char**)lParam)[2], szSetting, 0) == (WORD)((char**)lParam)[1])
  {
    char** block = (char**)malloc(2*sizeof(char*));
    block[1] = _strdup(szSetting);
    block[0] = ((char**)lParam)[0];
    ((char**)lParam)[0] = (char*)block;
  }
  return 0;
}



void removeGroupPathLinks(WORD wGroupID)
{ // remove miranda grouppath links targeting to this groupid
  DBCONTACTENUMSETTINGS dbces;
  char szModule[MAX_PATH+6];
  char* pars[3];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "Groups");

  pars[0] = NULL;
  pars[1] = (char*)wGroupID;
  pars[2] = szModule;

  dbces.pfnEnumProc = &GroupLinksEnumProc;
  dbces.szModule = szModule;
  dbces.lParam = (LPARAM)pars;

  if (!CallService(MS_DB_CONTACT_ENUMSETTINGS, (WPARAM)NULL, (LPARAM)&dbces))
  { // we found some links, remove them
    char** list = (char**)pars[0];
    while (list)
    {
      void* bet;

      DBDeleteContactSetting(NULL, szModule, list[1]);
      SAFE_FREE(&list[1]);
      bet = list;
      list = (char**)list[0];
      SAFE_FREE(&bet);
    }
  }
  return;
}



char* getServerGroupName(WORD wGroupID)
{
  DBVARIANT dbv;
  char szModule[MAX_PATH+9];
  char szGroup[16];
  char *szRes;

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");
  _itoa(wGroupID, szGroup, 0x10);

  if (!CheckServerID(wGroupID, 0))
  { // check if valid id, if not give empty and remove
    DBDeleteContactSetting(NULL, szModule, szGroup);
    return NULL;
  }

  if (DBGetContactSetting(NULL, szModule, szGroup, &dbv))
    szRes = NULL;
  else
    szRes = _strdup(dbv.pszVal);
  DBFreeVariant(&dbv);

  return szRes;
}



void setServerGroupName(WORD wGroupID, const char* szGroupName)
{
  char szModule[MAX_PATH+9];
  char szGroup[16];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");
  _itoa(wGroupID, szGroup, 0x10);

  if (szGroupName)
    DBWriteContactSettingString(NULL, szModule, szGroup, szGroupName);
  else
  {
    DBDeleteContactSetting(NULL, szModule, szGroup);
    removeGroupPathLinks(wGroupID);
  }
  return;
}



void setServerGroupNameUtf(WORD wGroupID, const char* szGroupNameUtf)
{
  char szModule[MAX_PATH+9];
  char szGroup[16];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "SrvGroups");
  _itoa(wGroupID, szGroup, 0x10);

  if (szGroupNameUtf)
    UniWriteContactSettingUtf(NULL, szModule, szGroup, (char*)szGroupNameUtf);
  else
  {
    DBDeleteContactSetting(NULL, szModule, szGroup);
    removeGroupPathLinks(wGroupID);
  }
  return;
}
 


WORD getServerGroupID(const char* szPath)
{
  char szModule[MAX_PATH+6];
  WORD wGroupId;

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "Groups");

  wGroupId = DBGetContactSettingWord(NULL, szModule, szPath, 0);

  if (wGroupId && !CheckServerID(wGroupId, 0))
  { // known, check if still valid, if not remove
    DBDeleteContactSetting(NULL, szModule, szPath);
    wGroupId = 0;
  }

  return wGroupId;
}



void setServerGroupID(const char* szPath, WORD wGroupID)
{
  char szModule[MAX_PATH+6];

  strcpy(szModule, gpszICQProtoName);
  strcat(szModule, "Groups");

  if (wGroupID)
    DBWriteContactSettingWord(NULL, szModule, szPath, wGroupID);
  else
    DBDeleteContactSetting(NULL, szModule, szPath);

  return;
}



// copied from groups.c - horrible, but only possible as this is not available as service
int GroupNameExists(const char *name,int skipGroup)
{
  char idstr[33];
  DBVARIANT dbv;
  int i;

  if (name == NULL) return 1; // no group always exists
  for(i=0;;i++)
  {
    if(i==skipGroup) continue;
    itoa(i,idstr,10);
    if(DBGetContactSetting(NULL,"CListGroups",idstr,&dbv)) break;
    if(!strcmp(dbv.pszVal+1,name)) 
    {
      DBFreeVariant(&dbv);
      return 1;
    }
    DBFreeVariant(&dbv);
  }
  return 0;
}



// utility function which counts > on start of a server group name
int countGroupLevel(WORD wGroupId)
{
  char* szGroupName = getServerGroupName(wGroupId);
  int nNameLen = strlennull(szGroupName);
  int i = 0;

  while (i<nNameLen)
  {
    if (szGroupName[i] != '>')
    {
      SAFE_FREE(&szGroupName);
      return i;
    }
    i++;
  }
  SAFE_FREE(&szGroupName);
  return -1; // something went wrong
}



// demangle group path
char* makeGroupPath(WORD wGroupId)
{
  char* szGroup = NULL;

  if (szGroup = getServerGroupName(wGroupId))
  { // this groupid is not valid
    while (strstr(szGroup, "\\")!=NULL) *strstr(szGroup, "\\") = '_'; // remove invalid char
    if (getServerGroupID(szGroup) == wGroupId)
    { // this grouppath is known and is for this group, set user group
      return szGroup;
    }
    else
    {
      if (strlennull(szGroup) && (szGroup[0] == '>'))
      { // it is probably a sub-group
        WORD wId = wGroupId-1;
        int level = countGroupLevel(wGroupId);
        int levnew = countGroupLevel(wId);
        char* szTempGroup;

        if (level == -1)
        { // this is just an ordinary group
          int hGroup;

          if (!GroupNameExists(szGroup, -1))
          { // if the group does not exist, create it
            hGroup = CallService(MS_CLIST_GROUPCREATE, 0, 0);
            CallService(MS_CLIST_GROUPRENAME, hGroup, (LPARAM)szGroup);
          }
          setServerGroupID(szGroup, wGroupId); // set grouppath id
          return szGroup;
        }
        while ((levnew >= level) && (levnew != -1))
        { // we look for parent group
          wId--;
          levnew = countGroupLevel(wId);
        }
        if (levnew == -1)
        { // that was not a sub-group, it was just a group starting with >
          int hGroup;

          if (!GroupNameExists(szGroup, -1))
          { // if the group does not exist, create it
            hGroup = CallService(MS_CLIST_GROUPCREATE, 0, 0);
            CallService(MS_CLIST_GROUPRENAME, hGroup, (LPARAM)szGroup);
          }
          setServerGroupID(szGroup, wGroupId); // set grouppath id
          return szGroup;
        }

        szTempGroup = makeGroupPath(wId);

        szTempGroup = realloc(szTempGroup, strlennull(szGroup)+strlennull(szTempGroup)+2);
        strcat(szTempGroup, "\\");
        strcat(szTempGroup, szGroup+level);
        SAFE_FREE(&szGroup);
        szGroup = szTempGroup;
        
        if (getServerGroupID(szGroup) == wGroupId)
        { // known path, give
          return szGroup;
        }
        else
        { // unknown path, create
          int hGroup;

          if (!GroupNameExists(szGroup, -1))
          { // if the group does not exist, create it
            hGroup = CallService(MS_CLIST_GROUPCREATE, 0, 0);
            CallService(MS_CLIST_GROUPRENAME, hGroup, (LPARAM)szGroup);
          }
          setServerGroupID(szGroup, wGroupId); // set grouppath id
          return szGroup;
        }
      }
      else
      { // create that group
        int hGroup;

        if (!GroupNameExists(szGroup, -1))
        { // if the group does not exist, create it
          hGroup = CallService(MS_CLIST_GROUPCREATE, 0, 0);
          CallService(MS_CLIST_GROUPRENAME, hGroup, (LPARAM)szGroup);
        }
        setServerGroupID(szGroup, wGroupId); // set grouppath id
        return szGroup;
      }
    }
  }
  return NULL;
}



// this is the second pard of recursive event-driven procedure
void madeMasterGroupId(WORD wGroupID, LPARAM lParam)
{
  servlistcookie* clue = (servlistcookie*)lParam;
  char* szGroup = clue->szGroupName;
  GROUPADDCALLBACK ofCallback = clue->ofCallback;
  servlistcookie* param = (servlistcookie*)clue->lParam;
  int level;
  
  if (wGroupID) // if we got an id count level
    level = countGroupLevel(wGroupID);
  else
    level = -1;

  SAFE_FREE(&clue);

  if (level == -1)
  { // something went wrong, give the id and go away
    if (ofCallback) ofCallback(wGroupID, (LPARAM)param);

    SAFE_FREE(&szGroup);
    return;
  }
  level++; // we are a sub

  // check if on that id is not group of the same or greater level, if yes, try next
  while (CheckServerID((WORD)(wGroupID+1),0) && (countGroupLevel((WORD)(wGroupID+1)) >= level))
  {
    wGroupID++;
  }

  if (!CheckServerID((WORD)(wGroupID+1), 0))
  { // the next id is free, so create our group with that id
    servlistcookie* ack;
    DWORD dwCookie;
    char* szSubGroup = (char*)malloc(strlennull(szGroup)+level+1);

    if (szSubGroup)
    {
      int i;

      for (i=0; i<level; i++)
      {
        szSubGroup[i] = '>';
      }
      strcpy(szSubGroup+level, szGroup);
      szSubGroup[strlennull(szGroup)+level] = '\0';

      if (ack = (servlistcookie*)malloc(sizeof(servlistcookie)))
      { // we have cookie good, go on
        ReserveServerID((WORD)(wGroupID+1), SSIT_GROUP);
        
        ack->hContact = NULL;
        ack->wContactId = 0;
        ack->wGroupId = wGroupID+1;
        ack->szGroupName = szSubGroup; // we need that name
        ack->dwAction = SSA_GROUP_ADD;
        ack->dwUin = 0;
        ack->ofCallback = ofCallback;
        ack->lParam = (LPARAM)param;
        dwCookie = AllocateCookie(ICQ_LISTS_ADDTOLIST, 0, ack);

        sendAddStart(0);
        icq_sendGroup(dwCookie, ICQ_LISTS_ADDTOLIST, ack->wGroupId, szSubGroup, NULL, 0);

        SAFE_FREE(&szGroup);
        return;
      }
      SAFE_FREE(&szSubGroup);
    }
  }
  // we failed to create sub-group give parent groupid
  if (ofCallback) ofCallback(wGroupID, (LPARAM)param);

  SAFE_FREE(&szGroup);
  return;
}



// create group with this path, a bit complex task
// this supposes that all server groups are known
WORD makeGroupId(const char* szGroupPath, GROUPADDCALLBACK ofCallback, servlistcookie* lParam)
{
  WORD wGroupID = 0;
  char* szGroup = (char*)szGroupPath;

  if (!szGroup || szGroup[0]=='\0') szGroup = DEFAULT_SS_GROUP;

  if (wGroupID = getServerGroupID(szGroup))
  {
    if (ofCallback) ofCallback(wGroupID, (LPARAM)lParam);
    return wGroupID; // if the path is known give the id
  }

  if (!strstr(szGroup, "\\"))
  { // a root group can be simply created without problems
    servlistcookie* ack;
    DWORD dwCookie;

    if (ack = (servlistcookie*)malloc(sizeof(servlistcookie)))
    { // we have cookie good, go on
      ack->hContact = NULL;
      ack->wContactId = 0;
      ack->wGroupId = GenerateServerId(SSIT_GROUP);
      ack->szGroupName = _strdup(szGroup); // we need that name
      ack->dwAction = SSA_GROUP_ADD;
      ack->dwUin = 0;
      ack->ofCallback = ofCallback;
      ack->lParam = (LPARAM)lParam;
      dwCookie = AllocateCookie(ICQ_LISTS_ADDTOLIST, 0, ack);

      sendAddStart(0);
      icq_sendGroup(dwCookie, ICQ_LISTS_ADDTOLIST, ack->wGroupId, szGroup, NULL, 0);

      return 0;
    }
  }
  else
  { // this is a sub-group
    char* szSub = _strdup(szGroup); // create subgroup, recursive, event-driven, possibly relocate 
    servlistcookie* ack;
    char *szLast;

    if (strstr(szSub, "\\") != NULL)
    { // determine parent group
      szLast = strstr(szSub, "\\")+1;

      while (strstr(szLast, "\\") != NULL)
        szLast = strstr(szLast, "\\")+1; // look for last backslash
      szLast[-1] = '\0'; 
    }
    // make parent group id
    ack = (servlistcookie*)malloc(sizeof(servlistcookie));
    if (ack)
    {
      WORD wRes;
      ack->lParam = (LPARAM)lParam;
      ack->ofCallback = ofCallback;
      ack->szGroupName = _strdup(szLast); // groupname
      wRes = makeGroupId(szSub, madeMasterGroupId, ack);
      SAFE_FREE(&szSub);

      return wRes;
    }

    SAFE_FREE(&szSub); 
  }
  
  if (strstr(szGroup, "\\") != NULL)
  { // we failed to get grouppath, trim it by one group
    WORD wRes;
    char *szLast = _strdup(szGroup);
    char *szLess = szLast;

    while (strstr(szLast, "\\") != NULL)
      szLast = strstr(szLast, "\\")+1; // look for last backslash
    szLast[-1] = '\0'; 
    wRes = makeGroupId(szLess, ofCallback, lParam);
    SAFE_FREE(&szLess);

    return wRes;
  }

  wGroupID = 0; // everything failed, let callback handle error
  if (ofCallback) ofCallback(wGroupID, (LPARAM)lParam);
  
  return wGroupID;
}


/*****************************************
 *
 *    --- Server-List Operations ---
 *
 */

void addServContactReady(WORD wGroupID, LPARAM lParam)
{
  WORD wItemID;
  DWORD dwUin;
  char* szUid;
  servlistcookie* ack;
  DWORD dwCookie;

  ack = (servlistcookie*)lParam;

  if (!ack || !wGroupID) // something went wrong
  {
    if (ack) RemovePendingOperation(ack->hContact, 0);
    return;
  }

  wItemID = ICQGetContactSettingWord(ack->hContact, "ServerId", 0);

  if (wItemID)
  { // Only add the contact if it doesnt already have an ID
    RemovePendingOperation(ack->hContact, 0);
    NetLog_Server("Failed to add contact to server side list (%s)", "already there");
    return;
  }

	// Get UID
	if (ICQGetContactSettingUID(ack->hContact, &dwUin, &szUid))
  { // Could not do anything without uid
    RemovePendingOperation(ack->hContact, 0);
    NetLog_Server("Failed to add contact to server side list (%s)", "no UID");
    return;
  }

	wItemID = GenerateServerId(SSIT_ITEM);

  ack->dwAction = SSA_CONTACT_ADD;
  ack->dwUin = dwUin;
  ack->szUID = szUid;
  ack->wGroupId = wGroupID;
  ack->wContactId = wItemID;

  dwCookie = AllocateCookie(ICQ_LISTS_ADDTOLIST, dwUin, ack);

  sendAddStart(1); // TODO: make some sense here
  icq_sendBuddy(dwCookie, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wGroupID, wItemID, ack->szGroupName, NULL, 0, SSI_ITEM_BUDDY);
}



// Called when contact should be added to server list, if group does not exist, create one
DWORD addServContact(HANDLE hContact, const char *pszNick, const char *pszGroup)
{
  servlistcookie* ack;

  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  { // Could not do anything without cookie
    NetLog_Server("Failed to add contact to server side list (%s)", "malloc failed");
    return 0;
  }
  else
  {
    ack->hContact = hContact;
    ack->szGroupName = _strdup(pszNick); // we need this for resending

    if (AddPendingOperation(hContact, pszGroup, ack, addServContactReady))
      makeGroupId(pszGroup, addServContactReady, ack);

    return 1;
  }
}



// Called when contact should be removed from server list, remove group if it remain empty
DWORD removeServContact(HANDLE hContact)
{
  WORD wGroupID;
  WORD wItemID;
  DWORD dwUin;
  char* szUid;
  servlistcookie* ack;
  DWORD dwCookie;

  // Get the contact's group ID
  if (!(wGroupID = ICQGetContactSettingWord(hContact, "SrvGroupId", 0)))
  {
    // Could not find a usable group ID
    NetLog_Server("Failed to remove contact from server side list (%s)", "no group ID");
    return 0;
  }

  // Get the contact's item ID
  if (!(wItemID = ICQGetContactSettingWord(hContact, "ServerId", 0)))
  {
    // Could not find usable item ID
    NetLog_Server("Failed to remove contact from server side list (%s)", "no item ID");
    return 0;
  }

	// Get UID
	if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
  {
    // Could not do anything without uid
    NetLog_Server("Failed to remove contact from server side list (%s)", "no UID");
    return 0;
  }

  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  { // Could not do anything without cookie
    NetLog_Server("Failed to remove contact from server side list (%s)", "malloc failed");
    return 0;
  }
  else
  {
    ack->dwAction = SSA_CONTACT_REMOVE;
    ack->dwUin = dwUin;
    ack->szUID = szUid;
    ack->hContact = hContact;
    ack->wGroupId = wGroupID;
    ack->wContactId = wItemID;

    dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUin, ack);
  }

  sendAddStart(0);
  icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wGroupID, wItemID, NULL, NULL, 0, SSI_ITEM_BUDDY);

  return 0;
}



void moveServContactReady(WORD wNewGroupID, LPARAM lParam)
{
  DBVARIANT dbvNick;
  WORD wItemID;
  WORD wGroupID;
  DWORD dwUin;
  char* szUid;
  servlistcookie* ack;
  DWORD dwCookie, dwCookie2;
  DBVARIANT dbvNote;
  char* pszNote;
  char* pszNick;
  BYTE bAuth;

  ack = (servlistcookie*)lParam;

  if (!ack || !wNewGroupID) // something went wrong
  {
    if (ack) RemovePendingOperation(ack->hContact, 0);
    return;
  }

  if (!ack->hContact) return; // we do not move us, caused our uin was wrongly added to list

  wItemID = ICQGetContactSettingWord(ack->hContact, "ServerId", 0);
  wGroupID = ICQGetContactSettingWord(ack->hContact, "SrvGroupId", 0);

  // Read nick name from DB
  if (DBGetContactSetting(ack->hContact, "CList", "MyHandle", &dbvNick))
    pszNick = NULL; // if not read, no nick
  else
    pszNick = dbvNick.pszVal;
    
  ack->szGroupName = _strdup(pszNick); // we need this for sending

  DBFreeVariant(&dbvNick);

  if (!wItemID) 
  { // We have no ID, so try to simply add the contact to serv-list 
    NetLog_Server("Unable to move contact (no ItemID) -> trying to add");
    // we know the GroupID, so directly call add
    addServContactReady(wNewGroupID, lParam);
    return;
  }

  if (!wGroupID)
  { // Only move the contact if it had an GroupID
    RemovePendingOperation(ack->hContact, 0);
    NetLog_Server("Failed to move contact to group on server side list (%s)", "no Group");
    return;
  }

  if (wGroupID == wNewGroupID)
  { // Only move the contact if it had different GroupID
    RemovePendingOperation(ack->hContact, 1);
    NetLog_Server("Contact not moved to group on server side list (same Group)");
    return;
  }

	// Get UID
	if (ICQGetContactSettingUID(ack->hContact, &dwUin, &szUid))
  { // Could not do anything without uin
    RemovePendingOperation(ack->hContact, 0);
    NetLog_Server("Failed to move contact to group on server side list (%s)", "no UID");
    return;
  }

  // Read comment from DB
  if (DBGetContactSetting(ack->hContact, "UserInfo", "MyNotes", &dbvNote))
    pszNote = NULL; // if not read, no note
  else
    pszNote = dbvNote.pszVal;

  bAuth = ICQGetContactSettingByte(ack->hContact, "Auth", 0);

  pszNick = ack->szGroupName;

  ack->szGroupName = NULL;
  ack->dwAction = SSA_CONTACT_SET_GROUP;
  ack->dwUin = dwUin;
  ack->szUID = szUid;
  ack->wGroupId = wGroupID;
  ack->wContactId = wItemID;
  ack->wNewContactId = GenerateServerId(SSIT_ITEM); // icq5 recreates also this, imitate
  ack->wNewGroupId = wNewGroupID;
  ack->lParam = 0; // we use this as a sign

  dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUin, ack);
  dwCookie2 = AllocateCookie(ICQ_LISTS_ADDTOLIST, dwUin, ack);

  sendAddStart(0);
  /* this is just like Licq does it, icq5 sends that in different order, but sometimes it gives unwanted
  /* side effect, so I changed the order. */
  if (dwUin)
  {
    icq_sendBuddy(dwCookie2, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wNewGroupID, ack->wNewContactId, pszNick, pszNote, bAuth, SSI_ITEM_BUDDY);
    icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wGroupID, wItemID, NULL, NULL, bAuth, SSI_ITEM_BUDDY);
  }
  else
  { // aim contacts cannot be moved this way, imitate icq5
    icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUin, szUid, wGroupID, wItemID, NULL, NULL, bAuth, SSI_ITEM_BUDDY);
    icq_sendBuddy(dwCookie2, ICQ_LISTS_ADDTOLIST, dwUin, szUid, wNewGroupID, ack->wNewContactId, pszNick, pszNote, bAuth, SSI_ITEM_BUDDY);
  }

  DBFreeVariant(&dbvNote);
  SAFE_FREE(&pszNick);
}



// Called when contact should be moved from one group to another, create new, remove empty
DWORD moveServContactGroup(HANDLE hContact, const char *pszNewGroup)
{
  servlistcookie* ack;

  if (!GroupNameExists(pszNewGroup, -1) && (pszNewGroup != NULL) && (pszNewGroup[0]!='\0'))
  { // the contact moved to non existing group, do not do anything: MetaContact hack
    NetLog_Server("Contact not moved - probably hiding by MetaContacts.");
    return 0;
  }

  if (!ICQGetContactSettingWord(hContact, "ServerId", 0))
  { // the contact is not stored on the server, check if we should try to add
    if (!ICQGetContactSettingByte(NULL, "ServerAddRemove", DEFAULT_SS_ADDSERVER))
      return 0;
  }

  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  { // Could not do anything without cookie
    NetLog_Server("Failed to add contact to server side list (%s)", "malloc failed");
    return 0;
  }
  else
  {
    ack->hContact = hContact;

    if (AddPendingOperation(hContact, pszNewGroup, ack, moveServContactReady))
      makeGroupId(pszNewGroup, moveServContactReady, ack);
    return 1;
  }
}



// Is called when a contact has been renamed locally to update
// the server side nick name.
DWORD renameServContact(HANDLE hContact, const char *pszNick)
{
  WORD wGroupID;
  WORD wItemID;
  DWORD dwUin;
  char* szUid;
  BOOL bAuthRequired;
  DBVARIANT dbvNote;
  char* pszNote;
  servlistcookie* ack;
  DWORD dwCookie;

  // Get the contact's group ID
  if (!(wGroupID = ICQGetContactSettingWord(hContact, "SrvGroupId", 0)))
  {
    // Could not find a usable group ID
    NetLog_Server("Failed to upload new nick name to server side list (%s)", "no group ID");
    RemovePendingOperation(hContact, 0);
    return 0;
  }

  // Get the contact's item ID
  if (!(wItemID = ICQGetContactSettingWord(hContact, "ServerId", 0)))
  {
    // Could not find usable item ID
    NetLog_Server("Failed to upload new nick name to server side list (%s)", "no item ID");
    RemovePendingOperation(hContact, 0);
    return 0;
  }

  // Check if contact is authorized
  bAuthRequired = (ICQGetContactSettingByte(hContact, "Auth", 0) == 1);

  // Read comment from DB
  if (DBGetContactSetting(hContact, "UserInfo", "MyNotes", &dbvNote))
    pszNote = NULL; // if not read, no note
  else
    pszNote = dbvNote.pszVal;

	// Get UID
	if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
  {
    // Could not set nickname on server without uid
    NetLog_Server("Failed to upload new nick name to server side list (%s)", "no UID");

    RemovePendingOperation(hContact, 0);
    DBFreeVariant(&dbvNote);
    return 0;
  }
  
  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  {
    // Could not allocate cookie - use old fake
    NetLog_Server("Failed to allocate cookie");

    dwCookie = GenerateCookie(ICQ_LISTS_UPDATEGROUP);
  }
  else
  {
    ack->dwAction = SSA_CONTACT_RENAME;
    ack->wContactId = wItemID;
    ack->wGroupId = wGroupID;
    ack->dwUin = dwUin;
    ack->szUID = szUid;
    ack->hContact = hContact;

    dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, dwUin, ack);
  }

  // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
  // ICQ_LISTS_CLI_MODIFYEND when just changing nick name
  icq_sendBuddy(dwCookie, ICQ_LISTS_UPDATEGROUP, dwUin, szUid, wGroupID, wItemID, pszNick, pszNote, bAuthRequired, 0 /* contact */);

  DBFreeVariant(&dbvNote);

  return dwCookie;
}



// Is called when a contact's note was changed to update
// the server side comment.
DWORD setServContactComment(HANDLE hContact, const char *pszNote)
{
  WORD wGroupID;
  WORD wItemID;
  DWORD dwUin;
  char* szUid;
  BOOL bAuthRequired;
  DBVARIANT dbvNick;
  char* pszNick;
  servlistcookie* ack;
  DWORD dwCookie;

  // Get the contact's group ID
  if (!(wGroupID = ICQGetContactSettingWord(hContact, "SrvGroupId", 0)))
  {
    // Could not find a usable group ID
    NetLog_Server("Failed to upload new comment to server side list (%s)", "no group ID");
    return 0;
  }

  // Get the contact's item ID
  if (!(wItemID = ICQGetContactSettingWord(hContact, "ServerId", 0)))
  {
    // Could not find usable item ID
    NetLog_Server("Failed to upload new comment to server side list (%s)", "no item ID");
    return 0;
  }

  // Check if contact is authorized
  bAuthRequired = (ICQGetContactSettingByte(hContact, "Auth", 0) == 1);

  // Read nick name from DB
  if (DBGetContactSetting(hContact, "CList", "MyHandle", &dbvNick))
    pszNick = NULL; // if not read, no nick
  else
    pszNick = dbvNick.pszVal;

	// Get UID
	if (ICQGetContactSettingUID(hContact, &dwUin, &szUid))
  {
    // Could not set comment on server without uid
    NetLog_Server("Failed to upload new comment to server side list (%s)", "no UID");

    DBFreeVariant(&dbvNick);
    return 0;
  }
  
  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  {
    // Could not allocate cookie - use old fake
    NetLog_Server("Failed to allocate cookie");

    dwCookie = GenerateCookie(ICQ_LISTS_UPDATEGROUP);
  }
  else
  {
    ack->dwAction = SSA_CONTACT_COMMENT; 
    ack->wContactId = wItemID;
    ack->wGroupId = wGroupID;
    ack->dwUin = dwUin;
    ack->szUID = szUid;
    ack->hContact = hContact;

    dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, dwUin, ack);
  }

  // There is no need to send ICQ_LISTS_CLI_MODIFYSTART or
  // ICQ_LISTS_CLI_MODIFYEND when just changing nick name
  icq_sendBuddy(dwCookie, ICQ_LISTS_UPDATEGROUP, dwUin, szUid, wGroupID, wItemID, pszNick, pszNote, bAuthRequired, 0 /* contact */);

  DBFreeVariant(&dbvNick);

  return dwCookie;
}



void renameServGroup(WORD wGroupId, char* szGroupName)
{
  servlistcookie* ack;
  DWORD dwCookie;
  char* szGroup, *szLast;
  int level = countGroupLevel(wGroupId);
  int i;
  void* groupData;
  DWORD groupSize;

  if (IsGroupRenamed(wGroupId)) return; // the group was already renamed

  if (level == -1) return; // we failed to prepare group

  szLast = szGroupName;
  i = level;
  while (i)
  { // find correct part of grouppath
    szLast = strstr(szLast, "\\")+1;
    i--;
  }
  szGroup = (char*)malloc(strlennull(szLast)+1+level);
  if (!szGroup) return;
  szGroup[level] = '\0';

  for (i=0;i<level;i++)
  {
    szGroup[i] = '>';
  }
  strcat(szGroup, szLast);
  szLast = strstr(szGroup, "\\");
  if (szLast)
    szLast[0] = '\0';

  if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
  { // cookie failed, use old fake
    dwCookie = GenerateCookie(ICQ_LISTS_UPDATEGROUP);
  }
  if (groupData = collectBuddyGroup(wGroupId, &groupSize))
  {
    ack->dwAction = SSA_GROUP_RENAME;
    ack->dwUin = 0;
    ack->hContact = NULL;
    ack->wGroupId = wGroupId;
    ack->wContactId = 0;
    ack->szGroupName = szGroup; // we need this name

    dwCookie = AllocateCookie(ICQ_LISTS_UPDATEGROUP, 0, ack);

    AddGroupRename(wGroupId);

    icq_sendGroup(dwCookie, ICQ_LISTS_UPDATEGROUP, wGroupId, szGroup, groupData, groupSize);
    SAFE_FREE(&groupData);
  }
}


/*****************************************
 *
 *   --- Miranda Contactlist Hooks ---
 *
 */



static int ServListDbSettingChanged(WPARAM wParam, LPARAM lParam)
{
  DBCONTACTWRITESETTING* cws = (DBCONTACTWRITESETTING*)lParam;

  // We can't upload changes to NULL contact
  if ((HANDLE)wParam == NULL)
    return 0;

  // TODO: Queue changes that occur while offline
  if (!icqOnline || !gbSsiEnabled || bIsSyncingCL)
    return 0;

  { // only our contacts will be handled
    char* szProto;

    szProto = (char*)CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)wParam, 0);
    if (szProto && !strcmp(szProto, gpszICQProtoName))
      ;// our contact, fine; otherwise return
    else 
      return 0;
  }
  
  if (!strcmp(cws->szModule, "CList"))
  {
    // Has a temporary contact just been added permanently?
    if (!strcmp(cws->szSetting, "NotOnList") &&
      (cws->value.type == DBVT_DELETED || (cws->value.type == DBVT_BYTE && cws->value.bVal == 0)) &&
      ICQGetContactSettingByte(NULL, "ServerAddRemove", DEFAULT_SS_ADDSERVER))
    {
      DWORD dwUin;
      char* szUid;

      // Does this contact have a UID?
      if (!ICQGetContactSettingUID((HANDLE)wParam, &dwUin, &szUid))
      {
        char *pszNick;
        char *pszGroup;
        DBVARIANT dbvNick;
        DBVARIANT dbvGroup;

        SAFE_FREE(&szUid);
        // Read nick name from DB
        if (DBGetContactSetting((HANDLE)wParam, "CList", "MyHandle", &dbvNick))
          pszNick = NULL; // if not read, no nick
        else
          pszNick = dbvNick.pszVal;

        // Read group from DB
        if (DBGetContactSetting((HANDLE)wParam, "CList", "Group", &dbvGroup))
          pszGroup = NULL; // if not read, no group
        else
          pszGroup = dbvGroup.pszVal;

        addServContact((HANDLE)wParam, pszNick, pszGroup);

        DBFreeVariant(&dbvNick);
        DBFreeVariant(&dbvGroup);
      }
    }

    // Has contact been renamed?
    if (!strcmp(cws->szSetting, "MyHandle") &&
      ICQGetContactSettingByte(NULL, "StoreServerDetails", DEFAULT_SS_STORE))
    {
      if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
      {
        if (AddPendingOperation((HANDLE)wParam, cws->value.pszVal, (servlistcookie*)1, NULL))
          renameServContact((HANDLE)wParam, cws->value.pszVal);
      }
      else
      {
        if (AddPendingOperation((HANDLE)wParam, NULL, (servlistcookie*)1, NULL))
          renameServContact((HANDLE)wParam, NULL);
      }
    }

    // Has contact been moved to another group?
    if (!strcmp(cws->szSetting, "Group") &&
      ICQGetContactSettingByte(NULL, "StoreServerDetails", DEFAULT_SS_STORE))
    {
      if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
      { // Test if group was not renamed...
        WORD wGroupId = ICQGetContactSettingWord((HANDLE)wParam, "SrvGroupId", 0);
        char* szGroup = makeGroupPath(wGroupId);
        int bRenamed = 0;
        int bMoved = 1;

        if (wGroupId && !GroupNameExists(szGroup, -1))
        { // if we moved from non-existing group, it can be rename
          if (!getServerGroupID(cws->value.pszVal))
          { // the target group is not known - it is probably rename
            if (getServerGroupID(szGroup))
            { // source group not known -> already renamed
              if (!IsGroupRenamed(wGroupId))
              { // is rename in progress ?
                bRenamed = 1; // TODO: we should really check if group was not moved to sub-group
                NetLog_Server("Group %x renamed to ""%s"".", wGroupId, cws->value.pszVal);
              }
              else // if rename in progress do not move contacts
                bMoved = 0;
            }
          }
        }
        SAFE_FREE(&szGroup);

        if (bRenamed)
          renameServGroup(wGroupId, cws->value.pszVal);
        else if (bMoved)
          moveServContactGroup((HANDLE)wParam, cws->value.pszVal);
      }
      else
      {
        moveServContactGroup((HANDLE)wParam, NULL);
      }
    }		
  }

  if (!strcmp(cws->szModule, "UserInfo"))
  {
    if (!strcmp(cws->szSetting, "MyNotes") &&
      ICQGetContactSettingByte(NULL, "StoreServerDetails", DEFAULT_SS_STORE))
    {
      if (cws->value.type == DBVT_ASCIIZ && cws->value.pszVal != 0)
      {
        setServContactComment((HANDLE)wParam, cws->value.pszVal);
      }
      else
      {
        setServContactComment((HANDLE)wParam, NULL);
      }
    }
  }

  if (!strcmp(cws->szModule, "ContactPhoto"))
  { // contact photo changed ?
    ContactPhotoSettingChanged((HANDLE)wParam);
  }

  return 0;
}



static int ServListDbContactDeleted(WPARAM wParam, LPARAM lParam)
{
  DeleteFromCache((HANDLE)wParam);

	if (!icqOnline || !gbSsiEnabled)
		return 0;

  { // we need all server contacts on local buddy list
		WORD wContactID;
		WORD wGroupID;
		WORD wVisibleID;
		WORD wInvisibleID;
    WORD wIgnoreID;
		DWORD dwUIN;
    char* szUID;

		wContactID = ICQGetContactSettingWord((HANDLE)wParam, "ServerId", 0);
		wGroupID = ICQGetContactSettingWord((HANDLE)wParam, "SrvGroupId", 0);
		wVisibleID = ICQGetContactSettingWord((HANDLE)wParam, "SrvPermitId", 0);
		wInvisibleID = ICQGetContactSettingWord((HANDLE)wParam, "SrvDenyId", 0);
		wIgnoreID = ICQGetContactSettingWord((HANDLE)wParam, "SrvIgnoreId", 0);
    if (ICQGetContactSettingUID((HANDLE)wParam, &dwUIN, &szUID))
      return 0;

		if ((wGroupID && wContactID) || wVisibleID || wInvisibleID || wIgnoreID)
		{
			if (wContactID)
      { // delete contact from server
        removeServContact((HANDLE)wParam);
      }

      if (wVisibleID)
      { // detete permit record
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_REMOVEFROMLIST);
        }
        else
        {
          ack->dwAction = SSA_PRIVACY_REMOVE;
          ack->dwUin = dwUIN;
          ack->szUID = strdup(szUID);
          ack->hContact = (HANDLE)wParam;
          ack->wGroupId = 0;
          ack->wContactId = wVisibleID;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUIN, ack);
        }

        icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUIN, szUID, 0, wVisibleID, NULL, NULL, 0, SSI_ITEM_PERMIT);
      }

      if (wInvisibleID)
      { // delete deny record
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_REMOVEFROMLIST);
        }
        else
        {
          ack->dwAction = SSA_PRIVACY_REMOVE;
          ack->dwUin = dwUIN;
          ack->szUID = strdup(szUID);
          ack->hContact = (HANDLE)wParam;
          ack->wGroupId = 0;
          ack->wContactId = wInvisibleID;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUIN, ack);
        }

        icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUIN, szUID, 0, wInvisibleID, NULL, NULL, 0, SSI_ITEM_DENY);
      }

      if (wIgnoreID)
      { // delete ignore record
        servlistcookie* ack;
        DWORD dwCookie;

        if (!(ack = (servlistcookie*)malloc(sizeof(servlistcookie))))
        { // cookie failed, use old fake
          dwCookie = GenerateCookie(ICQ_LISTS_REMOVEFROMLIST);
        }
        else
        {
          ack->dwAction = SSA_PRIVACY_REMOVE; // remove privacy item
          ack->dwUin = dwUIN;
          ack->szUID = strdup(szUID);
          ack->hContact = (HANDLE)wParam;
          ack->wGroupId = 0;
          ack->wContactId = wIgnoreID;

          dwCookie = AllocateCookie(ICQ_LISTS_REMOVEFROMLIST, dwUIN, ack);
        }

        icq_sendBuddy(dwCookie, ICQ_LISTS_REMOVEFROMLIST, dwUIN, szUID, 0, wIgnoreID, NULL, NULL, 0, SSI_ITEM_IGNORE);
      }
    }
    SAFE_FREE(&szUID);
  }

	return 0;
}



void InitServerLists(void)
{
  InitializeCriticalSection(&servlistMutex);
  
  hHookSettingChanged = HookEvent(ME_DB_CONTACT_SETTINGCHANGED, ServListDbSettingChanged);
  hHookContactDeleted = HookEvent(ME_DB_CONTACT_DELETED, ServListDbContactDeleted);
}



void UninitServerLists(void)
{
  if (hHookSettingChanged)
    UnhookEvent(hHookSettingChanged);

  if (hHookContactDeleted)
    UnhookEvent(hHookContactDeleted);

  FlushServerIDs();
  FlushPendingOperations();

  DeleteCriticalSection(&servlistMutex);
}
