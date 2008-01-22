/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2005 Miranda ICQ/IM project,
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

// block these plugins
#define DEFMOD_REMOVED_UIPLUGINOPTS     21
#define DEFMOD_REMOVED_PROTOCOLNETLIB   22

// basic export prototypes
typedef int (__cdecl * Miranda_Plugin_Load) ( PLUGINLINK * );
typedef int (__cdecl * Miranda_Plugin_Unload) ( void );
// version control
typedef PLUGININFOEX * (__cdecl * Miranda_Plugin_InfoEx) ( DWORD mirandaVersion );
// prototype for databases
typedef DATABASELINK * (__cdecl * Database_Plugin_Info) ( void * reserved );
// prototype for clists
typedef int (__cdecl * CList_Initialise) ( PLUGINLINK * );
// Interface support
typedef MUUID * (__cdecl * Miranda_Plugin_Interfaces) ( void );

typedef struct { // can all be NULL
	HINSTANCE hInst;
	Miranda_Plugin_Load Load;
	Miranda_Plugin_Unload Unload;
	Miranda_Plugin_InfoEx InfoEx;
	Miranda_Plugin_Interfaces Interfaces;
	Database_Plugin_Info DbInfo;
	CList_Initialise clistlink;
	PLUGININFOEX * pluginInfo;	 // must be freed if hInst==NULL then its a copy
	DATABASELINK * dblink;		 // only valid during module being in memory
} BASIC_PLUGIN_INFO;

#define PCLASS_FAILED	 0x1  	// not a valid plugin, or API is invalid, pluginname is valid
#define PCLASS_BASICAPI  0x2  	// has Load, Unload, MirandaPluginInfo() -> PLUGININFO seems valid, this dll is in memory.
#define PCLASS_DB	 	 0x4    // has DatabasePluginInfo() and is valid as can be, and PCLASS_BASICAPI has to be set too
#define PCLASS_LAST		 0x8    // this plugin should be unloaded after everything else
#define PCLASS_OK		 0x10   // plugin should be loaded, if DB means nothing
#define PCLASS_LOADED	 0x20   // Load() has been called, Unload() should be called.
#define PCLASS_STOPPED   0x40 	// wasn't loaded cos plugin name not on white list
#define PCLASS_CLIST 	 0x80  // a CList implementation

typedef struct pluginEntry {
	char pluginname[64];
	unsigned int pclass; // PCLASS_*
	BASIC_PLUGIN_INFO bpi;
	struct pluginEntry * nextclass;
} pluginEntry;

struct PluginUUIDList {
    MUUID uuid;
} pluginBannedList[] = {
    {0x7f65393b, 0x7771, 0x4f3f, { 0xa9, 0xeb, 0x5d, 0xba, 0xf2, 0xb3, 0x61, 0xf1 }}, // png2dib
    {0xe00f1643, 0x263c, 0x4599, { 0xb8, 0x4b, 0x5, 0x3e, 0x5c, 0x51, 0x1d, 0x28 }}, // loadavatars (unicode)
    {0xc9e01eb0, 0xa119, 0x42d2, { 0xb3, 0x40, 0xe8, 0x67, 0x8f, 0x5f, 0xea, 0xd9 }} // loadavatars (ansi)
};
const int pluginBannedListCount = SIZEOF(pluginBannedList);

SortedList pluginList = { 0 }, pluginListAddr = { 0 };

PLUGINLINK pluginCoreLink;
char   mirandabootini[MAX_PATH];
static DWORD mirandaVersion;
static pluginEntry * pluginListDb;
static pluginEntry * pluginListUI;
static pluginEntry * pluginList_freeimg = NULL;
static HANDLE hPluginListHeap = NULL;
static pluginEntry * pluginDefModList[DEFMOD_HIGHEST+1]; // do not free this memory
static int askAboutIgnoredPlugins;

int  InitIni(void);
void UninitIni(void);

#define PLUGINDISABLELIST "PluginDisable"

void KillModuleEventHooks( HINSTANCE );
void KillModuleServices( HINSTANCE );

int CallHookSubscribers( HANDLE hEvent, WPARAM wParam, LPARAM lParam );

int LoadDatabaseModule(void);
void ListView_SetItemTextA( HWND hwndLV, int i, int iSubItem, char* pszText );

void ListView_GetItemTextA( HWND hwndLV, int i, int iSubItem, char* pszText, size_t cchTextMax )
{
	LV_ITEMA  ms_lvi;
	ms_lvi.iSubItem = iSubItem;
	ms_lvi.cchTextMax = cchTextMax;
	ms_lvi.pszText = pszText;
	SendMessageA( hwndLV, LVM_GETITEMTEXTA, i, (LPARAM)&ms_lvi);
}

HINSTANCE GetInstByAddress( void* codePtr )
{
	int idx;
	HINSTANCE result;
	pluginEntry p; p.bpi.hInst = codePtr;

	if ( pluginListAddr.realCount == 0 )
		return NULL;

	List_GetIndex( &pluginListAddr, &p, &idx );
	if ( idx > 0 )
		idx--;

	result = (( pluginEntry* )( pluginListAddr.items[idx] ))->bpi.hInst;
	if ( idx == 0 && codePtr < ( void* )result )
		return NULL;

	return result;
}

static int uuidToString(const MUUID uuid, char *szStr, int cbLen)
{
	if (cbLen<1||!szStr) return 0;
	mir_snprintf(szStr, cbLen, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		uuid.a, uuid.b, uuid.c, uuid.d[0], uuid.d[1], uuid.d[2], uuid.d[3], uuid.d[4], uuid.d[5], uuid.d[6], uuid.d[7]);
	return 1;
}

static int equalUUID(MUUID u1, MUUID u2)
{
    return memcmp(&u1, &u2, sizeof(MUUID))?0:1;
}

static 	MUUID miid_last = MIID_LAST;

static int validInterfaceList(Miranda_Plugin_Interfaces ifaceProc)
{
	MUUID *piface = ( ifaceProc ) ? ifaceProc() : NULL;

	if (!piface) 
		return 0;
	if (equalUUID(miid_last, piface[0]))
		return 0;
	return 1;
}

static int isPluginBanned(MUUID u1) {
    int i;
    
    for (i=0; i<pluginBannedListCount; i++) {
        if (equalUUID(pluginBannedList[i].uuid, u1))
            return 1;
    }
    return 0;
}

// returns true if the API exports were good, otherwise, passed in data is returned
#define CHECKAPI_NONE 	0
#define CHECKAPI_DB 	1
#define CHECKAPI_CLIST  2

static int checkPI( BASIC_PLUGIN_INFO* bpi, PLUGININFOEX* pi )
{

	if ( pi == NULL )
		return FALSE;

	if ( pi->cbSize == sizeof(PLUGININFOEX))
		if ( !validInterfaceList(bpi->Interfaces) || isPluginBanned( pi->uuid ))
			return FALSE;

	if ( pi->shortName == NULL || pi->description == NULL || pi->author == NULL ||
		  pi->authorEmail == NULL || pi->copyright == NULL || pi->homepage == NULL )
		return FALSE;

	if ( pi->replacesDefaultModule > DEFMOD_HIGHEST || 
		  pi->replacesDefaultModule == DEFMOD_REMOVED_UIPLUGINOPTS || 
		  pi->replacesDefaultModule == DEFMOD_REMOVED_PROTOCOLNETLIB )
		return FALSE;

	return TRUE;
}

static int checkAPI(char* plugin, BASIC_PLUGIN_INFO* bpi, DWORD mirandaVersion, int checkTypeAPI, int* exports)
{
	HINSTANCE h = NULL;

	h = LoadLibraryA(plugin);
	if ( h == NULL ) return 0;
	// loaded, check for exports
	bpi->Load = (Miranda_Plugin_Load) GetProcAddress(h, "Load");
	bpi->Unload = (Miranda_Plugin_Unload) GetProcAddress(h, "Unload");
	bpi->InfoEx = (Miranda_Plugin_InfoEx) GetProcAddress(h, "MirandaPluginInfoEx");
	bpi->Interfaces = (Miranda_Plugin_Interfaces) GetProcAddress(h, "MirandaPluginInterfaces");

	// if they were present
	if ( bpi->Load && bpi->Unload && bpi->InfoEx && bpi->Interfaces ) {
		PLUGININFOEX* pi = bpi->InfoEx(mirandaVersion);

		if ( checkPI( bpi, pi )) {
			bpi->pluginInfo = pi;
			// basic API is present
			if ( checkTypeAPI == CHECKAPI_NONE ) {
				bpi->hInst=h;
				return 1;
			}
			// check for DB?
			if ( checkTypeAPI == CHECKAPI_DB ) {
				bpi->DbInfo = (Database_Plugin_Info) GetProcAddress(h, "DatabasePluginInfo");
				if ( bpi->DbInfo ) {
					// fetch internal database function pointers
					bpi->dblink = bpi->DbInfo(NULL);
					// validate returned link structure
					if ( bpi->dblink && bpi->dblink->cbSize==sizeof(DATABASELINK) ) {
						bpi->hInst=h;
						return 1;
					}
					// had DB exports
					if ( exports != NULL ) *exports=1;
				} //if
			} //if

			// check clist ?
			if ( checkTypeAPI == CHECKAPI_CLIST ) {
				bpi->clistlink = (CList_Initialise) GetProcAddress(h, "CListInitialise");
				#if defined( _UNICODE )
					if ( pi->flags & UNICODE_AWARE )
				#endif
				if ( bpi->clistlink ) {
					// nothing more can be done here, this export is a load function
					bpi->hInst=h;
					if ( exports != NULL ) *exports=1;
					return 1;
				}
			}
		} // if
		if ( exports != NULL ) *exports=1;
	} //if
	// not found, unload
	FreeLibrary(h);
	return 0;
}

// returns true if the given file is <anything>.dll exactly
static int valid_library_name(char * name)
{
	char * dot = strrchr(name, '.');
	if ( dot != NULL && lstrcmpiA(dot+1,"dll") == 0)
		if ( dot[4] == 0 )
			return 1;

	return 0;
}

// returns true if the given file matches dbx_*.dll, which is used to LoadLibrary()
static int validguess_db_name(char * name)
{
	int rc=0;
	// this is ONLY SAFE because name -> ffd.cFileName == MAX_PATH
	char x = name[4];
	name[4]=0;
	rc=lstrcmpiA(name,"dbx_") == 0;
	name[4]=x;
	return rc;
}

// returns true if the given file matches clist_*.dll
static int validguess_clist_name(char * name)
{
	int rc=0;
	// argh evil
	char x = name[6];
	name[6]=0;
	rc=lstrcmpiA(name,"clist_") == 0;
	name[6]=x;
	return rc;
}

// perform any API related tasks to freeing
static void Plugin_Uninit(pluginEntry * p)
{
	// if it was an installed database plugin, call its unload
	if ( p->pclass & PCLASS_DB )
		p->bpi.dblink->Unload( p->pclass & PCLASS_OK );

	// if the basic API check had passed, call Unload if Load() was ever called
	if ( p->pclass & PCLASS_LOADED )
		p->bpi.Unload();

	// release the library
	if ( p->bpi.hInst != NULL ) {
		// we need to kill all resources which belong to that DLL before calling FreeLibrary
		KillModuleEventHooks( p->bpi.hInst );
		KillModuleServices( p->bpi.hInst );

		FreeLibrary( p->bpi.hInst );
		ZeroMemory( &p->bpi, sizeof( p->bpi ));
	}
	List_RemovePtr( &pluginList, p );
	List_RemovePtr( &pluginListAddr, p );
}

typedef BOOL (*SCAN_PLUGINS_CALLBACK) ( WIN32_FIND_DATAA * fd, char * path, WPARAM wParam, LPARAM lParam );

static void enumPlugins(SCAN_PLUGINS_CALLBACK cb, WPARAM wParam, LPARAM lParam)
{
	char exe[MAX_PATH];
	char search[MAX_PATH];
	char * p = 0;
	// get miranda's exe path
	GetModuleFileNameA(NULL, exe, SIZEOF(exe));
	// find the last \ and null it out, this leaves no trailing slash
	p = strrchr(exe, '\\');
	if ( p ) *p=0;
	// create the search filter
	mir_snprintf(search,SIZEOF(search),"%s\\Plugins\\*.dll", exe);
	{
		// FFFN will return filenames for things like dot dll+ or dot dllx
		HANDLE hFind=INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAA ffd;
		hFind = FindFirstFileA(search, &ffd);
		if ( hFind != INVALID_HANDLE_VALUE ) {
			do {
				if ( !(ffd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && valid_library_name(ffd.cFileName) )
				{
					cb(&ffd,exe,wParam,lParam);
				} //if
			} while ( FindNextFileA(hFind, &ffd) );
			FindClose(hFind);
		} //if
	}
}

// this is called by the db module to return all DBs plugins, then when it finds the one it likes the others are unloaded
static int PluginsEnum(WPARAM wParam, LPARAM lParam)
{
	PLUGIN_DB_ENUM * de = (PLUGIN_DB_ENUM *) lParam;
	pluginEntry * x = pluginListDb;
	if ( de == NULL || de->cbSize != sizeof(PLUGIN_DB_ENUM) || de->pfnEnumCallback == NULL ) return 1;
	while ( x != NULL ) {
		int rc = de->pfnEnumCallback(x->pluginname, x->bpi.dblink, de->lParam);
		if ( rc == DBPE_DONE ) {
			// this db has been picked, get rid of all the others
			pluginEntry * y = pluginListDb, * n;
			while ( y != NULL ) {
				n = y->nextclass;
				if ( x != y )
					Plugin_Uninit(y);
				y = n;
			} // while
			x->pclass |= PCLASS_LOADED | PCLASS_OK | PCLASS_LAST;
			return 0;
		}
		else if ( rc == DBPE_HALT ) return 1;
		x = x->nextclass;
	} // while
	return pluginListDb != NULL ? 1 : -1;
}

static int PluginsGetDefaultArray(WPARAM wParam, LPARAM lParam)
{
	return (int) &pluginDefModList;
}

// called in the first pass to create pluginEntry* structures and validate database plugins
static BOOL scanPluginsDir (WIN32_FIND_DATAA * fd, char * path, WPARAM wParam, LPARAM lParam )
{
	int isdb = validguess_db_name(fd->cFileName);
	BASIC_PLUGIN_INFO bpi;
	pluginEntry * p = HeapAlloc(hPluginListHeap, HEAP_NO_SERIALIZE|HEAP_ZERO_MEMORY, sizeof(pluginEntry));
	strncpy(p->pluginname, fd->cFileName, SIZEOF(p->pluginname));
	// plugin name suggests its a db module, load it right now
	if ( isdb ) {
		char buf[MAX_PATH];
		mir_snprintf(buf,SIZEOF(buf),"%s\\Plugins\\%s", path, fd->cFileName);
		if ( checkAPI(buf, &bpi, mirandaVersion, CHECKAPI_DB, NULL) ) {
			// db plugin is valid
			p->pclass |= (PCLASS_DB|PCLASS_BASICAPI);
			// copy the dblink stuff
			p->bpi=bpi;
			// keep a faster list.
			if ( pluginListDb != NULL ) p->nextclass=pluginListDb;
			pluginListDb=p;
		}
		else {
			// didn't have basic APIs or DB exports - failed.
			p->pclass |= PCLASS_FAILED;
		}
	}
	else if ( validguess_clist_name(fd->cFileName) ) {
		// keep a note of this plugin for later
		if ( pluginListUI != NULL ) p->nextclass=pluginListUI;
		pluginListUI=p;
		p->pclass |= PCLASS_CLIST;
	}
	else if ( lstrcmpiA(fd->cFileName, "advaimg.dll") == 0)
		pluginList_freeimg = p;

	// add it to the list
	List_InsertPtr( &pluginList, p );
	return TRUE;
}

static void SetPluginOnWhiteList(char * pluginname, int allow)
{
	DBWriteContactSettingByte(NULL, PLUGINDISABLELIST, pluginname, (BYTE)(allow ? 0 : 1));
}

// returns 1 if the plugin should be enabled within this profile, filename is always lower case
static int isPluginOnWhiteList(char * pluginname)
{
	int rc = DBGetContactSettingByte(NULL, PLUGINDISABLELIST, pluginname, 0);
	if ( rc != 0 && askAboutIgnoredPlugins ) {
		char buf[256];
		mir_snprintf(buf,SIZEOF(buf),Translate("'%s' is disabled, re-enable?"),pluginname);
		if ( MessageBoxA(NULL,buf,Translate("Re-enable Miranda plugin?"),MB_YESNO|MB_ICONQUESTION) == IDYES ) {
			SetPluginOnWhiteList(pluginname, 1);
			return 1;
	}	}

	return rc == 0;
}

static pluginEntry* getCListModule(char * exe, char * slice, int useWhiteList)
{
	pluginEntry * p = pluginListUI;
	BASIC_PLUGIN_INFO bpi;
	while ( p != NULL )
	{
		mir_snprintf(slice, &exe[MAX_PATH] - slice, "\\Plugins\\%s", p->pluginname);
		CharLowerA(p->pluginname);
		if ( useWhiteList ? isPluginOnWhiteList(p->pluginname) : 1 ) {
			if ( checkAPI(exe, &bpi, mirandaVersion, CHECKAPI_CLIST, NULL) ) {
				p->bpi = bpi;
				p->pclass |= PCLASS_LAST | PCLASS_OK | PCLASS_BASICAPI;
				List_InsertPtr( &pluginListAddr, p );
				if ( bpi.clistlink(&pluginCoreLink) == 0 ) {
					p->bpi=bpi;
					p->pclass |= PCLASS_LOADED;
					return p;
				}
				else Plugin_Uninit( p );
			} //if
		} //if
		p = p->nextclass;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Event hook to unload all non-core plugins
//   hooked very late, after all the internal plugins, blah

void UnloadNewPlugins(void)
{
	int i;

	// unload everything but the special db/clist plugins
	for ( i = pluginList.realCount-1; i >= 0; i-- ) {
		pluginEntry* p = pluginList.items[i];
		if ( !(p->pclass & PCLASS_LAST) && (p->pclass & PCLASS_OK))
			Plugin_Uninit( p );
}	}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Plugins options page dialog

typedef struct 
{
	int   flags;
	char* author;
	char* authorEmail;
	char* description;
	char* copyright;
	char* homepage;
	MUUID uuid;
}
	PluginListItemData;

static BOOL dialogListPlugins(WIN32_FIND_DATAA * fd, char * path, WPARAM wParam, LPARAM lParam)
{
	LVITEMA it;
	int iRow;
	HWND hwndList=(HWND)lParam;
	BASIC_PLUGIN_INFO pi;
	int exports=0;
	char buf[MAX_PATH];
	int isdb = 0;
	HINSTANCE gModule;
	PluginListItemData* dat;

	mir_snprintf(buf,SIZEOF(buf),"%s\\Plugins\\%s",path,fd->cFileName);
	CharLowerA(fd->cFileName);
	gModule=GetModuleHandleA(buf);
	if ( checkAPI(buf, &pi, mirandaVersion, CHECKAPI_NONE, &exports) == 0 ) {
		// failed to load anything, but if exports were good, show some info.
		return TRUE;
	}
	isdb = pi.pluginInfo->replacesDefaultModule == DEFMOD_DB;
	ZeroMemory(&it, sizeof(it));
	it.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
	it.pszText = Translate(fd->cFileName);
	it.iImage = ( pi.pluginInfo->flags & 1 ) ? 0 : 1;
	it.lParam = (LPARAM)( dat = (PluginListItemData*)mir_alloc( sizeof( PluginListItemData )));
	iRow=SendMessageA( hwndList, LVM_INSERTITEMA, 0, (LPARAM)&it );
	if ( isPluginOnWhiteList(fd->cFileName) ) 
		ListView_SetItemState(hwndList, iRow, !isdb ? 0x2000 : 0x3000, LVIS_STATEIMAGEMASK);
	if ( iRow != -1 ) {
		dat->flags = pi.pluginInfo->replacesDefaultModule;
		dat->author = mir_strdup( pi.pluginInfo->author );
		dat->authorEmail = mir_strdup( pi.pluginInfo->authorEmail );
		dat->copyright = mir_strdup( pi.pluginInfo->copyright );
		dat->description = mir_strdup( pi.pluginInfo->description );
		dat->homepage = mir_strdup( pi.pluginInfo->homepage );
		if ( pi.pluginInfo->cbSize == sizeof( PLUGININFOEX ))
			dat->uuid = pi.pluginInfo->uuid;
		else
			memset( &dat->uuid, 0, sizeof(dat->uuid));

		ListView_SetItemTextA(hwndList, iRow, 1, pi.pluginInfo->shortName);
		mir_snprintf(buf,SIZEOF(buf),"%d.%d.%d.%d", HIBYTE(HIWORD(pi.pluginInfo->version)), LOBYTE(HIWORD(pi.pluginInfo->version)), HIBYTE(LOWORD(pi.pluginInfo->version)), LOBYTE(LOWORD(pi.pluginInfo->version)));
		ListView_SetItemTextA(hwndList, iRow, 2, buf);

		it.mask = LVIF_IMAGE;
		it.iItem = iRow;
		it.iSubItem = 3;
		it.iImage = ( gModule != NULL ) ? 2 : 3;
		ListView_SetItem( hwndList, &it );
	}
	else mir_free( dat );
	FreeLibrary(pi.hInst);
	return TRUE;
}

static RemoveAllItems( HWND hwnd )
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iItem = 0;
	while ( ListView_GetItem( hwnd, &lvi )) {
		PluginListItemData* dat = ( PluginListItemData* )lvi.lParam;
		mir_free( dat->author );
		mir_free( dat->authorEmail );
		mir_free( dat->copyright );
		mir_free( dat->description );
		mir_free( dat->homepage );
		mir_free( dat );
		lvi.iItem ++;
}	}

static TCHAR* PluginCutCopyright(TCHAR * buf)
{
	/* Some plugins include (C,c)opyright, which is fine but it looks stupid in the UI */
	TCHAR tmp[16];
	_tcsncpy(tmp, buf, 9);
	tmp[9]=0;
	if ( lstrcmpi( tmp, _T("copyright")) == 0 ) return buf+10;
	return buf;
}

static BOOL CALLBACK DlgPluginOpt(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
	{
		HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGLIST);
		LVCOLUMN col;
		HIMAGELIST hIml = ImageList_Create(16, 16, ILC_MASK | (IsWinVerXPPlus()? ILC_COLOR32 : ILC_COLOR16), 4, 0);
		ImageList_AddIcon_IconLibLoaded( hIml, SKINICON_OTHER_UNICODE );
		ImageList_AddIcon_IconLibLoaded( hIml, SKINICON_OTHER_ANSI );
		ImageList_AddIcon_IconLibLoaded( hIml, SKINICON_OTHER_LOADED );
		ImageList_AddIcon_IconLibLoaded( hIml, SKINICON_OTHER_NOTLOADED );
		ListView_SetImageList( hwndList, hIml, LVSIL_SMALL );

		TranslateDialogDefault(hwndDlg);

		col.mask = LVCF_TEXT | LVCF_WIDTH;
		col.pszText = TranslateT("Plugin");
		col.cx = 70;//max = 140;
		ListView_InsertColumn(hwndList,0,&col);

		col.pszText=TranslateT("Name");
		col.cx = 70;//max = 220;
		ListView_InsertColumn(hwndList,1,&col);

		col.pszText=TranslateT("Version");
		col.cx=55;
		ListView_InsertColumn(hwndList,2,&col);

		col.pszText=_T("");
		col.cx=20;
		ListView_InsertColumn(hwndList,3,&col);
		//ListView_InsertColumn(hwndList,4,&col);

		// XXX: Won't work on windows 95 without IE3+ or 4.70
		ListView_SetExtendedListViewStyleEx( hwndList, 0, LVS_EX_SUBITEMIMAGES | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT );
		// scan the plugin dir for plugins, cos
		enumPlugins( dialogListPlugins, ( WPARAM )hwndDlg, ( LPARAM )hwndList );
		// sort out the headers
        {
            int w, max;
            
            ListView_SetColumnWidth( hwndList, 0, LVSCW_AUTOSIZE ); // dll name
            w = ListView_GetColumnWidth( hwndList, 0 );
            if (w>140) {
                ListView_SetColumnWidth( hwndList, 0, 140 );
                w = 140;
            }
            max = w<140? 220+140-w:220;
            ListView_SetColumnWidth( hwndList, 1, LVSCW_AUTOSIZE ); // short name
            w = ListView_GetColumnWidth( hwndList, 1 );
            if (w>max)
                ListView_SetColumnWidth( hwndList, 1, max );
        }
		return TRUE;
	}
	case WM_NOTIFY:
	{
		NMLISTVIEW * hdr = (NMLISTVIEW *) lParam;
		if ( hdr && hdr->hdr.code == LVN_ITEMCHANGED && hdr->uOldState != 0
			&& (hdr->uNewState == 0x1000 || hdr->uNewState == 0x2000 ) && IsWindowVisible(hdr->hdr.hwndFrom) ) {
			HWND hwndList = GetDlgItem(hwndDlg,IDC_PLUGLIST);
			PluginListItemData* dat;
			int iRow;
			LVITEM it;
			it.mask=LVIF_PARAM | LVIF_STATE;
			it.iItem = hdr->iItem;
			if ( !ListView_GetItem( hwndList, &it ))
				break;

			dat = ( PluginListItemData* )it.lParam;
			if ( dat->flags == DEFMOD_DB ) {
				ListView_SetItemState(hwndList, hdr->iItem, 0x3000, LVIS_STATEIMAGEMASK);
				return FALSE;
			}
			// if enabling and replaces, find all other replaces and toggle off
			if ( hdr->uNewState & 0x2000 && dat->flags != 0 )  {
				for ( iRow=0; iRow != -1; ) {
					if ( iRow != hdr->iItem ) {
						LVITEM dt;
						dt.mask = LVIF_PARAM;
						dt.iItem = iRow;
						if ( ListView_GetItem( hwndList, &dt )) {
							PluginListItemData* dat2 = ( PluginListItemData* )dt.lParam;
							if ( dat2->flags == dat->flags ) {
								// the lParam is unset, so when the check is unset the clist block doesnt trigger
								int lParam = dat2->flags;
								dat2->flags = 0;
								ListView_SetItemState(hwndList, iRow, 0x1000, LVIS_STATEIMAGEMASK );
								dat2->flags = lParam;
					}	}	}

					iRow = ListView_GetNextItem( hwndList, iRow, LVNI_ALL );
			}	}

			ShowWindow( GetDlgItem(hwndDlg, IDC_RESTART ), TRUE );
			SendMessage( GetParent( hwndDlg ), PSM_CHANGED, 0, 0 );
			break;
		}

		if ( hdr && hdr->hdr.code == LVN_ITEMCHANGED && IsWindowVisible(hdr->hdr.hwndFrom) && hdr->iItem != -1 ) {
			TCHAR buf[1024];
			int sel = hdr->uNewState & LVIS_SELECTED;
			HWND hwndList = GetDlgItem(hwndDlg, IDC_PLUGLIST);
			LVITEM lvi = { 0 };
			lvi.mask = LVIF_PARAM;
			lvi.iItem = hdr->iItem;
			if ( ListView_GetItem( hwndList, &lvi )) {
				PluginListItemData* dat = ( PluginListItemData* )lvi.lParam;

				ListView_GetItemText(hwndList, hdr->iItem, 1, buf, SIZEOF(buf));
				SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGININFOFRAME),sel ? buf : _T(""));

				SetWindowTextA(GetDlgItem(hwndDlg,IDC_PLUGINAUTHOR), sel ? dat->author : "" );
				SetWindowTextA(GetDlgItem(hwndDlg,IDC_PLUGINEMAIL), sel ? dat->authorEmail : "" );
				{
					TCHAR* p = LangPackPcharToTchar( dat->description );
					SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINLONGINFO), sel ? p : _T(""));
					mir_free( p );
				}
				SetWindowTextA(GetDlgItem(hwndDlg,IDC_PLUGINCPYR), sel ? dat->copyright : "" );
				SetWindowTextA(GetDlgItem(hwndDlg,IDC_PLUGINURL), sel ? dat->homepage : "" );
				if(equalUUID(miid_last, dat->uuid))
					SetWindowText(GetDlgItem(hwndDlg,IDC_PLUGINPID), sel ? TranslateT("<none>") : _T(""));
				else
				{
					char szUID[128];
					uuidToString( dat->uuid, szUID, sizeof(szUID));
					SetWindowTextA(GetDlgItem(hwndDlg,IDC_PLUGINPID), sel ? szUID : "" );
				}
		}	}

		if ( hdr && hdr->hdr.code == PSN_APPLY ) {
			HWND hwndList=GetDlgItem(hwndDlg,IDC_PLUGLIST);
			int iRow;
			int iState;
			char buf[1024];
			for (iRow=0 ; iRow != (-1) ; ) {
				ListView_GetItemTextA(hwndList, iRow, 0, buf, SIZEOF(buf));
				iState=ListView_GetItemState(hwndList, iRow, LVIS_STATEIMAGEMASK);
				SetPluginOnWhiteList(buf, iState&0x2000 ? 1 : 0);
				iRow=ListView_GetNextItem(hwndList, iRow, LVNI_ALL);
		}	}
		break;
	}

	case WM_COMMAND:
		if ( HIWORD(wParam) == STN_CLICKED ) {
			switch (LOWORD(wParam)) {
				case IDC_PLUGINEMAIL:
				case IDC_PLUGINURL:
				{
					char buf[512];
					char * p = &buf[7];
					lstrcpyA(buf,"mailto:");
					if ( GetWindowTextA(GetDlgItem(hwndDlg, LOWORD(wParam)), p, SIZEOF(buf) - 7) ) {
						CallService(MS_UTILS_OPENURL,0,(LPARAM) (LOWORD(wParam)==IDC_PLUGINEMAIL ? buf : p) );
					}
					break;
		}	}	}
		break;

	case WM_DESTROY:
		RemoveAllItems( GetDlgItem( hwndDlg, IDC_PLUGLIST ));
		break;
	}
	return FALSE;
}

static int PluginOptionsInit(WPARAM wParam, LPARAM lParam)
{
	OPTIONSDIALOGPAGE odp = { 0 };
	odp.cbSize = sizeof(odp);
	odp.hInstance = GetModuleHandle(NULL);
	odp.pfnDlgProc = DlgPluginOpt;
	odp.pszTemplate = MAKEINTRESOURCEA(IDD_OPT_PLUGINS);
	odp.position = 1300000000;
	odp.pszTitle = LPGEN("Plugins");
	odp.flags = ODPF_BOLDGROUPS;
	CallService( MS_OPT_ADDPAGE, wParam, ( LPARAM )&odp );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Loads all plugins

int LoadNewPluginsModule(void)
{
	char exe[MAX_PATH];
	char* slice;
	pluginEntry* p;
	pluginEntry* clist = NULL;
	int useWhiteList, i;

	// make full path to the plugin
	GetModuleFileNameA(NULL, exe, SIZEOF(exe));
	slice=strrchr(exe, '\\');
	if ( slice != NULL )
		*slice=0;

	// remember some useful options
	askAboutIgnoredPlugins=(UINT) GetPrivateProfileIntA( "PluginLoader", "AskAboutIgnoredPlugins", 0, mirandabootini);

	// if freeimage is present, load it to provide the basic core functions
	if ( pluginList_freeimg != NULL ) {
		BASIC_PLUGIN_INFO bpi;
		mir_snprintf(slice,&exe[SIZEOF(exe)] - slice, "\\Plugins\\%s", pluginList_freeimg->pluginname);
		if ( checkAPI(exe, &bpi, mirandaVersion, CHECKAPI_NONE, NULL) ) {
			pluginList_freeimg->bpi = bpi;
			pluginList_freeimg->pclass |= PCLASS_OK | PCLASS_BASICAPI;
			if ( bpi.Load(&pluginCoreLink) == 0 )
				pluginList_freeimg->pclass |= PCLASS_LOADED;
			else
				Plugin_Uninit( pluginList_freeimg );
	}	}

	// first load the clist cos alot of plugins need that to be present at Load()
	for ( useWhiteList = 1; useWhiteList >= 0 && clist == NULL; useWhiteList-- )
		clist=getCListModule(exe, slice, useWhiteList);
	/* the loop above will try and get one clist DLL to work, if all fail then just bail now */
	if ( clist == NULL ) {
		// result = 0, no clist_* can be found
		if ( pluginListUI )
			MessageBox(0, TranslateT("Unable to start any of the installed contact list plugins, I even ignored your preferences for which contact list couldn't load any."), _T(""), MB_OK | MB_ICONINFORMATION);
		else
			MessageBox(0, TranslateT("Can't find a contact list plugin! you need clist_classic or clist_mw.") , _T(""), MB_OK | MB_ICONINFORMATION);
		return 1;
	}

	/* enable and disable as needed  */
	p=pluginListUI;
	while ( p != NULL ) {
		SetPluginOnWhiteList(p->pluginname, clist != p ? 0 : 1 );
		p = p->nextclass;
	}
	/* now loop thru and load all the other plugins, do this in one pass */

	for ( i=0; i < pluginList.realCount; i++ ) {
		p = pluginList.items[i];
		CharLowerA(p->pluginname);
		if ( !(p->pclass&PCLASS_LOADED) && !(p->pclass&PCLASS_DB)
			&& !(p->pclass&PCLASS_CLIST) && isPluginOnWhiteList(p->pluginname) ) {
			BASIC_PLUGIN_INFO bpi;
			mir_snprintf(slice,&exe[SIZEOF(exe)] - slice, "\\Plugins\\%s", p->pluginname);
			if ( checkAPI(exe, &bpi, mirandaVersion, CHECKAPI_NONE, NULL) ) {
				int rm = bpi.pluginInfo->replacesDefaultModule;
				p->bpi = bpi;
				p->pclass |= PCLASS_OK | PCLASS_BASICAPI;

				List_InsertPtr( &pluginListAddr, p );

				if ( pluginDefModList[rm] == NULL ) {
					if ( bpi.Load(&pluginCoreLink) == 0 ) p->pclass |= PCLASS_LOADED;
					else {
						Plugin_Uninit( p );
						i--;
					}
					if ( rm ) pluginDefModList[rm]=p;
				} //if
				else {
					SetPluginOnWhiteList( p->pluginname, 0 );
					Plugin_Uninit( p );
					i--;
				}
			}
			else p->pclass |= PCLASS_FAILED;
		}
		else if ( p->bpi.hInst != NULL )
			List_InsertPtr( &pluginListAddr, p );
	}

	HookEvent(ME_OPT_INITIALISE, PluginOptionsInit);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Plugins module initialization
//   called before anything real is loaded, incl. database

static int sttComparePlugins( pluginEntry* p1, pluginEntry* p2 )
{	return ( int )( p1->bpi.hInst - p2->bpi.hInst );
}

static int sttComparePluginsByName( pluginEntry* p1, pluginEntry* p2 )
{	return lstrcmpA( p1->pluginname, p2->pluginname );
}

int LoadNewPluginsModuleInfos(void)
{
	pluginList.increment = 10;
	pluginList.sortFunc = sttComparePluginsByName;

	pluginListAddr.increment = 10;
	pluginListAddr.sortFunc = sttComparePlugins;

	hPluginListHeap=HeapCreate(HEAP_NO_SERIALIZE, 0, 0);
	mirandaVersion = (DWORD)CallService(MS_SYSTEM_GETVERSION, 0, 0);
	//
	CreateServiceFunction(MS_PLUGINS_ENUMDBPLUGINS, PluginsEnum);
	CreateServiceFunction(MS_PLUGINS_GETDISABLEDEFAULTARRAY, PluginsGetDefaultArray);
	// make sure plugins can get internal core APIs
	pluginCoreLink.CallService                    = CallService;
	pluginCoreLink.ServiceExists                  = ServiceExists;
	pluginCoreLink.CreateServiceFunction          = CreateServiceFunction;
	pluginCoreLink.CreateTransientServiceFunction = CreateServiceFunction;
	pluginCoreLink.CreateServiceFunctionParam     = CreateServiceFunctionParam;
	pluginCoreLink.DestroyServiceFunction         = DestroyServiceFunction;
	pluginCoreLink.CreateHookableEvent            = CreateHookableEvent;
	pluginCoreLink.DestroyHookableEvent           = DestroyHookableEvent;
	pluginCoreLink.HookEvent                      = HookEvent;
	pluginCoreLink.HookEventParam                 = HookEventParam;
	pluginCoreLink.HookEventMessage               = HookEventMessage;
	pluginCoreLink.UnhookEvent                    = UnhookEvent;
	pluginCoreLink.NotifyEventHooks               = NotifyEventHooks;
	pluginCoreLink.SetHookDefaultForHookableEvent = SetHookDefaultForHookableEvent;
	pluginCoreLink.CallServiceSync                = CallServiceSync;
	pluginCoreLink.CallFunctionAsync              = CallFunctionAsync;
	pluginCoreLink.NotifyEventHooksDirect         = CallHookSubscribers;
	pluginCoreLink.CallProtoService               = CallProtoService;
	pluginCoreLink.CallContactService             = CallContactService;

	// remember where the mirandaboot.ini goes
	{
		char exe[MAX_PATH];
		char * slice;
		GetModuleFileNameA(NULL, exe, SIZEOF(exe));
		slice=strrchr(exe, '\\');
		if ( slice != NULL ) *slice=0;
		mir_snprintf(mirandabootini, SIZEOF(mirandabootini), "%s\\mirandaboot.ini", exe);
	}
	// look for all *.dll's
	enumPlugins(scanPluginsDir, 0, 0);
	// the database will select which db plugin to use, or fail if no profile is selected
	if (LoadDatabaseModule()) return 1;
	InitIni();
	//  could validate the plugin entries here but internal modules arent loaded so can't call Load() in one pass
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Plugins module unloading
//   called at the end of module chain unloading, just modular engine left at this point

int UnloadNewPluginsModule(void)
{
	int i;

	// unload everything but the DB
	for ( i = pluginList.realCount-1; i >= 0; i-- ) {
		pluginEntry* p = pluginList.items[i];
		if ( !(p->pclass & PCLASS_DB) )
			Plugin_Uninit( p );
	}

	// unload the DB
	for ( i = pluginList.realCount-1; i >= 0; i-- ) {
		pluginEntry * p = pluginList.items[i];
		Plugin_Uninit( p );
	}

	if ( hPluginListHeap ) HeapDestroy(hPluginListHeap);
	hPluginListHeap=0;

	List_Destroy( &pluginList );
	List_Destroy( &pluginListAddr );
	UninitIni();
	return 0;
}
