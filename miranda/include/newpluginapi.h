/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2007 Miranda ICQ/IM project, 
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

#ifndef M_NEWPLUGINAPI_H__
#define M_NEWPLUGINAPI_H__

#include "m_plugins.h"

#define PLUGIN_MAKE_VERSION(a,b,c,d)   (((((DWORD)(a))&0xFF)<<24)|((((DWORD)(b))&0xFF)<<16)|((((DWORD)(c))&0xFF)<<8)|(((DWORD)(d))&0xFF))
#define MAXMODULELABELLENGTH 64

#if defined( _UNICODE )
	#define UNICODE_AWARE 1
#else
	#define UNICODE_AWARE 0
#endif

typedef struct {
	int cbSize;
	char *shortName;
	DWORD version;
	char *description;
	char *author;
	char *authorEmail;
	char *copyright;
	char *homepage;
	BYTE flags;	   // right now the only flag, UNICODE_AWARE, is recognized here
	int replacesDefaultModule;		   //one of the DEFMOD_ constants in m_plugins.h or zero
	         //if non-zero, this will supress the loading of the specified built-in module
			 //with the implication that this plugin provides back-end-compatible features
} PLUGININFO;

/* 0.7+ 
   New plugin loader implementation
*/
/* The UUID structure below is used to for plugin UUID's and module type definitions */
typedef struct _MUUID {
  unsigned long a;
  unsigned short b;
  unsigned short c;
  unsigned char d[8];
} MUUID;


/* Used to define the end of the MirandaPluginInterface list */
#define MIID_LAST  {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}}

/* TODO: Add replaceable module interface ids */
/* TODO: Add common external plugin interfaces */

#define MIID_DATABASE       {0xae77fd33, 0xe484, 0x4dc7, {0x8c, 0xbc, 0x9, 0x9f, 0xed, 0xcc, 0xcf, 0xdd}}
#define MIID_CLIST          {0x9d8da8bf, 0x665b, 0x4908, {0x9e, 0x61, 0x9f, 0x75, 0x98, 0xae, 0x33, 0xe}}
#define MIID_CHAT           {0x23576a43, 0x3a26, 0x4357, {0x9b, 0x1b, 0x4a, 0x71, 0x9e, 0x42, 0x5d, 0x48}}
#define MIID_SRMM           {0x58c7eea6, 0xf9db, 0x4dd9, {0x80, 0x36, 0xae, 0x80, 0x2b, 0xc0, 0x41, 0x4c}}
#define MIID_IMPORT         {0x5f3bcad4, 0x75f8, 0x476e, {0xb3, 0x6b, 0x2b, 0x30, 0x70, 0x32, 0x49, 0xc}}
#define MIID_IMGSERVICES    {0xf3974915, 0xc9d5, 0x4c87, {0x85, 0x64, 0xa0, 0xeb, 0xf9, 0xd2, 0x5a, 0xa0}}
#define MIID_TEST           {0x53b974f4, 0x3c74, 0x4dba, {0x8f, 0xc2, 0x6f, 0x92, 0xfe, 0x1, 0x3b, 0x8c}}

#define MIID_HISTORY        {0x5ca0cbc1, 0x999a, 0x4ea2, {0x8b, 0x44, 0xf8, 0xf6, 0x7d, 0x7f, 0x8e, 0xbe}}

#define MIID_VERSIONINFO    {0xcfeb6325, 0x334e, 0x4052, {0xa6, 0x45, 0x56, 0x21, 0x93, 0xdf, 0xcc, 0x77}}
#define MIID_FOLDERS        {0xcfebec29, 0x39ef, 0x4b62, {0xad, 0x38, 0x9a, 0x65, 0x2c, 0xa3, 0x24, 0xed}}
#define MIID_BIRTHDAYNOTIFY {0xcfba5784, 0x3701, 0x4d83, {0x81, 0x6a, 0x19, 0x9c, 0x00, 0xd4, 0xa6, 0x7a}}

#define MIID_BONSAI         {0xcfaae811, 0x30e1, 0x4a4f, {0x87, 0x84, 0x15, 0x3c, 0xcc, 0xb0, 0x03, 0x7a}}
#define MIID_EXCHANGE       {0xcfd79a89, 0x9959, 0x4e65, {0xb0, 0x76, 0x41, 0x3f, 0x98, 0xfe, 0x0d, 0x15}}
#define MIID_MIRPY          {0xcff91a5c, 0x1786, 0x41c1, {0x88, 0x86, 0x09, 0x4b, 0x14, 0x28, 0x1f, 0x15}}
#define MIID_SERVICESLIST   {0xcf4bdf02, 0x5d27, 0x4241, {0x99, 0xe5, 0x19, 0x51, 0xaa, 0xb0, 0xc4, 0x54}}
#define MIID_TRANSLATOR     {0xcfb637b0, 0x7217, 0x4c1e, {0xb2, 0x2a, 0xd9, 0x22, 0x32, 0x3a, 0x5d, 0x0b}}

/* Special exception interface for protocols.
   This interface allows more than one plugin to implement it at the same time
*/
#define MIID_PROTOCOL    {0x2a3c815e, 0xa7d9, 0x424b, {0xba, 0x30, 0x2, 0xd0, 0x83, 0x22, 0x90, 0x85}}


typedef struct {
	int cbSize;
	char *shortName;
	DWORD version;
	char *description;
	char *author;
	char *authorEmail;
	char *copyright;
	char *homepage;
	BYTE flags;	   // right now the only flag, UNICODE_AWARE, is recognized here
	int replacesDefaultModule;		   //one of the DEFMOD_ constants in m_plugins.h or zero
	         //if non-zero, this will supress the loading of the specified built-in module
			 //with the implication that this plugin provides back-end-compatible features
             /***********  WILL BE DEPRECATED in 0.8 * *************/
    MUUID uuid; // Not required until 0.8.
} PLUGININFOEX;

#ifndef MODULES_H_
typedef int (*MIRANDAHOOK)(WPARAM,LPARAM);
typedef int (*MIRANDASERVICE)(WPARAM,LPARAM);
#define CALLSERVICE_NOTFOUND      ((int)0x80000000)
#endif

//see modules.h for what all this stuff is
typedef struct {
	HANDLE (*CreateHookableEvent)(const char *);
	int (*DestroyHookableEvent)(HANDLE);
	int (*NotifyEventHooks)(HANDLE,WPARAM,LPARAM);
	HANDLE (*HookEvent)(const char *,MIRANDAHOOK);
	HANDLE (*HookEventMessage)(const char *,HWND,UINT);
	int (*UnhookEvent)(HANDLE);
	HANDLE (*CreateServiceFunction)(const char *,MIRANDASERVICE);
	HANDLE (*CreateTransientServiceFunction)(const char *,MIRANDASERVICE);
	int (*DestroyServiceFunction)(HANDLE);
	int (*CallService)(const char *,WPARAM,LPARAM);
	int (*ServiceExists)(const char *);		  //v0.1.0.1+
	int (*CallServiceSync)(const char *,WPARAM,LPARAM);		//v0.3.3+
	int (*CallFunctionAsync) (void (__stdcall *)(void *), void *);	//v0.3.4+
	int (*SetHookDefaultForHookableEvent) (HANDLE, MIRANDAHOOK); // v0.3.4 (2004/09/15)
} PLUGINLINK;

#ifndef MODULES_H_
#ifndef NODEFINEDLINKFUNCTIONS
//relies on a global variable 'pluginLink' in the plugins
extern PLUGINLINK *pluginLink;
#define CreateHookableEvent(a)               pluginLink->CreateHookableEvent(a)
#define DestroyHookableEvent(a)              pluginLink->DestroyHookableEvent(a)
#define NotifyEventHooks(a,b,c)              pluginLink->NotifyEventHooks(a,b,c)
#define HookEventMessage(a,b,c)              pluginLink->HookEventMessage(a,b,c)
#define HookEvent(a,b)                       pluginLink->HookEvent(a,b)
#define UnhookEvent(a)                       pluginLink->UnhookEvent(a)
#define CreateServiceFunction(a,b)           pluginLink->CreateServiceFunction(a,b)
#define CreateTransientServiceFunction(a,b)  pluginLink->CreateTransientServiceFunction(a,b)
#define DestroyServiceFunction(a)            pluginLink->DestroyServiceFunction(a)
#define CallService(a,b,c)                   pluginLink->CallService(a,b,c)
#define ServiceExists(a)                     pluginLink->ServiceExists(a)
#define CallServiceSync(a,b,c)               pluginLink->CallServiceSync(a,b,c)
#define CallFunctionAsync(a,b)				 pluginLink->CallFunctionAsync(a,b)
#define SetHookDefaultForHookableEvent(a,b)  pluginLink->SetHookDefaultForHookableEvent(a,b)
#endif
#endif

/*
 Database plugin stuff
*/

// grokHeader() error codes
#define EGROKPRF_NOERROR	0
#define EGROKPRF_CANTREAD	1	// can't open the profile for reading
#define EGROKPRF_UNKHEADER  2	// header not supported, not a supported profile
#define EGROKPRF_VERNEWER   3	// header correct, version in profile newer than reader/writer
#define EGROKPRF_DAMAGED	4	// header/version fine, other internal data missing, damaged.

// makeDatabase() error codes
#define EMKPRF_CREATEFAILED 1   // for some reason CreateFile() didnt like something

typedef struct {
	int cbSize;
	
	/*
	returns what the driver can do given the flag
	*/
	int (*getCapability) ( int flag );

	/* 
		buf: pointer to a string buffer
		cch: length of buffer
		shortName: if true, the driver should return a short but descriptive name, e.g. "3.xx profile"
		Affect: The database plugin must return a "friendly name" into buf and not exceed cch bytes,
			e.g. "Database driver for 3.xx profiles"
		Returns: 0 on success, non zero on failure
	*/
	int (*getFriendlyName) ( char * buf, size_t cch, int shortName );

	/*
		profile: pointer to a string which contains full path + name
		Affect: The database plugin should create the profile, the filepath will not exist at
			the time of this call, profile will be C:\..\<name>.dat
		Note: Do not prompt the user in anyway about this operation.
		Note: Do not initialise internal data structures at this point!
		Returns: 0 on success, non zero on failure - error contains extended error information, see EMKPRF_*
	*/
	int (*makeDatabase) ( char * profile, int * error );

	/*
		profile: [in] a null terminated string to file path of selected profile
		error: [in/out] pointer to an int to set with error if any
		Affect: Ask the database plugin if it supports the given profile, if it does it will
			return 0, if it doesnt return 1, with the error set in error -- EGROKPRF_* can be valid error
			condition, most common error would be [EGROKPRF_UNKHEADER] 
		Note: Just because 1 is returned, doesnt mean the profile is not supported, the profile might be damaged
			etc.
		Returns: 0 on success, non zero on failure
	*/
	int (*grokHeader) ( char * profile, int * error );

	/*
	Affect: Tell the database to create all services/hooks that a 3.xx legecy database might support into link,
		which is a PLUGINLINK structure
	Returns: 0 on success, nonzero on failure
	*/
	int (*Load) ( char * profile, void * link );

	/*
	Affect: The database plugin should shutdown, unloading things from the core and freeing internal structures
	Returns: 0 on success, nonzero on failure
	Note: Unload() might be called even if Load() was never called, wasLoaded is set to 1 if Load() was ever called.
	*/
	int (*Unload) ( int wasLoaded );

} DATABASELINK;

#endif // M_NEWPLUGINAPI_H__


