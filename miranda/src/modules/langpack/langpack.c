/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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
#include "../../core/commonheaders.h"

int LoadLangPackServices(void);

struct LangPackEntry {
	unsigned linePos;
	DWORD englishHash;
	char *english;	  //not currently used, the hash does everything
	char *local;
};

struct LangPackStruct {
	char filename[MAX_PATH];
	char language[64];
	char lastModifiedUsing[64];
	char authors[256];
	char authorEmail[128];
	struct LangPackEntry *entry;
	int entryCount;
} static langPack;

static void TrimString(char *str)
{
	int len,start;
	len=lstrlen(str);
	while(str[0] && (unsigned char)str[len-1]<=' ') str[--len]=0;
	for(start=0;str[start] && (unsigned char)str[start]<=' ';start++);
	MoveMemory(str,str+start,len-start+1);
}

static void TrimStringSimple(char *str) 
{
    if (str[lstrlen(str)-1] == '\n') str[lstrlen(str)-1] = '\0';
    if (str[lstrlen(str)-1] == '\r') str[lstrlen(str)-1] = '\0';
}

static int IsEmpty(char *str) {
    int i = 0;
    int len = lstrlen(str);

    while (str[i]) {
        if (!isspace(str[i])) return 0;
        i++;
    }
    return 1;
}

static void ConvertBackslashes(char *str)
{
	char *pstr;
	for(pstr=str;*pstr;pstr=CharNext(pstr)) {
		if(*pstr=='\\') {
			switch(pstr[1]) {
				case 'n': *pstr='\n'; break;
				case 't': *pstr='\t'; break;
				default: *pstr=pstr[1]; break;
			}
			MoveMemory(pstr+1,pstr+2,lstrlen(pstr+2)+1);
		}
	}
}

static DWORD LangPackHash(const char *szStr)
{
#if defined _M_IX86 && !defined _NUMEGA_BC_FINALCHECK && !defined __GNUC__
	__asm {				//this is mediocrely optimised, but I'm sure it's good enough
		xor  edx,edx
		mov  esi,szStr
		xor  cl,cl
	lph_top:
		xor  eax,eax
		and  cl,31
		mov  al,[esi]
		inc  esi
		test al,al
		jz   lph_end
		rol  eax,cl
		add  cl,5
		xor  edx,eax
		jmp  lph_top
	lph_end:
		mov  eax,edx
	}
#else
	DWORD hash=0;
	int i;
	int shift=0;
	for(i=0;szStr[i];i++) {
		hash^=szStr[i]<<shift;
		if(shift>24) hash^=(szStr[i]>>(32-shift))&0x7F;
		shift=(shift+5)&0x1F;
	}
	return hash;
#endif
}

static int SortLangPackHashesProc(struct LangPackEntry *arg1,struct LangPackEntry *arg2)
{
	if(arg1->englishHash<arg2->englishHash) return -1;
	if(arg1->englishHash>arg2->englishHash) return 1;
	/* both source strings of the same hash (may not be the same string thou) put
	the one that was written first to be found first */
	if(arg1->linePos<arg2->linePos) return -1;
	if(arg1->linePos>arg2->linePos) return 1;
	return 0;
}


static int SortLangPackHashesProc2(struct LangPackEntry *arg1,struct LangPackEntry *arg2)
{
	if(arg1->englishHash<arg2->englishHash) return -1;
	if(arg1->englishHash>arg2->englishHash) return 1;
	return 0;
}

static int LoadLangPack(const char *szLangPack)
{
	FILE *fp;
	char line[4096];
	char *pszColon;
    char *pszLine;
	int entriesAlloced;
	int startOfLine=0;
	unsigned int linePos=1;

	lstrcpy(langPack.filename,szLangPack);
	fp=fopen(szLangPack,"rt");
	if(fp==NULL) return 1;
	fgets(line,sizeof(line),fp);
	TrimString(line);
	if(lstrcmp(line,"Miranda Language Pack Version 1")) {fclose(fp); return 2;}
	//headers
	while(!feof(fp)) {
		startOfLine=ftell(fp);
		if(fgets(line,sizeof(line),fp)==NULL) break;
        TrimString(line);
		if(IsEmpty(line) || line[0]==';' || line[0]==0) continue;
		if(line[0]=='[') break;
		pszColon=strchr(line,':');
		if(pszColon==NULL) {fclose(fp); return 3;}
		*pszColon=0;
		if(!lstrcmp(line,"Language")) {lstrcpy(langPack.language,pszColon+1); TrimString(langPack.language);}
		else if(!lstrcmp(line,"Last-Modified-Using")) {lstrcpy(langPack.lastModifiedUsing,pszColon+1); TrimString(langPack.lastModifiedUsing);}
		else if(!lstrcmp(line,"Authors")) {lstrcpy(langPack.authors,pszColon+1); TrimString(langPack.authors);}
		else if(!lstrcmp(line,"Author-email")) {lstrcpy(langPack.authorEmail,pszColon+1); TrimString(langPack.authorEmail);}
	}
	//body
	fseek(fp,startOfLine,SEEK_SET);
	entriesAlloced=0;
	while(!feof(fp)) {
		if(fgets(line,sizeof(line),fp)==NULL) break;
		if(IsEmpty(line) || line[0]==';' || line[0]==0) continue;
        TrimStringSimple(line);
		ConvertBackslashes(line);
        if(line[0]=='[' && line[lstrlen(line)-1]==']') {
			if(langPack.entryCount && langPack.entry[langPack.entryCount-1].local==NULL) {
				if(langPack.entry[langPack.entryCount-1].english!=NULL) free(langPack.entry[langPack.entryCount-1].english);
				langPack.entryCount--;
			}
            pszLine = line+1;
			line[lstrlen(line)-1]='\0';
			TrimStringSimple(line);
			if(++langPack.entryCount>entriesAlloced) {
				entriesAlloced+=128;
				langPack.entry=(struct LangPackEntry*)realloc(langPack.entry,sizeof(struct LangPackEntry)*entriesAlloced);
			}
			langPack.entry[langPack.entryCount-1].english=NULL;
			langPack.entry[langPack.entryCount-1].englishHash=LangPackHash(pszLine);
			langPack.entry[langPack.entryCount-1].local=NULL;
			langPack.entry[langPack.entryCount-1].linePos=linePos++;
		}
		else if(langPack.entryCount) {
			if(langPack.entry[langPack.entryCount-1].local==NULL)
				langPack.entry[langPack.entryCount-1].local=_strdup(line);
			else {
				langPack.entry[langPack.entryCount-1].local=(char*)realloc(langPack.entry[langPack.entryCount-1].local,lstrlen(langPack.entry[langPack.entryCount-1].local)+lstrlen(line)+2);
				lstrcat(langPack.entry[langPack.entryCount-1].local,"\n");
				lstrcat(langPack.entry[langPack.entryCount-1].local,line);
			}
		}
	}
	qsort(langPack.entry,langPack.entryCount,sizeof(struct LangPackEntry),(int(*)(const void*,const void*))SortLangPackHashesProc);
	fclose(fp);
	return 0;
}

char *LangPackTranslateString(const char *szEnglish)
{
	struct LangPackEntry key,*entry;

	if ( langPack.entryCount == 0 || szEnglish == NULL ) return (char*)szEnglish;

	key.englishHash=LangPackHash(szEnglish);
	entry=(struct LangPackEntry*)bsearch(&key,langPack.entry,langPack.entryCount,sizeof(struct LangPackEntry),(int(*)(const void*,const void*))SortLangPackHashesProc2);
	if(entry==NULL) return (char*)szEnglish;
	while(entry>langPack.entry)
	{
		entry--;
		if(entry->englishHash!=key.englishHash) {
			entry++;
			return entry->local;
		}
	}
	return entry->local;
}

static int LangPackShutdown(WPARAM wParam,LPARAM lParam)
{
	int i;
	for(i=0;i<langPack.entryCount;i++) {
		if(langPack.entry[i].english!=NULL) free(langPack.entry[i].english);
		if(langPack.entry[i].local!=NULL) { free(langPack.entry[i].local); }
	}
	if(langPack.entryCount) {
		free(langPack.entry);
		langPack.entry=0;
		langPack.entryCount=0;
	}
	return 0;
}

int LoadLangPackModule(void)
{
	HANDLE hFind;
	char szSearch[MAX_PATH],*str2,szLangPack[MAX_PATH];
	WIN32_FIND_DATA fd;

	ZeroMemory(&langPack,sizeof(langPack));
	HookEvent(ME_SYSTEM_SHUTDOWN,LangPackShutdown);
	LoadLangPackServices();
	GetModuleFileName(GetModuleHandle(NULL),szSearch,sizeof(szSearch));
	str2=strrchr(szSearch,'\\');
	if(str2!=NULL) *str2=0;
	else str2=szSearch;
	lstrcat(szSearch,"\\langpack_*.txt");
	hFind=FindFirstFile(szSearch,&fd);
	if(hFind!=INVALID_HANDLE_VALUE) {
		lstrcpy(str2+1,fd.cFileName);
		lstrcpy(szLangPack,szSearch);
		FindClose(hFind);
		LoadLangPack(szLangPack);
	}
	return 0;
}
