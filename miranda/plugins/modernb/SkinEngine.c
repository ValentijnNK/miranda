/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project, 
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

//Include
#include <assert.h>

#include "commonheaders.h" 
#include <stdio.h>
//#include <Wingdi.h>
#include "m_skin_eng.h"
#include "mod_skin_selector.h"
//#include "commonheaders.h" 
#include "CLUIFRAMES\cluiframes.h"

#include <m_png.h>

#define _EFFECTENUM_FULL_H
#include "effectenum.h"
#undef _EFFECTENUM_FULL_H
#include "SkinEngine.h" 
//Implementation

#include "commonprototypes.h"
#include "shlwapi.h"
#include "math.h"


/* Global variables */

SKINOBJECTSLIST g_SkinObjectList={0};
CURRWNDIMAGEDATA * g_pCachedWindow=NULL;

BOOL (WINAPI *g_proc_UpdateLayeredWindow)(HWND,HDC,POINT*,SIZE*,HDC,POINT*,COLORREF,BLENDFUNCTION*,DWORD);

BOOL    g_flag_bPostWasCanceled =FALSE,
		g_flag_bFullRepaint     =FALSE;

BOOL    g_mutex_bLockUpdating   =FALSE;

SortedList * gl_plGlyphTexts=NULL,
* gl_plSkinFonts =NULL;

/* Private module variables */

static GLYPHIMAGE * pLoadedImages=NULL;
static DWORD        dwLoadedImagesCount=0;
static DWORD        dwLoadedImagesAlocated=0;
static HANDLE       hEventServicesCreated=NULL;

static BOOL flag_bUpdateQueued=FALSE,
flag_bJustDrawNonFramedObjects=FALSE;

static BOOL mutex_bLockUpdate=FALSE;

static SortedList       * pEffectStack=NULL;
static SKINOBJECTSLIST  * pCurrentSkin=NULL;
static char            ** pszSettingName=NULL;
static int                nArrayLen=0;
static char             * iniCurrentSection=NULL;
static char             * szFileName=NULL;

static BYTE             pbGammaWeight[256]={0};
static BYTE             pbGammaWeightAdv[256]={0};
static BOOL             bGammaWeightFilled=FALSE;

static CRITICAL_SECTION cs_SkinChanging={0};


/* Private module procedures */
static BOOL SkinEngine_GetMaskBit(BYTE *line, int x);
static int  SkinEngine_Service_AlphaTextOut(WPARAM wParam,LPARAM lParam);
static BOOL SkinEngine_Service_DrawIconEx(WPARAM wParam,LPARAM lParam);
static int  SkinEngine_Service_RegisterFramePaintCallbackProcedure(WPARAM wParam, LPARAM lParam);


static int  SkinEngine_AlphaTextOut (HDC hDC, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format, DWORD ARGBcolor);
static void SkinEngine_AddParseTextGlyphObject(char * szGlyphTextID,char * szDefineString,SKINOBJECTSLIST *Skin);
static void SkinEngine_AddParseSkinFont(char * szFontID,char * szDefineString,SKINOBJECTSLIST *Skin);
static int  SkinEngine_DeleteAllSettingInSection(char * SectionName);
static int  SkinEngine_GetSkinFromDB(char * szSection, SKINOBJECTSLIST * Skin);
static LPSKINOBJECTDESCRIPTOR SkinEngine_FindObject(const char * szName, BYTE objType,SKINOBJECTSLIST* Skin);
static HBITMAP SkinEngine_LoadGlyphImageByDecoders(char * szFileName);
static int  SkinEngine_LoadSkinFromResource();
static void SkinEngine_PreMultiplyChanells(HBITMAP hbmp,BYTE Mult);
static int  SkinEngine_ValidateSingleFrameImage(wndFrame * Frame, BOOL SkipBkgBlitting);


//Decoders
static HMODULE hImageDecoderModule;

typedef  DWORD  (__stdcall *pfnImgNewDecoder)(void ** ppDecoder); 
static pfnImgNewDecoder ImgNewDecoder;

typedef DWORD (__stdcall *pfnImgDeleteDecoder)(void * pDecoder);
static pfnImgDeleteDecoder ImgDeleteDecoder;

typedef  DWORD  (__stdcall *pfnImgNewDIBFromFile)(LPVOID /*in*/pDecoder, LPCSTR /*in*/pFileName, LPVOID /*out*/*pImg);
static pfnImgNewDIBFromFile ImgNewDIBFromFile;

typedef DWORD (__stdcall *pfnImgDeleteDIBSection)(LPVOID /*in*/pImg);
static pfnImgDeleteDIBSection ImgDeleteDIBSection;

typedef DWORD (__stdcall *pfnImgGetHandle)(LPVOID /*in*/pImg, HBITMAP /*out*/*pBitmap, LPVOID /*out*/*ppDIBBits);
static pfnImgGetHandle ImgGetHandle;    

static MODERNEFFECT meCurrentEffect={-1,{0},0,0};

BOOL SkinEngine_AlphaBlend(HDC hdcDest,int nXOriginDest,int nYOriginDest,int nWidthDest,int nHeightDest,HDC hdcSrc,int nXOriginSrc,int nYOriginSrc,int nWidthSrc,int nHeightSrc,BLENDFUNCTION blendFunction)
{
    if (!g_CluiData.fGDIPlusFail && blendFunction.BlendFlags&128 ) //Use gdi+ engine
    {
        return GDIPlus_AlphaBlend( hdcDest,nXOriginDest,nYOriginDest,nWidthDest,nHeightDest,
            hdcSrc,nXOriginSrc,nYOriginSrc,nWidthSrc,nHeightSrc,
            &blendFunction);
    }
    blendFunction.BlendFlags&=~128;
    return AlphaBlend(hdcDest,nXOriginDest,nYOriginDest,nWidthDest,nHeightDest,hdcSrc,nXOriginSrc,nYOriginSrc,nWidthSrc,nHeightSrc,blendFunction);
}


static int SkinEngine_LockSkin()
{
    EnterCriticalSection(&cs_SkinChanging);
    return 0;
}
static int SkinEngine_UnlockSkin()
{
    LeaveCriticalSection(&cs_SkinChanging);
    return 0;
}

int SkinEngine_LoadModule()
{
    InitializeCriticalSection(&cs_SkinChanging);
    MainModernMaskList=mir_alloc(sizeof(LISTMODERNMASK));
    memset(MainModernMaskList,0,sizeof(LISTMODERNMASK));   
    //init variables
    g_SkinObjectList.dwObjLPAlocated=0;
    g_SkinObjectList.dwObjLPReserved=0;
    g_SkinObjectList.pObjects=NULL;
    // Initialize GDI+
    InitGdiPlus();
    //load decoder
    hImageDecoderModule=NULL;
    if (g_CluiData.fGDIPlusFail)
    {
        hImageDecoderModule = LoadLibrary(TEXT("ImgDecoder.dll"));
        if (hImageDecoderModule==NULL) 
        {
            char tDllPath[ MAX_PATH ];
            GetModuleFileNameA( g_hInst, tDllPath, sizeof( tDllPath ));
            {
                char* p = strrchr( tDllPath, '\\' );
                if ( p != NULL )
                    strcpy( p+1, "ImgDecoder.dll" );
                else
                {
                    strcpy( tDllPath, "ImgDecoder.dll" );
                }
            }

            hImageDecoderModule = LoadLibraryA(tDllPath);
        }
        if (hImageDecoderModule!=NULL) 
        {
            ImgNewDecoder=(pfnImgNewDecoder )GetProcAddress( hImageDecoderModule, "ImgNewDecoder");
            ImgDeleteDecoder=(pfnImgDeleteDecoder )GetProcAddress( hImageDecoderModule, "ImgDeleteDecoder");
            ImgNewDIBFromFile=(pfnImgNewDIBFromFile)GetProcAddress( hImageDecoderModule, "ImgNewDIBFromFile");
            ImgDeleteDIBSection=(pfnImgDeleteDIBSection)GetProcAddress( hImageDecoderModule, "ImgDeleteDIBSection");
            ImgGetHandle=(pfnImgGetHandle)GetProcAddress( hImageDecoderModule, "ImgGetHandle");
        }




    }
    //create services
    {       
        //  CreateServiceFunction(MS_SKIN_REGISTEROBJECT,SkinEngine_RegisterObject);
        CreateServiceFunction(MS_SKIN_DRAWGLYPH,SkinEngine_Service_DrawGlyph);
        //    CreateServiceFunction(MS_SKIN_REGISTERDEFOBJECT,ServCreateGlyphedObjectDefExt);
        CreateServiceFunction(MS_SKINENG_REGISTERPAINTSUB,SkinEngine_Service_RegisterFramePaintCallbackProcedure);
        CreateServiceFunction(MS_SKINENG_UPTATEFRAMEIMAGE,SkinEngine_Service_UpdateFrameImage);
        CreateServiceFunction(MS_SKINENG_INVALIDATEFRAMEIMAGE,SkinEngine_Service_InvalidateFrameImage);

        CreateServiceFunction(MS_SKINENG_ALPHATEXTOUT,SkinEngine_Service_AlphaTextOut);
        //		CreateServiceFunction(MS_SKINENG_IL_REPLACEICONFIX,ImageList_ReplaceIcon_FixAlphaServ);
        //CreateServiceFunction(MS_SKINENG_IL_ADDICONFIX,ImageList_AddIcon_FixAlphaServ);
        //CreateServiceFunction(MS_SKINENG_IL_ALPHAFIX,FixAlphaServ);
        CreateServiceFunction(MS_SKINENG_DRAWICONEXFIX,SkinEngine_Service_DrawIconEx);
    }
    //create event handle
    hEventServicesCreated=CreateHookableEvent(ME_SKIN_SERVICESCREATED);
    g_hSkinLoadedEvent=HookEvent(ME_SKIN_SERVICESCREATED,CLUI_OnSkinLoad);



    // if (DBGetContactSettingByte(NULL,"CLUI","StoreNotUsingElements",0))
    //     SkinEngine_GetSkinFromDB(DEFAULTSKINSECTION,&g_SkinObjectList);

    //notify services created
    {
        int t=NotifyEventHooks(hEventServicesCreated,0,0);
        t=t;

    }

    return 1;
}

int SkinEngine_UnloadModule()
{
    //unload services
    ModernButton_UnloadModule(0,0);
    SkinEngine_UnloadSkin(&g_SkinObjectList);
    if (g_SkinObjectList.pObjects) 
        mir_free_and_nill(g_SkinObjectList.pObjects);
    if (g_SkinObjectList.pMaskList) 
        mir_free_and_nill(g_SkinObjectList.pMaskList);
    if (MainModernMaskList)
        mir_free_and_nill(MainModernMaskList);
    if (pEffectStack)
    {
        int i;
        for (i=0; i<pEffectStack->realCount; i++)
            if (pEffectStack->items[i])
            {
                EFFECTSSTACKITEM * effect=(EFFECTSSTACKITEM*)(pEffectStack->items[i]);
                mir_free_and_nill(effect);
            }
            li.List_Destroy(pEffectStack);
            mir_free_and_nill(pEffectStack);
    }
    if (g_pCachedWindow)
    {
        SelectObject(g_pCachedWindow->hBackDC,g_pCachedWindow->hBackOld);
        SelectObject(g_pCachedWindow->hImageDC,g_pCachedWindow->hImageOld);
        DeleteObject(g_pCachedWindow->hBackDIB);
        DeleteObject(g_pCachedWindow->hImageDIB);
        mod_DeleteDC(g_pCachedWindow->hBackDC);
        mod_DeleteDC(g_pCachedWindow->hImageDC);
        ReleaseDC(NULL,g_pCachedWindow->hScreenDC);
        mir_free_and_nill(g_pCachedWindow);
        g_pCachedWindow=NULL;
    }
    DeleteCriticalSection(&cs_SkinChanging);
    GdiFlush();
    DestroyServiceFunction(MS_SKIN_REGISTEROBJECT);
    DestroyServiceFunction(MS_SKIN_DRAWGLYPH);
    DestroyHookableEvent(hEventServicesCreated);
    if (hImageDecoderModule) FreeLibrary(hImageDecoderModule);
    ShutdownGdiPlus();
    //free variables
    return 1;
}


BOOL SkinEngine_SetRgnOpaque(HDC memdc,HRGN hrgn)
{
    RGNDATA * rdata;
    DWORD rgnsz;
    DWORD d;
    RECT * rect;
    rgnsz=GetRegionData(hrgn,0,NULL);
    rdata=(RGNDATA *) mir_alloc(rgnsz);
    GetRegionData(hrgn,rgnsz,rdata);
    rect=(RECT *)rdata->Buffer;
    for (d=0; d<rdata->rdh.nCount; d++)
    {
        SkinEngine_SetRectOpaque(memdc,&rect[d]);
    }
    mir_free_and_nill(rdata);
    return TRUE;
}

BOOL SkinEngine_SetRectOpaque(HDC memdc,RECT *fr)
{
    int x,y;
    int sx,sy,ex,ey;
    int f=0;
    BYTE * bits;
    BITMAP bmp;
    HBITMAP hbmp=GetCurrentObject(memdc,OBJ_BITMAP);  
    GetObject(hbmp, sizeof(bmp),&bmp);
    sx=(fr->left>0)?fr->left:0;
    sy=(fr->top>0)?fr->top:0;
    ex=(fr->right<bmp.bmWidth)?fr->right:bmp.bmWidth;
    ey=(fr->bottom<bmp.bmHeight)?fr->bottom:bmp.bmHeight;
    if (!bmp.bmBits)
    {
        f=1;
        bits=malloc(bmp.bmWidthBytes*bmp.bmHeight);
        GetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);
    }
    else
        bits=bmp.bmBits;
    for (y=sy;y<ey;y++)
        for (x=sx;x<ex;x++)
            (((BYTE*)bits)+(bmp.bmHeight-y-1)*bmp.bmWidthBytes+x*4)[3]=255;
    if (f)
    {
        SetBitmapBits(hbmp,bmp.bmWidthBytes*bmp.bmHeight,bits);    
        free(bits);
    }
    // DeleteObject(hbmp);
    return 1;
}

static BOOL SkinEngine_SkinFillRectByGlyph(HDC hDest, HDC hSource, RECT * rFill, RECT * rGlyph, RECT * rClip, BYTE mode, BYTE drawMode, int depth)
{
    int destw=0, desth=0;
    int xstart=0, xmax=0;
    int ystart=0, ymax=0;
    BLENDFUNCTION bfa={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA }; 
    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA }; 
    //     int res;
    //initializations
    // SetStretchBltMode(hDest, HALFTONE);
    if (mode==FM_STRETCH)
    {
        HDC mem2dc;
        HBITMAP mem2bmp, oldbmp;
        RECT wr;
        IntersectRect(&wr,rClip,rFill);
        if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;       
        if (drawMode!=2)
        {
            mem2dc=CreateCompatibleDC(hDest);
            mem2bmp=SkinEngine_CreateDIB32(wr.right-wr.left,wr.bottom-wr.top);
            oldbmp=SelectObject(mem2dc,mem2bmp);

        }

        if (drawMode==0 || drawMode==2)
        {
            if (drawMode==0)
            {
                //   SetStretchBltMode(mem2dc, HALFTONE);
                SkinEngine_AlphaBlend(mem2dc,rFill->left-wr.left,rFill->top-wr.top,rFill->right-rFill->left,rFill->bottom-rFill->top,
                    hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf);
                SkinEngine_AlphaBlend(hDest,wr.left,wr.top,wr.right-wr.left, wr.bottom -wr.top,mem2dc,0,0,wr.right-wr.left, wr.bottom -wr.top,bf);
            }
            else 
            {
                SkinEngine_AlphaBlend(hDest,rFill->left,rFill->top,rFill->right-rFill->left,rFill->bottom-rFill->top,
                    hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf);

            }
        }
        else
        {
            //            BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
            SkinEngine_AlphaBlend(mem2dc,rFill->left-wr.left,rFill->top-wr.top,rFill->right-rFill->left,rFill->bottom-rFill->top,
                hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,rGlyph->bottom-rGlyph->top,bf); 
            SkinEngine_AlphaBlend(hDest,wr.left,wr.top,wr.right-wr.left, wr.bottom -wr.top,mem2dc,0,0,wr.right-wr.left, wr.bottom -wr.top,bf);
        }
        if (drawMode!=2)
        {
            SelectObject(mem2dc,oldbmp);
            DeleteObject(mem2bmp);
            mod_DeleteDC(mem2dc);
        }
        return 1;
    }
    else if (mode==FM_TILE_VERT && (rGlyph->bottom-rGlyph->top>0)&& (rGlyph->right-rGlyph->left>0))
    {
        HDC mem2dc;
        HBITMAP mem2bmp,oldbmp;
        RECT wr;
        IntersectRect(&wr,rClip,rFill);
        if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
        mem2dc=CreateCompatibleDC(hDest);
        //SetStretchBltMode(mem2dc, HALFTONE);
        mem2bmp=SkinEngine_CreateDIB32(wr.right-wr.left, rGlyph->bottom-rGlyph->top);
        oldbmp=SelectObject(mem2dc,mem2bmp);
        if (!oldbmp) 
            return 0;
        //MessageBoxA(NULL,"Tile bitmap not selected","ERROR", MB_OK);
        /// draw here
        {
            int  y=0, sy=0, maxy=0;
            int w=rFill->right-rFill->left;
            int h=rGlyph->bottom-rGlyph->top;
            if (h>0 && (wr.bottom-wr.top)*(wr.right-wr.left)!=0)
            {
                w=wr.right-wr.left;
                {
                    //                   BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
                    SkinEngine_AlphaBlend(mem2dc,-(wr.left-rFill->left),0,rFill->right-rFill->left,h,hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,h,bf);
                    //StretchBlt(mem2dc,-(wr.left-rFill->left),0,rFill->right-rFill->left,h,hSource,rGlyph->left,rGlyph->top,rGlyph->right-rGlyph->left,h,SRCCOPY);
                }
                if (drawMode==0 || drawMode==2)
                {
                    if (drawMode==0 )
                    {

                        int dy;
                        dy=(wr.top-rFill->top)%h;
                        if (dy>=0)
                        {
                            int ht;
                            y=wr.top;
                            ht=(y+h-dy<=wr.bottom)?(h-dy):(wr.bottom-wr.top);
                            BitBlt(hDest,wr.left,y,w,ht,mem2dc,0,dy,SRCCOPY);
                        }

                        y=wr.top+h-dy;
                        while (y<wr.bottom-h){
                            BitBlt(hDest,wr.left,y,w,h,mem2dc,0,0,SRCCOPY);
                            y+=h;
                        }
                        if (y<=wr.bottom)
                            BitBlt(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,SRCCOPY);                                            

                    }
                    else 
                    {  
                        y=wr.top;
                        while (y<wr.bottom-h)
                        {
                            //                             BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
                            SkinEngine_AlphaBlend(hDest,wr.left,y,w,h, mem2dc,0,0,w,h,bf);                    
                            y+=h;
                        }
                        if (y<=wr.bottom)
                        {
                            //                           BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
                            SkinEngine_AlphaBlend(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,w,wr.bottom-y,bf);                                            
                        }
                    }

                }
                else
                {
                    int dy;

                    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

                    dy=(wr.top-rFill->top)%h;

                    if (dy>=0)
                    {
                        int ht;
                        y=wr.top;
                        ht=(y+h-dy<=wr.bottom)?(h-dy):(wr.bottom-wr.top);
                        SkinEngine_AlphaBlend(hDest,wr.left,y,w,ht,mem2dc,0,dy,w,ht,bf);
                    }

                    y=wr.top+h-dy;
                    while (y<wr.bottom-h)
                    {
                        SkinEngine_AlphaBlend(hDest,wr.left,y,w,h,mem2dc,0,0,w,h,bf);
                        y+=h;
                    }
                    if (y<=wr.bottom)
                        SkinEngine_AlphaBlend(hDest,wr.left,y,w,wr.bottom-y, mem2dc,0,0,w,wr.bottom-y,bf);
                }
            }
        }
        SelectObject(mem2dc,oldbmp);
        DeleteObject(mem2bmp);
        mod_DeleteDC(mem2dc);
    }
    else if (mode==FM_TILE_HORZ && (rGlyph->right-rGlyph->left>0)&& (rGlyph->bottom-rGlyph->top>0)&&(rFill->bottom-rFill->top)>0 && (rFill->right-rFill->left)>0)
    {
        HDC mem2dc;
        RECT wr;
        HBITMAP mem2bmp,oldbmp;
        int w=rGlyph->right-rGlyph->left;
        int h=rFill->bottom-rFill->top;
        IntersectRect(&wr,rClip,rFill);
        if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
        h=wr.bottom-wr.top;
        mem2dc=CreateCompatibleDC(hDest);

        mem2bmp=SkinEngine_CreateDIB32(w,h);
        oldbmp=SelectObject(mem2dc,mem2bmp);

        if (!oldbmp)
            return 0;
        /// draw here
        {
            int  x=0, sy=0, maxy=0;
            {
                //SetStretchBltMode(mem2dc, HALFTONE);
                //StretchBlt(mem2dc,0,0,w,h,hSource,rGlyph->left+(wr.left-rFill->left),rGlyph->top,w,h,SRCCOPY);

                //                    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, 0 };
                SkinEngine_AlphaBlend(mem2dc,0,-(wr.top-rFill->top),w,rFill->bottom-rFill->top,hSource,rGlyph->left,rGlyph->top,w,rGlyph->bottom-rGlyph->top,bf);
                if (drawMode==0 || drawMode==2)
                {
                    if (drawMode==0)
                    {

                        int dx;
                        dx=(wr.left-rFill->left)%w;
                        if (dx>=0)
                        {   
                            int wt;
                            x=wr.left;
                            wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left);                                                   
                            BitBlt(hDest,x,wr.top,wt,h,mem2dc,dx,0,SRCCOPY);
                        }
                        x=wr.left+w-dx;                        
                        while (x<wr.right-w){
                            BitBlt(hDest,x,wr.top,w,h,mem2dc,0,0,SRCCOPY);
                            x+=w;
                        }
                        if (x<=wr.right);
                        BitBlt(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,SRCCOPY);  
                    }
                    else 
                    {
                        int dx;
                        dx=(wr.left-rFill->left)%w;
                        x=wr.left-dx;
                        while (x<wr.right-w){
                            SkinEngine_AlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
                            x+=w;
                        }
                        if (x<=wr.right)
                            SkinEngine_AlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);
                    }

                }
                else
                {
                    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                    int dx;
                    dx=(wr.left-rFill->left)%w;
                    if (dx>=0)
                    {
                        int wt;
                        x=wr.left;
                        wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left); 
                        SkinEngine_AlphaBlend(hDest,x,wr.top,wt,h,mem2dc,dx,0,wt,h,bf);
                    }
                    x=wr.left+w-dx;
                    while (x<wr.right-w){
                        SkinEngine_AlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
                        x+=w;
                    }
                    if (x<=wr.right)
                        SkinEngine_AlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);  

                }
            }
        }
        SelectObject(mem2dc,oldbmp);
        DeleteObject(mem2bmp);
        mod_DeleteDC(mem2dc);
    }
    else if (mode==FM_TILE_BOTH && (rGlyph->right-rGlyph->left>0) && (rGlyph->bottom-rGlyph->top>0))
    {
        HDC mem2dc;
        int w=rGlyph->right-rGlyph->left;
        int  x=0, sy=0, maxy=0;
        int h=rFill->bottom-rFill->top;
        HBITMAP mem2bmp,oldbmp;
        RECT wr;
        IntersectRect(&wr,rClip,rFill);
        if ((wr.bottom-wr.top)*(wr.right-wr.left)==0) return 0;
        mem2dc=CreateCompatibleDC(hDest);
        mem2bmp=SkinEngine_CreateDIB32(w,wr.bottom-wr.top);
        h=wr.bottom-wr.top;
        oldbmp=SelectObject(mem2dc,mem2bmp);
#ifdef _DEBUG
        if (!oldbmp) 
            (NULL,"Tile bitmap not selected","ERROR", MB_OK);
#endif
        /// draw here
        {

            //fill temp bitmap
            {
                int y;
                int dy;
                dy=(wr.top-rFill->top)%(rGlyph->bottom-rGlyph->top);
                y=-dy;
                while (y<wr.bottom-wr.top)
                {

                    SkinEngine_AlphaBlend(mem2dc,0,y,w,rGlyph->bottom-rGlyph->top, hSource,rGlyph->left,rGlyph->top,w,rGlyph->bottom-rGlyph->top,bf);                    
                    y+=rGlyph->bottom-rGlyph->top;
                }

                //--    
                //end temp bitmap
                if (drawMode==0 || drawMode==2)
                {
                    if (drawMode==0)
                    {

                        int dx;
                        dx=(wr.left-rFill->left)%w;
                        if (dx>=0)
                        {   
                            int wt;
                            x=wr.left;
                            wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left);                                                   
                            BitBlt(hDest,x,wr.top,wt,h,mem2dc,dx,0,SRCCOPY);
                        }
                        x=wr.left+w-dx;                        
                        while (x<wr.right-w){
                            BitBlt(hDest,x,wr.top,w,h,mem2dc,0,0,SRCCOPY);
                            x+=w;
                        }
                        if (x<=wr.right);
                        BitBlt(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,SRCCOPY);  
                    }
                    else 
                    {
                        int dx;
                        dx=(wr.left-rFill->left)%w;
                        x=wr.left-dx;
                        while (x<wr.right-w){
                            SkinEngine_AlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
                            x+=w;
                        }
                        if (x<=wr.right)
                            SkinEngine_AlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);
                    }

                }
                else
                {
                    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

                    int dx;
                    dx=(wr.left-rFill->left)%w;
                    if (dx>=0)
                    {
                        int wt;
                        x=wr.left;
                        wt=(x+w-dx<=wr.right)?(w-dx):(wr.right-wr.left); 
                        SkinEngine_AlphaBlend(hDest,x,wr.top,wt,h,mem2dc,dx,0,wt,h,bf);
                    }
                    x=wr.left+w-dx;
                    while (x<wr.right-w){
                        SkinEngine_AlphaBlend(hDest,x,wr.top,w,h,mem2dc,0,0,w,h,bf);
                        x+=w;
                    }
                    if (x<=wr.right)
                        SkinEngine_AlphaBlend(hDest,x,wr.top,wr.right-x,h, mem2dc,0,0,wr.right-x,h,bf);  

                }
            }

        }
        SelectObject(mem2dc,oldbmp);
        DeleteObject(mem2bmp);
        mod_DeleteDC(mem2dc);
    }
    return 1;

}

HBITMAP SkinEngine_CreateDIB32(int cx, int cy)
{
    return SkinEngine_CreateDIB32Point(cx,cy,NULL);
}

HBITMAP SkinEngine_CreateDIB32Point(int cx, int cy, void ** bits)
{
    BITMAPINFO RGB32BitsBITMAPINFO; 
    UINT * ptPixels;
    HBITMAP DirectBitmap;

    if ( cx < 0 || cy < 0 ) {
#ifdef _DEBUG
        DebugBreak();
#endif
        return NULL;
    }

    ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
    RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    RGB32BitsBITMAPINFO.bmiHeader.biWidth=cx;//bm.bmWidth;
    RGB32BitsBITMAPINFO.bmiHeader.biHeight=cy;//bm.bmHeight;
    RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
    RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;
    // pointer used for direct Bitmap pixels access


    DirectBitmap = CreateDIBSection(NULL, 
        (BITMAPINFO *)&RGB32BitsBITMAPINFO, 
        DIB_RGB_COLORS,
        (void **)&ptPixels, 
        NULL, 0);
    if ((DirectBitmap == NULL || ptPixels == NULL) && cx!= 0 && cy!=0) 
    {
#ifdef _DEBUG
        MessageBoxA(NULL,"Object not allocated. Check GDI object count","ERROR",MB_OK|MB_ICONERROR); 
        DebugBreak();
#endif
        ;
    }
    else	memset(ptPixels,0,cx*cy*4);
    if (bits!=NULL) *bits=ptPixels;  
    return DirectBitmap;
}

static int SkinEngine_DrawSkinObject(SKINDRAWREQUEST * preq, GLYPHOBJECT * pobj)
{
    HDC memdc=NULL, glyphdc=NULL;
    int k=0;
    //BITMAP bmp={0};
    HBITMAP membmp=0,oldbmp=0,oldglyph=0;
    BYTE Is32Bit=0;
    RECT PRect;
    POINT mode2offset={0};
    int depth=0; 
    int mode=0; //0-FastDraw, 1-DirectAlphaDraw, 2-BufferedAlphaDraw

    if (!(preq && pobj)) return -1;
    if ((!pobj->hGlyph || pobj->hGlyph==(HBITMAP)-1) && ((pobj->Style&7) ==ST_IMAGE ||(pobj->Style&7) ==ST_FRAGMENT|| (pobj->Style&7) ==ST_SOLARIZE)) return 0;
    // Determine painting mode   
    depth=GetDeviceCaps(preq->hDC,BITSPIXEL);
    depth=depth<16?16:depth;
    Is32Bit=pobj->bmBitsPixel==32;
    if ((!Is32Bit && pobj->dwAlpha==255)&& pobj->Style!=ST_BRUSH) mode=0;
    else if (pobj->dwAlpha==255 && pobj->Style!=ST_BRUSH) mode=1;
    else mode=2;
    // End painting mode

    //force mode

    if(preq->rcClipRect.bottom-preq->rcClipRect.top*preq->rcClipRect.right-preq->rcClipRect.left==0)
        preq->rcClipRect=preq->rcDestRect;
    IntersectRect(&PRect,&preq->rcDestRect,&preq->rcClipRect);
    if (IsRectEmpty(&PRect))
    {
        return 0;
    }
    if (mode==2)
    {   
        memdc=CreateCompatibleDC(preq->hDC);
        membmp=SkinEngine_CreateDIB32(PRect.right-PRect.left,PRect.bottom-PRect.top);
        oldbmp=SelectObject(memdc,membmp);
        if (oldbmp==NULL) 
        {
            if (mode==2)
            {
                SelectObject(memdc,oldbmp);
                mod_DeleteDC(memdc);
                DeleteObject(membmp);
            }
            return 0;
        }
    } 

    if (mode!=2) memdc=preq->hDC;
    {
        if (pobj->hGlyph && pobj->hGlyph!=(HBITMAP)-1)
        {
            glyphdc=CreateCompatibleDC(preq->hDC);
            if (!oldglyph) oldglyph=SelectObject(glyphdc,pobj->hGlyph);
            else 
                SelectObject(glyphdc,pobj->hGlyph);
        }
        // Drawing
        {    
            RECT rFill, rGlyph, rClip;
            if ((pobj->Style&7) ==ST_BRUSH)
            {
                HBRUSH br=CreateSolidBrush(pobj->dwColor);
                RECT fr;
                if (mode==2)
                {
                    SetRect(&fr,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top);
                    FillRect(memdc,&fr,br);
                    SkinEngine_SetRectOpaque(memdc,&fr);
                    // FillRectAlpha(memdc,&fr,pobj->dwColor|0xFF000000);
                }
                else
                {
                    fr=PRect;
                    // SetRect(&fr,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top);
                    FillRect(preq->hDC,&fr,br);
                }
                DeleteObject(br);
                k=-1;
            }
            else
            {
                if (mode==2)  
                {
                    mode2offset.x=PRect.left;
                    mode2offset.y=PRect.top;
                    OffsetRect(&PRect,-mode2offset.x,-mode2offset.y);
                }
                rClip=(preq->rcClipRect);

                {
                    int lft=0;
                    int top=0;
                    int rgh=pobj->bmWidth;
                    int btm=pobj->bmHeight;
                    if ((pobj->Style&7) ==ST_FRAGMENT)
                    {
                        lft=pobj->clipArea.x;
                        top=pobj->clipArea.y;
                        rgh=min(rgh,lft+pobj->szclipArea.cx);
                        btm=min(btm,top+pobj->szclipArea.cy);
                    }

                    // Draw center...
                    if (1)
                    {
                        rFill.top=preq->rcDestRect.top+pobj->dwTop;
                        rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
                        rFill.left=preq->rcDestRect.left+pobj->dwLeft;
                        rFill.right=preq->rcDestRect.right-pobj->dwRight;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

                        rGlyph.top=top+pobj->dwTop;
                        rGlyph.left=lft+pobj->dwLeft;
                        rGlyph.right=rgh-pobj->dwRight;
                        rGlyph.bottom=btm-pobj->dwBottom;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode,mode,depth);
                    }

                    // Draw top side...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.top;
                        rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
                        rFill.left=preq->rcDestRect.left+pobj->dwLeft;
                        rFill.right=preq->rcDestRect.right-pobj->dwRight;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

                        rGlyph.top=top+0;
                        rGlyph.left=lft+pobj->dwLeft;
                        rGlyph.right=rgh-pobj->dwRight;
                        rGlyph.bottom=top+pobj->dwTop;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_HORZ,mode,depth);
                    }
                    // Draw bottom side...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
                        rFill.bottom=preq->rcDestRect.bottom; 
                        rFill.left=preq->rcDestRect.left+pobj->dwLeft;
                        rFill.right=preq->rcDestRect.right-pobj->dwRight;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.top=btm-pobj->dwBottom;
                        rGlyph.left=lft+pobj->dwLeft;
                        rGlyph.right=rgh-pobj->dwRight;
                        rGlyph.bottom=btm;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_HORZ,mode,depth);
                    }
                    // Draw left side...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.top+pobj->dwTop;
                        rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
                        rFill.left=preq->rcDestRect.left;
                        rFill.right=preq->rcDestRect.left+pobj->dwLeft;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.top=top+pobj->dwTop;
                        rGlyph.left=lft;
                        rGlyph.right=lft+pobj->dwLeft;
                        rGlyph.bottom=btm-pobj->dwBottom;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_VERT,mode,depth);
                    }

                    // Draw right side...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.top+pobj->dwTop;
                        rFill.bottom=preq->rcDestRect.bottom-pobj->dwBottom; 
                        rFill.left=preq->rcDestRect.right-pobj->dwRight;
                        rFill.right=preq->rcDestRect.right;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.top=top+pobj->dwTop;
                        rGlyph.left=rgh-pobj->dwRight;
                        rGlyph.right=rgh;
                        rGlyph.bottom=btm-pobj->dwBottom;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,pobj->FitMode&FM_TILE_VERT,mode,depth);
                    }


                    // Draw Top-Left corner...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.top;
                        rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
                        rFill.left=preq->rcDestRect.left;
                        rFill.right=preq->rcDestRect.left+pobj->dwLeft;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.top=top;
                        rGlyph.left=lft;
                        rGlyph.right=lft+pobj->dwLeft;
                        rGlyph.bottom=top+pobj->dwTop;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
                    }
                    // Draw Top-Right corner...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.top;
                        rFill.bottom=preq->rcDestRect.top+pobj->dwTop; 
                        rFill.left=preq->rcDestRect.right-pobj->dwRight;
                        rFill.right=preq->rcDestRect.right;

                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.top=top;
                        rGlyph.left=rgh-pobj->dwRight;
                        rGlyph.right=rgh;
                        rGlyph.bottom=top+pobj->dwTop;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
                    }

                    // Draw Bottom-Left corner...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
                        rFill.bottom=preq->rcDestRect.bottom; 
                        rFill.left=preq->rcDestRect.left;
                        rFill.right=preq->rcDestRect.left+pobj->dwLeft;


                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);


                        rGlyph.left=lft;
                        rGlyph.right=lft+pobj->dwLeft; 
                        rGlyph.top=btm-pobj->dwBottom;
                        rGlyph.bottom=btm;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
                    }
                    // Draw Bottom-Right corner...
                    if(1)
                    {
                        rFill.top=preq->rcDestRect.bottom-pobj->dwBottom;
                        rFill.bottom=preq->rcDestRect.bottom;
                        rFill.left=preq->rcDestRect.right-pobj->dwRight;
                        rFill.right=preq->rcDestRect.right;


                        if (mode==2)
                            OffsetRect(&rFill,-mode2offset.x,-mode2offset.y);

                        rGlyph.left=rgh-pobj->dwRight;
                        rGlyph.right=rgh;
                        rGlyph.top=btm-pobj->dwBottom;
                        rGlyph.bottom=btm;

                        k+=SkinEngine_SkinFillRectByGlyph(memdc,glyphdc,&rFill,&rGlyph,&PRect,0,mode,depth);
                    }
                }

            }

            if ((k>0 || k==-1) && mode==2)
            {            
                {
                    BLENDFUNCTION bf={AC_SRC_OVER, 0, /*(bm.bmBitsPixel==32)?255:*/pobj->dwAlpha, (pobj->bmBitsPixel==32 && pobj->Style!=ST_BRUSH)?AC_SRC_ALPHA:0};
                    if (mode==2)
                        OffsetRect(&PRect,mode2offset.x,mode2offset.y);
                    SkinEngine_AlphaBlend( preq->hDC,PRect.left,PRect.top,PRect.right-PRect.left,PRect.bottom-PRect.top, 
                        memdc,0,0,PRect.right-PRect.left,PRect.bottom-PRect.top,bf);
                }                 
            }
        }
        //free GDI resources
        //--++--

        //free GDI resources
        {

            if (oldglyph) SelectObject(glyphdc,oldglyph);
            if (glyphdc) mod_DeleteDC(glyphdc);
        }    
        if (mode==2)
        {
            SelectObject(memdc,oldbmp);
            mod_DeleteDC(memdc);
            DeleteObject(membmp);
        }

    }  
    if (pobj->plTextList && pobj->plTextList->realCount>0)
    {
        int i;
        HFONT hOldFont;
        for (i=0; i<pobj->plTextList->realCount; i++)
        {
            GLYPHTEXT * gt=(GLYPHTEXT *)pobj->plTextList->items[i];
            if (!gt->hFont)
            {
                if (gl_plSkinFonts && gl_plSkinFonts->realCount>0)
                {
                    int j=0;
                    for (j=0; j<gl_plSkinFonts->realCount; j++)
                    {
                        SKINFONT * sf;
                        sf=(SKINFONT*)gl_plSkinFonts->items[j];
                        if (sf->szFontID && !strcmp(sf->szFontID,gt->szFontID))
                        {
                            gt->hFont=sf->hFont;
                            break;
                        }
                    }
                }
                if (!gt->hFont) gt->hFont=(HFONT)-1;
            }
            if (gt->hFont!=(HFONT)-1)
            {
                RECT rc={0};
                hOldFont=SelectObject(preq->hDC,gt->hFont);



                if (gt->RelativeFlags&2) rc.left=preq->rcDestRect.right+gt->iLeft;
                else if (gt->RelativeFlags&1) rc.left=((preq->rcDestRect.right-preq->rcDestRect.left)>>1)+gt->iLeft;
                else rc.left=preq->rcDestRect.left+gt->iLeft;

                if (gt->RelativeFlags&8) rc.top=preq->rcDestRect.bottom+gt->iTop;
                else if (gt->RelativeFlags&4) rc.top=((preq->rcDestRect.bottom-preq->rcDestRect.top)>>1)+gt->iTop;
                else rc.top=preq->rcDestRect.top+gt->iTop;

                if (gt->RelativeFlags&32) rc.right=preq->rcDestRect.right+gt->iRight;
                else if (gt->RelativeFlags&16) rc.right=((preq->rcDestRect.right-preq->rcDestRect.left)>>1)+gt->iRight;
                else rc.right=preq->rcDestRect.left+gt->iRight;

                if (gt->RelativeFlags&128) rc.bottom=preq->rcDestRect.bottom+gt->iBottom;
                else if (gt->RelativeFlags&64) rc.bottom=((preq->rcDestRect.bottom-preq->rcDestRect.top)>>1)+gt->iBottom;
                else rc.bottom=preq->rcDestRect.top+gt->iBottom;

                SkinEngine_AlphaTextOut(preq->hDC, gt->stText, -1, &rc,gt->dwFlags, gt->dwColor);
                SelectObject(preq->hDC,hOldFont);
            }
        }
    }

    return 0;
}



int SkinEngine_AddDescriptorToSkinObjectList (LPSKINOBJECTDESCRIPTOR lpDescr, SKINOBJECTSLIST* Skin)
{
    SKINOBJECTSLIST *sk;
    if (Skin) sk=Skin; else sk=&g_SkinObjectList;
    if (!sk) return 0;
    if (mir_bool_strcmpi(lpDescr->szObjectID,"_HEADER_")) return 0;
    {//check if new object allready presents.
        DWORD i=0;
        for (i=0; i<sk->dwObjLPAlocated;i++)
            if (!mir_strcmp(sk->pObjects[i].szObjectID,lpDescr->szObjectID)) return 0;
    }
    if (sk->dwObjLPAlocated+1>sk->dwObjLPReserved)
    { // Realocated list to add space for new object

        sk->pObjects=mir_realloc(sk->pObjects,sizeof(SKINOBJECTDESCRIPTOR)*(sk->dwObjLPReserved+1)/*alloc step*/);
        sk->dwObjLPReserved++; 
    }
    { //filling new objects field
        sk->pObjects[sk->dwObjLPAlocated].bType=lpDescr->bType;
        sk->pObjects[sk->dwObjLPAlocated].Data=NULL;
        sk->pObjects[sk->dwObjLPAlocated].szObjectID=mir_strdup(lpDescr->szObjectID);
        //  sk->Objects[sk->dwObjLPAlocated].szObjectName=mir_strdup(lpDescr->szObjectName);
        if (lpDescr->Data!=NULL)
        {   //Copy defaults values
            switch (lpDescr->bType) 
            {
            case OT_GLYPHOBJECT:
                {   
                    GLYPHOBJECT * obdat;
                    GLYPHOBJECT * gl=(GLYPHOBJECT*)lpDescr->Data;
                    sk->pObjects[sk->dwObjLPAlocated].Data=mir_alloc(sizeof(GLYPHOBJECT));
                    obdat=(GLYPHOBJECT*)sk->pObjects[sk->dwObjLPAlocated].Data;
                    memmove(obdat,gl,sizeof(GLYPHOBJECT));
                    if (gl->szFileName!=NULL)                    
                    {
                        obdat->szFileName=mir_strdup(gl->szFileName);
                        mir_free_and_nill(gl->szFileName);
                    }
                    else
                        obdat->szFileName=NULL;
                    obdat->hGlyph=NULL;
                    break;
                }
            }

        }
    }
    sk->dwObjLPAlocated++;
    return 1;
}

static LPSKINOBJECTDESCRIPTOR SkinEngine_FindObject(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin)
{
    // DWORD i;
    SKINOBJECTSLIST* sk;
    sk=(Skin==NULL)?(&g_SkinObjectList):Skin;
    return skin_FindObjectByRequest((char *)szName,sk->pMaskList);
}

static LPSKINOBJECTDESCRIPTOR SkinEngine_FindObjectByMask(MODERNMASK * pModernMask, BYTE objType, SKINOBJECTSLIST* Skin)
{
    // DWORD i;
    SKINOBJECTSLIST* sk;
    sk=(Skin==NULL)?(&g_SkinObjectList):Skin;
    return skin_FindObjectByMask(pModernMask,sk->pMaskList);
}

LPSKINOBJECTDESCRIPTOR SkinEngine_FindObjectByName(const char * szName, BYTE objType, SKINOBJECTSLIST* Skin)
{
    DWORD i;
    SKINOBJECTSLIST* sk;
    sk=(Skin==NULL)?(&g_SkinObjectList):Skin;
    for (i=0; i<sk->dwObjLPAlocated; i++)
    {
        if (sk->pObjects[i].bType==objType || objType==OT_ANY)
        {
            if (!mir_strcmp(sk->pObjects[i].szObjectID,szName))
                return &(sk->pObjects[i]);
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
// Paint glyph
// wParam - LPSKINDRAWREQUEST
// lParam - possible direct pointer to modern mask
//////////////////////////////////////////////////////////////////////////

int SkinEngine_Service_DrawGlyph(WPARAM wParam,LPARAM lParam)
{
    LPSKINDRAWREQUEST preq;
    LPSKINOBJECTDESCRIPTOR pgl;
    LPGLYPHOBJECT gl;
    if (!wParam) return -1;
    SkinEngine_LockSkin();
    __try
    {
        preq=(LPSKINDRAWREQUEST)wParam;   
        if (lParam)
            pgl=SkinEngine_FindObjectByMask((MODERNMASK*)lParam, OT_GLYPHOBJECT,NULL);
        else
            pgl=SkinEngine_FindObject(preq->szObjectID, OT_GLYPHOBJECT,NULL);
        if (pgl==NULL) return -1;
        if (pgl->Data==NULL) return -1;
        gl= (LPGLYPHOBJECT)pgl->Data;
        if ((gl->Style&7) ==ST_SKIP) return ST_SKIP;
        if (gl->hGlyph==NULL && gl->hGlyph!=(HBITMAP)-1 &&
            (  (gl->Style&7)==ST_IMAGE 
            ||(gl->Style&7)==ST_FRAGMENT 
            ||(gl->Style&7)==ST_SOLARIZE ) )
            if (gl->szFileName) 
            {
                gl->hGlyph=SkinEngine_LoadGlyphImage(gl->szFileName);
                if (gl->hGlyph)
                {
                    BITMAP bmp={0};
                    GetObject(gl->hGlyph,sizeof(BITMAP),&bmp);
                    gl->bmBitsPixel=(BYTE)bmp.bmBitsPixel;
                    gl->bmHeight=bmp.bmHeight;
                    gl->bmWidth=bmp.bmWidth;
                }
                else
                    gl->hGlyph=(HBITMAP)-1; //invalid
            }
            return SkinEngine_DrawSkinObject(preq,gl);
    }
    __finally
    {
        SkinEngine_UnlockSkin();
    }   
    return -1;
}


void SkinEngine_PreMultiplyChanells(HBITMAP hbmp,BYTE Mult)
{
    BITMAP bmp;     
    BOOL flag=FALSE;
    BYTE * pBitmapBits;
    DWORD Len;
    int bh,bw,y,x;

    GetObject(hbmp, sizeof(BITMAP), (LPSTR)&bmp);
    bh=bmp.bmHeight;
    bw=bmp.bmWidth;
    Len=bh*bw*4;
    flag=(bmp.bmBits==NULL);
    if (flag)
    {
        pBitmapBits=(LPBYTE)malloc(Len);
        GetBitmapBits(hbmp,Len,pBitmapBits);
    }
    else 
        pBitmapBits=bmp.bmBits;
    for (y=0; y<bh; ++y)
    {
        BYTE *pPixel= pBitmapBits + bw * 4 * y;

        for (x=0; x<bw ; ++x)
        {
            if (Mult)
            {
                pPixel[0]= pPixel[0]*pPixel[3]/255;
                pPixel[1]= pPixel[1]*pPixel[3]/255;
                pPixel[2]= pPixel[2]*pPixel[3]/255;
            }
            else
            {
                pPixel[3]=255;
            }
            pPixel+= 4;
        }
    }
    if (flag)
    {
        Len=SetBitmapBits(hbmp,Len,pBitmapBits);
        free (pBitmapBits);
    }
    return;
}

int SkinEngine_GetFullFilename(char * buf, char *file, char * skinfolder,BOOL madeAbsolute)
{
    char b2[MAX_PATH]={0};
    char *SkinPlace=DBGetStringA(NULL,SKIN,"SkinFolder");
    if (!SkinPlace) SkinPlace=mir_strdup("\\Skin\\default");
    if (file[0]!='\\' && file[1]!=':') 
        _snprintf(b2, MAX_PATH,"%s\\%s",((int)skinfolder==0)?SkinPlace:((int)skinfolder!=-1)?skinfolder:"",file);
    else
        _snprintf(b2, MAX_PATH,"%s",file);
    if (madeAbsolute) 
        if (b2[0]=='\\' && b2[1]!='\\')
            CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)(b2+1), (LPARAM)buf);
        else
            CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)(b2), (LPARAM)buf);
    else
        memcpy(buf,b2,MAX_PATH);

    if(SkinPlace) mir_free_and_nill(SkinPlace);
    return 0;
}


static HBITMAP SkinEngine_skinLoadGlyphImage(char * szFileName)
{
    if (!g_CluiData.fGDIPlusFail && !wildcmpi(szFileName,"*.tga"))
        return GDIPlus_LoadGlyphImage(szFileName);
    else 
        return SkinEngine_LoadGlyphImageByDecoders(szFileName);
}

/* 
This function is required to load TGA to dib buffer myself 
Major part of routines is from http://tfcduke.developpez.com/tutoriel/format/tga/fichiers/tga.c
*/

static BOOL SkinEngine_ReadTGAImageData(void * From, DWORD fromSize, BYTE * destBuf, DWORD bufSize, BOOL RLE)
{
    BYTE * pos=destBuf;
    BYTE * from=fromSize?(BYTE*)From:NULL;
    FILE * fp=!fromSize?(FILE*)From:NULL;
    DWORD destCount=0;
    DWORD fromCount=0;
    if (!RLE)
    {
        while (((from&&fromCount<fromSize) || (fp&& fromCount<bufSize))
            &&(destCount<bufSize))
        {
            BYTE r=from?from[fromCount++]:(BYTE)fgetc(fp);
            BYTE g=from?from[fromCount++]:(BYTE)fgetc(fp);
            BYTE b=from?from[fromCount++]:(BYTE)fgetc(fp);
            BYTE a=from?from[fromCount++]:(BYTE)fgetc(fp);
            pos[destCount++]=r;
            pos[destCount++]=g;
            pos[destCount++]=b;
            pos[destCount++]=a;

            if (destCount>bufSize) break;
            if (from) 	if (fromCount<fromSize) break;
        }
    }
    else
    {
        BYTE rgba[4];
        BYTE packet_header;
        BYTE *ptr=pos;
        BYTE size;
        int i;
        while (ptr < pos + bufSize)
        {
            /* read first byte */
            packet_header = from?from[fromCount]:(BYTE)fgetc(fp);
            if (from) from++;
            size = 1 + (packet_header & 0x7f);
            if (packet_header & 0x80)
            {
                /* run-length packet */
                if (from) 
                {
                    *((DWORD*)rgba)=*((DWORD*)(from+fromCount));
                    fromCount+=4;
                }
                else fread (rgba, sizeof (BYTE), 4, fp);
                for (i = 0; i < size; ++i, ptr += 4)
                {
                    ptr[2] = rgba[2];
                    ptr[1] = rgba[1];
                    ptr[0] = rgba[0];
                    ptr[3] = rgba[3];
                }
            }
            else
            {	/* not run-length packet */
                for (i = 0; i < size; ++i, ptr += 4)
                {
                    ptr[0] = from? from[fromCount++]:(BYTE)fgetc (fp);
                    ptr[1] = from? from[fromCount++]:(BYTE)fgetc (fp);
                    ptr[2] = from? from[fromCount++]:(BYTE)fgetc (fp);
                    ptr[3] = from? from[fromCount++]:(BYTE)fgetc (fp);
                }
            }
        }
    }
    return TRUE;
}

static HBITMAP SkinEngine_LoadGlyphImage_TGA(char * szFilename)
{
    BYTE *colormap = NULL;
    int cx=0,cy=0;
    BOOL err=FALSE;
    tga_header_t header;
    if (!szFilename) return NULL;
    if (!wildcmpi(szFilename,"*\\*%.tga")) 
    {
        //Loading TGA image from file
        FILE *fp;
        fp = fopen (szFilename, "rb");
        if (!fp)
        {
            TRACEVAR("error: couldn't open \"%s\"!\n", szFilename);
            return NULL;
        }
        /* read header */
        fread (&header, sizeof (tga_header_t), 1, fp);
        if (  (header.pixel_depth!=32)
            ||((header.image_type!=10)&&(header.image_type!=2))
            )
        {
            fclose(fp);
            return NULL;
        }

        /*memory allocation */
        colormap=(BYTE*)malloc(header.width*header.height*4);	
        cx=header.width;
        cy=header.height;
        fseek (fp, header.id_lenght, SEEK_CUR);
        fseek (fp, header.cm_length, SEEK_CUR);
        err=!SkinEngine_ReadTGAImageData((void*)fp, 0, colormap, header.width*header.height*4,header.image_type==10);
        fclose(fp);
    }


    else 
    {
        /* reading from resources IDR_TGA_DEFAULT_SKIN */
        DWORD size=0;
        BYTE * mem;
        HGLOBAL hRes;
        HRSRC hRSrc=FindResourceA(g_hInst,MAKEINTRESOURCEA(IDR_TGA_DEFAULT_SKIN),"TGA");
        if (!hRSrc) return NULL;
        hRes=LoadResource(g_hInst,hRSrc);
        if (!hRes) return NULL;
        size=SizeofResource(g_hInst,hRSrc);
        mem=(BYTE*) LockResource(hRes);
        if (size>sizeof(header))
        {
            tga_header_t * header=(tga_header_t *)mem;
            if (header->pixel_depth==32&& (header->image_type==2 ||header->image_type==10))
            {
                colormap=(BYTE*)malloc(header->width*header->height*4);	
                cx=header->width;
                cy=header->height;
                SkinEngine_ReadTGAImageData((void*)(mem+sizeof(tga_header_t)+header->id_lenght+header->cm_length), size-(sizeof(tga_header_t)+header->id_lenght+header->cm_length), colormap, cx*cy*4,header->image_type==10);
            }
        }
        FreeResource(hRes);
    }
    if (colormap)  //create dib section
    {
        BYTE * pt;
        HBITMAP hbmp=SkinEngine_CreateDIB32Point(cx,cy,&pt);
        if (hbmp) memcpy(pt,colormap,cx*cy*4);
        free(colormap);
        return hbmp;
    }
    return NULL;
}


//this function is required to load PNG to dib buffer myself
HBITMAP SkinEngine_LoadGlyphImage_Png2Dib(char * szFilename)
{

    {
        HANDLE hFile, hMap = NULL;
        BYTE* ppMap = NULL;
        long  cbFileSize = 0;
        BITMAPINFOHEADER* pDib;
        BYTE* pDibBits;

        if ( !ServiceExists( MS_PNG2DIB )) {
            MessageBox( NULL, TranslateT( "You need the png2dib plugin v. 0.1.3.x or later to process PNG images" ), TranslateT( "Error" ), MB_OK );
            return (HBITMAP)NULL;
        }

        if (( hFile = CreateFileA( szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL )) != INVALID_HANDLE_VALUE )
            if (( hMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL )) != NULL )
                if (( ppMap = ( BYTE* )MapViewOfFile( hMap, FILE_MAP_READ, 0, 0, 0 )) != NULL )
                    cbFileSize = GetFileSize( hFile, NULL );

        if ( cbFileSize != 0 ) {
            PNG2DIB param;
            param.pSource = ppMap;
            param.cbSourceSize = cbFileSize;
            param.pResult = &pDib;
            if ( CallService( MS_PNG2DIB, 0, ( LPARAM )&param ))
                pDibBits = ( BYTE* )( pDib+1 );
            else
                cbFileSize = 0;
        }

        if ( ppMap != NULL )	UnmapViewOfFile( ppMap );
        if ( hMap  != NULL )	CloseHandle( hMap );
        if ( hFile != NULL ) CloseHandle( hFile );

        if ( cbFileSize == 0 )
            return (HBITMAP)NULL;

        {
            BITMAPINFO* bi=( BITMAPINFO* )pDib;
            BYTE *pt=(BYTE*)bi;
            pt+=bi->bmiHeader.biSize;
            if (bi->bmiHeader.biBitCount!=32)
            {
                HDC sDC = GetDC( NULL );
                HBITMAP hBitmap = CreateDIBitmap( sDC, pDib, CBM_INIT, pDibBits, bi, DIB_PAL_COLORS );
                SelectObject( sDC, hBitmap );
                DeleteDC( sDC );
                GlobalFree( pDib );
                return hBitmap;
            }
            else
            {
                BYTE * ptPixels=pt;
                HBITMAP hBitmap=CreateDIBSection(NULL,bi, DIB_RGB_COLORS, (void **)&ptPixels,  NULL, 0);         
                memcpy(ptPixels,pt,bi->bmiHeader.biSizeImage);
                GlobalFree( pDib );
                return hBitmap;
            }	
        }
    }	
}

static HBITMAP SkinEngine_LoadGlyphImageByDecoders(char * szFileName)
{
    // Loading image from file by imgdecoder...    
    HBITMAP hBitmap=NULL;
    char ext[5];
    BYTE f=0;
    LPBYTE pBitmapBits;
    LPVOID pImg= NULL;
    LPVOID m_pImgDecoder;

    BITMAP bmpInfo;
    {
        int l;
        l=mir_strlen(szFileName);
        memmove(ext,szFileName +(l-4),5);   
    }
    if (!strchr(szFileName,'%') && !PathFileExistsA(szFileName)) return NULL;
    if (mir_bool_strcmpi(ext,".tga"))
    {
        hBitmap=SkinEngine_LoadGlyphImage_TGA(szFileName);
        f=1;
    }
    else if (ServiceExists("Image/Png2Dib") && mir_bool_strcmpi(ext,".png"))
    {
        hBitmap=SkinEngine_LoadGlyphImage_Png2Dib(szFileName);
        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        f=(bmpInfo.bmBits!=NULL);
        // hBitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szFileName);
        // f=1;

    }
    else if (hImageDecoderModule==NULL || !mir_bool_strcmpi(ext,".png"))
        hBitmap=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)szFileName);
    else
    {
        f=1;
        ImgNewDecoder(&m_pImgDecoder);
        if (!ImgNewDIBFromFile(m_pImgDecoder, szFileName, &pImg))	
        {
            ImgGetHandle(pImg, &hBitmap, (LPVOID *)&pBitmapBits);
            ImgDeleteDecoder(m_pImgDecoder);
        }
    }
    if (hBitmap)
    {

        GetObject(hBitmap, sizeof(BITMAP), &bmpInfo);
        if (bmpInfo.bmBitsPixel == 32)	
            SkinEngine_PreMultiplyChanells(hBitmap,f);
        else
        {
            HDC dc24,dc32;
            HBITMAP hBitmap32,obmp24,obmp32;
            dc32=CreateCompatibleDC(NULL);
            dc24=CreateCompatibleDC(NULL);
            hBitmap32=SkinEngine_CreateDIB32(bmpInfo.bmWidth,bmpInfo.bmHeight);
            obmp24=SelectObject(dc24,hBitmap);
            obmp32=SelectObject(dc32,hBitmap32);
            BitBlt(dc32,0,0,bmpInfo.bmWidth,bmpInfo.bmHeight,dc24,0,0,SRCCOPY);
            SelectObject(dc24,obmp24);
            SelectObject(dc32,obmp32);
            mod_DeleteDC(dc24);
            mod_DeleteDC(dc32);
            DeleteObject(hBitmap);
            hBitmap=hBitmap32;
            SkinEngine_PreMultiplyChanells(hBitmap,0);
        }

    }
    return hBitmap; 
}

HBITMAP SkinEngine_LoadGlyphImage(char * szFileName)
{
    // try to find image in loaded
    DWORD i;HBITMAP hbmp;
    char szFile [MAX_PATH];
    SkinEngine_GetFullFilename(szFile,szFileName,g_SkinObjectList.szSkinPlace,TRUE);
    /*{
    _snprintf(fn,sizeof(fn),"%s\\%s",g_SkinObjectList.SkinPlace,szfileName);
    CallService(MS_UTILS_PATHTOABSOLUTE, (WPARAM)fn, (LPARAM)&szFileName);
    }*/
    //CallService(MS_UTILS_PATHTOABSOLUTE,(WPARAM)szfileName,(LPARAM) &szFileName);
    for (i=0; i<dwLoadedImagesCount; i++)
    {
        if (mir_bool_strcmpi(pLoadedImages[i].szFileName,szFile))
        {
            pLoadedImages[i].dwLoadedTimes++;
            return pLoadedImages[i].hGlyph;
        }
    }
    {
        // load new image
        hbmp=SkinEngine_skinLoadGlyphImage(szFile);
        if (hbmp==NULL) return NULL;
        // add to loaded list
        if (dwLoadedImagesCount+1>dwLoadedImagesAlocated)
        {
            pLoadedImages=mir_realloc(pLoadedImages,sizeof(GLYPHIMAGE)*(dwLoadedImagesCount+1));
            if (pLoadedImages) dwLoadedImagesAlocated++;
            else return NULL;
        }
        pLoadedImages[dwLoadedImagesCount].dwLoadedTimes=1;
        pLoadedImages[dwLoadedImagesCount].hGlyph=hbmp;
        pLoadedImages[dwLoadedImagesCount].szFileName=mir_strdup(szFile);
        dwLoadedImagesCount++;
    }
    return hbmp;
}
int SkinEngine_UnloadGlyphImage(HBITMAP hbmp)
{
    DWORD i;
    for (i=0; i<dwLoadedImagesCount; i++)
    {
        if (hbmp==pLoadedImages[i].hGlyph)
        {
            pLoadedImages[i].dwLoadedTimes--;
            if (pLoadedImages[i].dwLoadedTimes==0)
            {
                LPGLYPHIMAGE gl=&(pLoadedImages[i]);
                if (gl->szFileName) mir_free_and_nill(gl->szFileName);
                memmove(&(pLoadedImages[i]),&(pLoadedImages[i+1]),sizeof(GLYPHIMAGE)*(dwLoadedImagesCount-i-1));
                dwLoadedImagesCount--;
                DeleteObject(hbmp);
                if (pLoadedImages && dwLoadedImagesCount==0) 
                {
                    dwLoadedImagesAlocated=0;
                    mir_free_and_nill(pLoadedImages);
                }
            }
            return 0;
        }

    }
    DeleteObject(hbmp);
    return 0;
}

int SkinEngine_UnloadSkin(SKINOBJECTSLIST * Skin)
{   

    DWORD i;
    SkinEngine_LockSkin();
    ClearMaskList(Skin->pMaskList);
    {//clear font list
        int i;
        if (gl_plSkinFonts && gl_plSkinFonts->realCount>0)
        {
            for (i=0; i<gl_plSkinFonts->realCount; i++)
            {
                SKINFONT * sf=gl_plSkinFonts->items[i];
                if (sf)
                {
                    if (sf->szFontID) mir_free_and_nill(sf->szFontID);
                    DeleteObject(sf->hFont);
					mir_free_and_nill(sf);
                }
            }
            li.List_Destroy(gl_plSkinFonts);
			mir_free_and_nill(gl_plSkinFonts);
        }
    }

    if (Skin->szSkinPlace) mir_free_and_nill(Skin->szSkinPlace);
    DeleteButtons();
    if (Skin->dwObjLPAlocated==0) { SkinEngine_UnlockSkin(); return 0;}
    for (i=0; i<Skin->dwObjLPAlocated; i++)
    {
        switch(Skin->pObjects[i].bType)
        {
        case OT_GLYPHOBJECT:
            {
                GLYPHOBJECT * dt;
                dt=(GLYPHOBJECT*)Skin->pObjects[i].Data;
                if (dt->hGlyph && dt->hGlyph!=(HBITMAP)-1) 
                    SkinEngine_UnloadGlyphImage(dt->hGlyph);
                dt->hGlyph=NULL;
                if (dt->szFileName) mir_free_and_nill(dt->szFileName);
                {// delete texts
                    int i;
                    if (dt->plTextList && dt->plTextList->realCount>0)
                    {
                        for (i=0; i<dt->plTextList->realCount; i++)
                        {
                            GLYPHTEXT * gt=dt->plTextList->items[i];
                            if (gt)
                            {
                                if (gt->stText)       mir_free_and_nill(gt->stText);
                                if (gt->stValueText)  mir_free_and_nill(gt->stValueText);
                                if (gt->szFontID)     mir_free_and_nill(gt->szFontID);
                                if (gt->szGlyphTextID)mir_free_and_nill(gt->szGlyphTextID);
								mir_free_and_nill(gt);
                            }
                        }
                        li.List_Destroy(dt->plTextList);
						mir_free_and_nill(dt->plTextList);
                    }
                }
                mir_free_and_nill(dt);
            }
            break;
        } 
        if (Skin->pObjects[i].szObjectID) mir_free_and_nill(Skin->pObjects[i].szObjectID); 

    }
    mir_free_and_nill(Skin->pObjects);
    Skin->dwObjLPAlocated=0;
    Skin->dwObjLPReserved=0;
    SkinEngine_UnlockSkin();
    return 0;
}

static int SkinEngine_enumdb_SkinObjectsProc (const char *szSetting,LPARAM lParam)
{   
    if (wildcmp((char *)szSetting,"$*",0))
    {
        char * value;
        value=DBGetStringA(NULL,SKIN,szSetting);
        RegisterObjectByParce((char *)szSetting,value);
        mir_free_and_nill(value);
    }
    else if (wildcmp((char *)szSetting,"#*",0))
    {
        char * value;
        value=DBGetStringA(NULL,SKIN,szSetting);
        RegisterButtonByParce((char *)szSetting,value);
        mir_free_and_nill(value);
    }
    return 0;
}
static int SkinEngine_enumdb_SkinMasksProc(const char *szSetting,LPARAM lParam)
{
    if (wildcmp((char *)szSetting,"@*",0) && pCurrentSkin)
    {
        DWORD ID=atoi(szSetting+1);
        int i=0;
        char * value;
        value=DBGetStringA(NULL,SKIN,szSetting);
        if (value)
        {
            for (i=0; i<mir_strlen(value); i++)  if (value[i]==':') break;
            if (i<mir_strlen(value))
            {
                char * Obj, *Mask;
                int res;
                Mask=value+i+1;
                Obj=mir_alloc(i+1);
                strncpy(Obj,value,i);
                Obj[i]='\0';
                res=AddStrModernMaskToList(ID,Mask,Obj,pCurrentSkin->pMaskList,pCurrentSkin);
                mir_free_and_nill(Obj);
            }
            mir_free_and_nill(value);
        }

    }
    else if (wildcmp((char *)szSetting,"t*",0) && pCurrentSkin)
    {
        char * value;
        value=DBGetStringA(NULL,SKIN,szSetting);
        SkinEngine_AddParseTextGlyphObject((char*)szSetting,value,pCurrentSkin);
        mir_free_and_nill(value);
    }
    else if (wildcmp((char *)szSetting,"f*",0) && pCurrentSkin)
    {
        char * value;
        value=DBGetStringA(NULL,SKIN,szSetting);
        SkinEngine_AddParseSkinFont((char*)szSetting,value,pCurrentSkin);
        mir_free_and_nill(value);
    }
    return 0;
}

// Getting skin objects and masks from DB
static int SkinEngine_GetSkinFromDB(char * szSection, SKINOBJECTSLIST * Skin)
{
    if (Skin==NULL) return 0;
    SkinEngine_UnloadSkin(Skin);
    Skin->pMaskList=mir_alloc(sizeof(LISTMODERNMASK));
    memset(Skin->pMaskList,0,sizeof(LISTMODERNMASK));
    Skin->szSkinPlace=DBGetStringA(NULL,SKIN,"SkinFolder");
    if (!Skin->szSkinPlace ) 
    {
        Skin->szSkinPlace=mir_strdup("%Default%");
        SkinEngine_LoadSkinFromResource();
    }
    //Load objects
    {
        DBCONTACTENUMSETTINGS dbces;
        pCurrentSkin=Skin;
        dbces.pfnEnumProc=SkinEngine_enumdb_SkinObjectsProc;
        dbces.szModule=SKIN;
        dbces.ofsSettings=0;
        CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

        dbces.pfnEnumProc=SkinEngine_enumdb_SkinMasksProc;
        dbces.ofsSettings=0;
        CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);
        SortMaskList(Skin->pMaskList);
        pCurrentSkin=NULL;
    }
    //Load Masks
    return 0;
}

//surrogate to be called from outside
void SkinEngine_LoadSkinFromDB(void) 
{ 
    SkinEngine_GetSkinFromDB(SKIN,&g_SkinObjectList); 
    g_CluiData.fUseKeyColor=(BOOL)DBGetContactSettingByte(NULL,"ModernSettings","UseKeyColor",1);
    g_CluiData.dwKeyColor=DBGetContactSettingDword(NULL,"ModernSettings","KeyColor",(DWORD)RGB(255,0,255));
}


int SkinEngine_GetSkinFolder(char * szFileName, char * t2)
{
    char *buf;   
    char *b2;

    b2=mir_strdup(szFileName);
    buf=b2+mir_strlen(b2);
    while (buf>b2 && *buf!='.') {buf--;}
    *buf='\0';
    strcpy(t2,b2);

    {
        char custom_folder[MAX_PATH];
        char cus[MAX_PATH];
        char *b3;
        strcpy(custom_folder,t2);
        b3=custom_folder+mir_strlen(custom_folder);
        while (b3>custom_folder && *b3!='\\') {b3--;}
        *b3='\0';

        GetPrivateProfileStringA("Skin_Description_Section","SkinFolder","",cus,sizeof(custom_folder),szFileName);
        if (mir_strlen(cus)>0)
            _snprintf(t2,MAX_PATH,"%s\\%s",custom_folder,cus);
    }   	
    mir_free_and_nill(b2);
    CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)t2, (LPARAM)t2);
    return 0;
}
//

static void SkinEngine_WriteParamToDatabase(char *cKey, char* cName, char* cVal, BOOL SecCheck)
{
    if (SecCheck)
    {
        //TODO check security here
        if (wildcmp(cKey,"Skin_Description_Section",1)) return;
    }
    if (strlen(cVal)>0 && cVal[strlen(cVal)-1]==10) cVal[strlen(cVal)-1]='\0';  //kill linefeed at the end  
    switch(cVal[0]) 
    {
    case 'b':
        {
            BYTE P;
            P=(BYTE)atoi(cVal+1);
            DBWriteContactSettingByte(NULL,cKey,cName,P);
        }
        break;
    case 'w':
        {
            WORD P;
            P=(WORD)atoi(cVal+1);
            DBWriteContactSettingWord(NULL,cKey,cName,P);
        }
        break;
    case 'd':
        {
            DWORD P;
            P=(DWORD)atoi(cVal+1);
            DBWriteContactSettingDword(NULL,cKey,cName,P);
        }
        break;
    case 's':
        DBWriteContactSettingString(NULL,cKey,cName,cVal+1);
        break;
    case 'f':
        if (szFileName)
        {
            char fn[MAX_PATH]={0};
            char bb[MAX_PATH*2]={0};
            int pp, i;
            pp=-1;
            CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)fn);
            {
                for (i=strlen(fn); i>=0; i--)  if (fn[i]=='.') break;
                if (i>0) fn[i]='\0';
            }                      
            _snprintf(bb,SIZEOF(bb),"%s\\%s",fn,cVal+1);
            DBWriteContactSettingString(NULL,cKey,cName,bb);
        }
        break;
    }
}

static BOOL SkinEngine_ParseLineOfIniFile(char * Line)
{
    DWORD i=0;
    DWORD len=strlen(Line);
    while (i<len && (Line[i]==' ' || Line[i]=='\t')) i++; //skip spaces&tabs
    if (i>=len) return FALSE; //only spaces (or tabs)
    if (len>0 && Line[len-1]==10) Line[len-1]='\0';
    switch(Line[i])
    {
    case ';':
        return FALSE; // start of comment is found
    case '[':
        //New section start here
        if (iniCurrentSection) mir_free_and_nill(iniCurrentSection);
        {
            char *tbuf=Line+i+1;		
            DWORD len2=strlen(tbuf);
            DWORD k=len2;
            while (k>0 && tbuf[k]!=']') k--; //searching close bracket
            tbuf[k]='\0';   //closing string
            if (k==0) return FALSE;
            iniCurrentSection=mir_strdup(tbuf);
        }
        return TRUE;
    default:
        if (!iniCurrentSection) return FALSE;  //param found out of section
        {
            char *keyName=Line+i;
            char *keyValue=Line+i;

            DWORD eqPlace=0;
            DWORD len2=strlen(keyName);
            while (eqPlace<len2 && keyName[eqPlace]!='=') eqPlace++; //find '='
            if (eqPlace==0 || eqPlace==len2) return FALSE; //= not found or no key name
            keyName[eqPlace]='\0';
            keyValue=keyName+eqPlace+1;
            //remove tail spaces in Name
            {
                DWORD len3=strlen(keyName);
                int j=len3-1;
                while (j>0 && (keyName[j]==' ' || keyName[j]=='\t')) j--;
                if (j>=0) keyName[j+1]='\0';
            }		
            //remove start spaces in Value
            {
                DWORD len3=strlen(keyValue);
                DWORD j=0;
                while (j<len3 && (keyValue[j]==' ' || keyValue[j]=='\t')) j++;
                if (j<len3) keyValue+=j;
            }
            //remove tail spaces in Value
            {
                DWORD len3=strlen(keyValue);
                int j=len3-1;
                while (j>0 && (keyValue[j]==' ' || keyValue[j]=='\t')) j--;
                if (j>=0) keyValue[j+1]='\0';
            }
            SkinEngine_WriteParamToDatabase(iniCurrentSection,keyName,keyValue,TRUE);
        }
    }
    return FALSE;
}

static int SkinEngine_LoadSkinFromResource()
{
    DWORD size=0;
    char * mem;
    char * pos;
    HGLOBAL hRes;
    HRSRC hRSrc=FindResourceA(g_hInst,MAKEINTRESOURCEA(IDR_MSF_DEFAULT_SKIN),"MSF");
    if (!hRSrc) return 0;
    hRes=LoadResource(g_hInst,hRSrc);
    if (!hRes) return 0;
    size=SizeofResource(g_hInst,hRSrc);
    mem=(char*) LockResource(hRes);
    SkinEngine_DeleteAllSettingInSection("ModernSkin");
    DBWriteContactSettingString(NULL,SKIN,"SkinFolder","%Default%");
    DBWriteContactSettingString(NULL,SKIN,"SkinFile","%Default%");
    {
        char line[513]={0};
        pos=(char*) mem;
        while (pos<mem+size)
        {
            int i=0;
            while (pos<mem+size && *pos!='\n' && *pos!='\0' && i<512)
            {
                if ((*pos)!='\r') line[i++]=*pos;
                pos++;
                line[i]='\0';
            }
            TRACE(line); TRACE("\n");
            SkinEngine_ParseLineOfIniFile(line);
            pos++;
        }
    }
    FreeResource(hRes);	
    return 0;
}

//Load data from ini file
int SkinEngine_LoadSkinFromIniFile(char * szFileName)
{
    FILE *stream=NULL;
    char line[512]={0};
    char skinFolder[MAX_PATH]={0};
    char skinFile[MAX_PATH]={0};
    if (strchr(szFileName,'%')) 
        return SkinEngine_LoadSkinFromResource();

    SkinEngine_DeleteAllSettingInSection("ModernSkin");
    SkinEngine_GetSkinFolder(szFileName,skinFolder);
    DBWriteContactSettingString(NULL,SKIN,"SkinFolder",skinFolder);
    CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)skinFile);
    DBWriteContactSettingString(NULL,SKIN,"SkinFile",skinFile);

    if( (stream = fopen( szFileName, "r" )) != NULL )
    {
        szFileName=szFileName;
        while (fgets( line, SIZEOF(line),stream ) != NULL)
        {
            SkinEngine_ParseLineOfIniFile(line);
        }
        fclose( stream );
        szFileName=NULL;
    }
    return 0;
}

//Load data from ini file

int SkinEngine_OldLoadSkinFromIniFile(char * szFileName)
{
    char bsn[MAXSN_BUFF_SIZE];
    char * Buff;

    int i=0;
    int f=0;
    int ReadingSection=0;
    char AllowedSection[260];
    int AllowedAll=0;
    char t2[MAX_PATH];
    char t3[MAX_PATH];

    DWORD retu=GetPrivateProfileSectionNamesA(bsn,MAXSN_BUFF_SIZE,szFileName);
    SkinEngine_DeleteAllSettingInSection("ModernSkin");
    SkinEngine_GetSkinFolder(szFileName,t2);
    DBWriteContactSettingString(NULL,SKIN,"SkinFolder",t2);
    CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)t3);
    DBWriteContactSettingString(NULL,SKIN,"SkinFile",t3);
    Buff=bsn;
    AllowedSection[0]=0;
    do         
    {
        f=mir_strlen(Buff);
        if (f>0 && !mir_bool_strcmpi(Buff,"Skin_Description_Section"))
        {
            char b3[MAX_BUFF_SIZE];
            DWORD ret=0;
            ret=GetPrivateProfileSectionA(Buff,b3,MAX_BUFF_SIZE,szFileName);
            if (ret>MAX_BUFF_SIZE-3) continue;
            if (ret==0) continue;
            {
                DWORD p=0;
                char *s1;
                char *s2;
                char *s3;
                {
                    DWORD t;
                    BOOL LOCK=FALSE;
                    for (t=0; t<ret-1;t++)
                    {
                        if (b3[t]=='\0') LOCK=FALSE;
                        if (b3[t]=='=' && !LOCK) 
                        {
                            b3[t]='\0';
                            LOCK=TRUE;
                        }
                    }
                }
                do
                {
                    s1=b3+p;

                    s2=s1+mir_strlen(s1)+1;
                    switch (s2[0])
                    {
                    case 'b':
                        {
                            BYTE P;
                            //                            char ba[255];
                            s3=s2+1;
                            P=(BYTE)atoi(s3);
                            DBWriteContactSettingByte(NULL,Buff,s1,P);
                        }
                        break;
                    case 'w':
                        {
                            WORD P;
                            //                           char ba[255];
                            s3=s2+1;
                            P=(WORD)atoi(s3);
                            DBWriteContactSettingWord(NULL,Buff,s1,P);
                        }break;
                    case 'd':
                        {
                            DWORD P;

                            s3=s2+1;
                            P=(DWORD)atoi(s3);
                            DBWriteContactSettingDword(NULL,Buff,s1,P);
                        }break;
                    case 's':
                        {
                            //                          char ba[255];
                            char bb[255];
                            s3=s2+1;
                            strncpy(bb,s3,sizeof(bb));
                            DBWriteContactSettingString(NULL,Buff,s1,s3);
                        }break;
                    case 'f': //file
                        {
                            //                         char ba[255];
                            char bb[255];

                            s3=s2+1;
                            {
                                char fn[MAX_PATH];
                                int pp, i;
                                pp=-1;
                                CallService(MS_UTILS_PATHTORELATIVE, (WPARAM)szFileName, (LPARAM)fn);
                                {
                                    for (i=0; i<mir_strlen(fn); i++)  if (fn[i]=='.') pp=i;
                                    if (pp!=-1)
                                    {
                                        fn[pp]='\0';
                                    }
                                }                      
                                sprintf(bb,"%s\\%s",fn,s3);
                                DBWriteContactSettingString(NULL,Buff,s1,bb);
                            }
                        }break;
                    }
                    p=p+mir_strlen(s1)+mir_strlen(s2)+2;
                } while (p<ret);

            }
        }
        Buff+=mir_strlen(Buff)+1;
    }while (((DWORD)Buff-(DWORD)bsn)<retu);
    return 0;
}


static int SkinEngine_enumdb_SkinSectionDeletionProc (const char *szSetting,LPARAM lParam)
{

    if (szSetting==NULL){return(0);};
    nArrayLen++;
    pszSettingName=(char **)realloc(pszSettingName,nArrayLen*sizeof(char *));
    pszSettingName[nArrayLen-1]=_strdup(szSetting);
    return(0);
};
static int SkinEngine_DeleteAllSettingInSection(char * SectionName)
{
    DBCONTACTENUMSETTINGS dbces;
    nArrayLen=0;
    pszSettingName=NULL;
    dbces.pfnEnumProc=SkinEngine_enumdb_SkinSectionDeletionProc;
    dbces.szModule=SectionName;
    dbces.ofsSettings=0;

    CallService(MS_DB_CONTACT_ENUMSETTINGS,0,(LPARAM)&dbces);

    //delete all settings
    if (nArrayLen==0){return(0);};
    {
        int i;
        for (i=0;i<nArrayLen;i++)
        {
            DBDeleteContactSetting(0,SectionName,pszSettingName[i]);
            free(pszSettingName[i]);
        };
        free(pszSettingName);
        pszSettingName=NULL;
        nArrayLen=0;    
    };
    return(0);
};


BOOL SkinEngine_TextOutA(HDC hdc, int x, int y, char * lpString, int nCount)
{
#ifdef UNICODE
    TCHAR *buf=mir_alloc((2+nCount)*sizeof(TCHAR));
    BOOL res;
    MultiByteToWideChar(CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 ), 0, lpString, -1, buf, (2+nCount)*sizeof(TCHAR)); 
    res=SkinEngine_TextOut(hdc,x,y,buf,nCount);
    mir_free_and_nill(buf);
    return res;
#else
    return SkinEngine_TextOut(hdc,x,y,lpString,nCount);
#endif
}

BOOL SkinEngine_TextOut(HDC hdc, int x, int y, LPCTSTR lpString, int nCount)
{
    int ta;
    SIZE sz;
    RECT rc={0};
    if (!g_CluiData.fGDIPlusFail &&0) ///text via gdi+
    {
        TextOutWithGDIp(hdc,x,y,lpString,nCount);
        return 0;
    }
    else

    {
        // return TextOut(hdc, x,y,lpString,nCount);
        GetTextExtentPoint32(hdc,lpString,nCount,&sz);
        ta=GetTextAlign(hdc);
        SetRect(&rc,x,y,x+sz.cx,y+sz.cy);
        SkinEngine_DrawText(hdc,lpString,nCount,&rc,DT_NOCLIP|DT_SINGLELINE|DT_LEFT);
    }
    return 1;
}

static int SkinEngine_Service_AlphaTextOut(WPARAM wParam,LPARAM lParam)
{
    if (!wParam) return 0;
    {
        AlphaTextOutParams ap=*(AlphaTextOutParams*)wParam;
        return SkinEngine_AlphaTextOut(ap.hDC,ap.lpString,ap.nCount,ap.lpRect,ap.format,ap.ARGBcolor);
    }
}

static __inline void SkinEngine_SetMatrix( sbyte * matrix,
                                         sbyte a, sbyte b, sbyte c, 
                                         sbyte d, sbyte e, sbyte f, 
                                         sbyte g, sbyte h, sbyte i)
{
    matrix[0]=a;	matrix[1]=b;	matrix[2]=c;
    matrix[3]=d;	matrix[4]=e;	matrix[5]=f;
    matrix[6]=g;	matrix[7]=h;	matrix[8]=i;
}

static void SkinEngine_SetTextEffect(BYTE EffectID, DWORD FirstColor, DWORD SecondColor)
{
    if (EffectID>MAXPREDEFINEDEFFECTS) return; 
    if (EffectID==-1) meCurrentEffect.EffectID=-1;
    else
    {
        meCurrentEffect.EffectID=EffectID;
        meCurrentEffect.EffectMatrix=ModernEffectsEnum[EffectID];
        meCurrentEffect.EffectColor1=FirstColor;
        meCurrentEffect.EffectColor2=SecondColor;
    }
}

BOOL SkinEngine_ResetTextEffect(HDC hdc)
{
    int i;
    if (!pEffectStack || !pEffectStack->realCount) return TRUE;
    for (i=0; i<pEffectStack->realCount; i++)
        if (pEffectStack->items[i] && ((EFFECTSSTACKITEM*)(pEffectStack->items[i]))->hdc==hdc)
        {
            EFFECTSSTACKITEM * effect=(EFFECTSSTACKITEM*)(pEffectStack->items[i]);
            mir_free_and_nill(effect);
            li.List_Remove(pEffectStack,i);
            return TRUE;
        }
        return FALSE;
};

BOOL SkinEngine_SelectTextEffect(HDC hdc, BYTE EffectID, DWORD FirstColor, DWORD SecondColor)
{
    if (EffectID>MAXPREDEFINEDEFFECTS) return 0; 
    if (EffectID==-1) return SkinEngine_ResetTextEffect(hdc);
    if (!pEffectStack)
    {
        pEffectStack=li.List_Create(0,1);
    }
    {
        int i;
        for (i=0; i<pEffectStack->realCount; i++)
            if (pEffectStack->items[i] && ((EFFECTSSTACKITEM*)(pEffectStack->items[i]))->hdc==hdc)
            {
                EFFECTSSTACKITEM * effect=(EFFECTSSTACKITEM*)(pEffectStack->items[i]);
                effect->EffectID=EffectID;		    
                effect->FirstColor=FirstColor;
                effect->SecondColor=SecondColor;
                return TRUE;
            }
    }
    {
        EFFECTSSTACKITEM * effect=(EFFECTSSTACKITEM *) mir_alloc(sizeof(EFFECTSSTACKITEM));
        effect->hdc=hdc;
        effect->EffectID=EffectID;
        effect->FirstColor=FirstColor;
        effect->SecondColor=SecondColor;
        li.List_Insert(pEffectStack, effect, 0);
        return TRUE;
    }	  
    return FALSE;
}

static BOOL SkinEngine_GetTextEffect(HDC hdc, MODERNEFFECT * modernEffect)
{
    int i=0;
    if (!pEffectStack || !pEffectStack->realCount) return FALSE;
    if (!modernEffect) return FALSE;
    for (i=0; i<pEffectStack->realCount; i++)
        if (pEffectStack->items[i] && ((EFFECTSSTACKITEM*)(pEffectStack->items[i]))->hdc==hdc)
        {
            EFFECTSSTACKITEM * effect=(EFFECTSSTACKITEM*)(pEffectStack->items[i]);
            modernEffect->EffectID=effect->EffectID;		    
            modernEffect->EffectColor1=effect->FirstColor;
            modernEffect->EffectColor2=effect->SecondColor;
            modernEffect->EffectMatrix=ModernEffectsEnum[effect->EffectID];
            return TRUE;
        }
        return FALSE;
}

static BOOL SkinEngine_DrawTextEffect(BYTE* destPt,BYTE* maskPt, DWORD width, DWORD height, MODERNEFFECT *effect)
{
    sbyte *buf;
    sbyte *outbuf;
    sbyte *bufline, *buflineTop, *buflineMid;
    int sign=0;
    BYTE *maskline,*destline;
    BYTE al,rl,gl,bl,ad,rd,gd,bd;
    int k=0;
    DWORD x,y;
    sbyte *matrix;
    BYTE mcTopStart;
    BYTE mcBottomEnd;
    BYTE mcLeftStart;
    BYTE mcRightEnd;
    BYTE effectCount;
    int minX=width;
    int maxX=0;
    int minY=height;
    int maxY=0;
    if (effect->EffectID==0xFF) return FALSE;
    if (!width || ! height) return FALSE;
    if (!destPt) return FALSE;
    buf=(BYTE*)malloc(width*height*sizeof(BYTE));
    {
        matrix=effect->EffectMatrix.matrix;
        mcTopStart=2-effect->EffectMatrix.topEffect;
        mcBottomEnd=3+effect->EffectMatrix.bottomEffect;
        mcLeftStart=2-effect->EffectMatrix.leftEffect;
        mcRightEnd=3+effect->EffectMatrix.rightEffect;
        effectCount=effect->EffectMatrix.cycleCount;
    }
    al=255-((BYTE)(effect->EffectColor1>>24));
    rl=GetRValue(effect->EffectColor1);
    gl=GetGValue(effect->EffectColor1);
    bl=GetBValue(effect->EffectColor1);
    rd=GetRValue(effect->EffectColor2);
    gd=GetGValue(effect->EffectColor2);
    bd=GetBValue(effect->EffectColor2);
    ad=255-((BYTE)(effect->EffectColor2>>24));
    rd=GetRValue(effect->EffectColor2);
    gd=GetGValue(effect->EffectColor2);
    bd=GetBValue(effect->EffectColor2);

    //Fill buffer by mid values of image
    for (y=0; y<height; y++)
    {
        bufline=buf+y*width;
        maskline=maskPt+((y*width)<<2);
        for (x=0; x<width; x++)
        {
            BYTE a=(sbyte)(DWORD)((maskline[0]+maskline[2]+maskline[1]+maskline[1])>>4);
            *bufline=a;
            if (a!=0)
            {
                minX=min((int)x,minX);
                minY=min((int)y,minY);
                maxX=max((int)x,maxX);
                maxY=max((int)y,maxY);
            }
            bufline++;
            maskline+=4;
        }
    }
    //Here perform effect on buffer and place results to outbuf
    for (k=0; k<(effectCount&0x7F); k++)
    {
        minX=max(0,minX+mcLeftStart-2);
        minY=max(0,minY+mcTopStart-2);
        maxX=min((int)width,maxX+mcRightEnd-1);
        maxY=min((int)height,maxX+mcBottomEnd-1);

        outbuf=(sbyte*)malloc(width*height*sizeof(sbyte));
        memset(outbuf,0,width*height*sizeof(sbyte));
        for (y=(DWORD)minY; y<(DWORD)maxY; y++)
        {
            int val;
            bufline=outbuf+y*width+minX;
            buflineMid=buf+y*width+minX; 
            for (x=(DWORD)minX; x<(DWORD)maxX; x++)
            {			
                int matrixHor,matrixVer;
                val=0;			
                for (matrixVer=mcTopStart; matrixVer<mcBottomEnd; matrixVer++)
                    for (matrixHor=mcLeftStart; matrixHor<mcRightEnd;matrixHor++)
                    {						
                        int a=y+matrixVer-2;
                        buflineTop=NULL;
                        if (a>=0 && (DWORD)a<height) buflineTop=buflineMid+width*(matrixVer-2);
                        a=x+matrixHor-2;
                        if (buflineTop && a>=0 && (DWORD)a<width) buflineTop+=matrixHor-2;
                        else buflineTop=NULL;
                        if (buflineTop) 
                            val+=((*buflineTop)*matrix[matrixVer*5+matrixHor]); 					
                    }
                    val=(val+1)>>5;
                    *bufline=(sbyte)((val>127)?127:(val<-125)?-125:val);
                    bufline++;
                    buflineMid++;
            }
        }
        free(buf);
        buf=outbuf;
    }
    {
        BYTE r1,b1,g1,a1;
        b1=bl; r1=rl; g1=gl; a1=al; sign=1;
        //perform out to dest
        for (y=0; y<height; y++)
        {		
            bufline=buf+y*width;
            destline=destPt+((y*width)<<2);
            for (x=0; x<width; x++)
            {
                sbyte val=*bufline;
                BYTE absVal=((val<0)?-val:val);

                if (val!=0)
                {	
                    if (val>0 && sign<0)
                    { b1=bl; r1=rl; g1=gl; a1=al; sign=1;}
                    else if (val<0 && sign>0)
                    { b1=bd; r1=rd; g1=gd; a1=ad; sign=-1;}

                    absVal=absVal*a1/255;				

                    destline[0]=((destline[0]*(128-absVal))+absVal*b1)>>7;
                    destline[1]=((destline[1]*(128-absVal))+absVal*g1)>>7;
                    destline[2]=((destline[2]*(128-absVal))+absVal*r1)>>7;
                    destline[3]+=((255-destline[3])*(a1*absVal))/32640;				
                }
                bufline++;
                destline+=4;
            }
        }
        free(buf);
    }
    return FALSE;
}

static int SkinEngine_AlphaTextOut (HDC hDC, LPCTSTR lpstring, int nCount, RECT * lpRect, UINT format, DWORD ARGBcolor)
{
    HBITMAP destBitmap;
    SIZE sz, fsize;
    SIZE wsize={0};
    HDC memdc;
    HBITMAP hbmp,holdbmp;
    HDC bufDC;
    HBITMAP bufbmp,bufoldbmp;
    BITMAP bmpdata;
    BYTE * destBits;
    BOOL noDIB=0;
    BOOL is16bit=0;
    BYTE * bits;
    BYTE * bufbits;
    HFONT hfnt, holdfnt;
    LPTSTR lpString=NULL;

    int drx=0;
    int dry=0;
    int dtx=0;
    int dty=0;
    int dtsy=0;
    int dtey=0;
    int dtsx=0;
    int dtex=0;
    RECT workRect;
    workRect=*lpRect;
    if (!bGammaWeightFilled)
    {
        int i;
        for(i=0;i<256;i++)
        {
            double f;
            double gamma=(double)DBGetContactSettingDword(NULL,"ModernData","AlphaTextOutGamma1",700)/1000;
            double f2;
            double gamma2=(double)DBGetContactSettingDword(NULL,"ModernData","AlphaTextOutGamma2",700)/1000;

            f=(double)i/255;
            f=pow(f,(1/gamma));
            f2=(double)i/255;
            f2=pow(f2,(1/gamma2));
            pbGammaWeight[i]=(BYTE)(255*f);
            pbGammaWeightAdv[i]=(BYTE)(255*f);
        }
        bGammaWeightFilled=1;
    }
    if (!lpstring) return 0;
    lpString=mir_tstrdup(lpstring);
    if (nCount==-1) nCount=lstrlen(lpString);
    // retrieve destination bitmap bits
    {
        destBitmap=(HBITMAP)GetCurrentObject(hDC,OBJ_BITMAP);
        GetObject(destBitmap, sizeof(BITMAP),&bmpdata);
        if (bmpdata.bmBits==NULL)
        {
            noDIB=1;
            destBits=(BYTE*)malloc(bmpdata.bmHeight*bmpdata.bmWidthBytes);
            GetBitmapBits(destBitmap,bmpdata.bmHeight*bmpdata.bmWidthBytes,destBits);
        }
        else 
            destBits=bmpdata.bmBits;
        is16bit=(bmpdata.bmBitsPixel)!=32;
    }
    // Create DC
    {
        memdc=CreateCompatibleDC(hDC);
        hfnt=(HFONT)GetCurrentObject(hDC,OBJ_FONT);
        SetBkColor(memdc,0);
        SetTextColor(memdc,RGB(255,255,255));
        holdfnt=(HFONT)SelectObject(memdc,hfnt);
    }
    {
        GetTextExtentPoint32(memdc,lpString,nCount,&sz);
        if ((format&DT_END_ELLIPSIS) && sz.cx>workRect.right-workRect.left)
        {
            SIZE szElipses={0};
            TCHAR *tem=NULL;
            int number=0;
            GetTextExtentPoint32A(memdc,"...",3,&szElipses);
            szElipses.cx+=1;
            if (workRect.right-workRect.left-szElipses.cx>0)
                GetTextExtentExPoint(memdc,lpString,nCount,
                workRect.right-workRect.left-szElipses.cx,
                &number, NULL, &sz);
            else
                GetTextExtentExPoint(memdc,lpString,nCount,
                0, &number, NULL, &sz);

            tem=(TCHAR*)mir_alloc((number+5)*sizeof(TCHAR));
            //memset(tem,0,(number+5)*sizeof(TCHAR));
            memcpy((void*)tem,lpString,number*sizeof(TCHAR));
            memcpy((void*)((TCHAR*)tem+number),_T("..."),3*sizeof(TCHAR));
            //tem[number+3]=(TCHAR)'\0';
            nCount=number+3;
            mir_free_and_nill(lpString);
            lpString=tem;

        }
    }
    // Calc Sizes
    {
        //Calc full text size
        //GetTextExtentPoint32(memdc,lpString,nCount,&sz);
        sz.cx+=2;
        fsize=sz;
        {
            if (workRect.right-workRect.left>sz.cx)
            {
                if (format&(DT_RIGHT|DT_RTLREADING)) drx=workRect.right-workRect.left-sz.cx;
                else if (format&DT_CENTER) 
                    drx=(workRect.right-workRect.left-sz.cx)>>1;
                else drx=0;
            }
            else
            {

                sz.cx=workRect.right-workRect.left;
                drx=0;
                // Calc buffer size
            }

            if (workRect.bottom-workRect.top>sz.cy)
            {
                if (format&DT_BOTTOM) dry=workRect.bottom-workRect.top-sz.cy;
                else if (format&DT_VCENTER) 
                    dry=(workRect.bottom-workRect.top-sz.cy)>>1;
                else dry=0;
            }
            else
            {
                sz.cy=workRect.bottom-workRect.top;
                dry=0;

            }
        }
        //if (sz.cy>2000) DebugBreak();
        sz.cx+=4;
        sz.cy+=4;
        if (sz.cx>0 && sz.cy>0)
        {
            //Create text bitmap
            {
                hbmp=SkinEngine_CreateDIB32Point(sz.cx,sz.cy,(void**)&bits);
                holdbmp=SelectObject(memdc,hbmp);

                bufDC=CreateCompatibleDC(hDC);
                bufbmp=SkinEngine_CreateDIB32Point(sz.cx,sz.cy,(void**)&bufbits);
                bufoldbmp=SelectObject(bufDC,bufbmp);
                BitBlt(bufDC,0,0,sz.cx,sz.cy,hDC,workRect.left+drx-2,workRect.top+dry-2,SRCCOPY);
            }
            //Calc text draw offsets
            //Draw text on temp bitmap
            {

                TextOut(memdc,2,2,lpString,nCount);
            }
            {
                MODERNEFFECT effect={0};
                if (SkinEngine_GetTextEffect(hDC,&effect)) 
                    SkinEngine_DrawTextEffect(bufbits,bits,sz.cx,sz.cy,&effect);	
            }
            //RenderText
            //if (1)
            {
                DWORD x,y;
                DWORD width=sz.cx;
                DWORD heigh=sz.cy;
                BYTE * ScanLine;
                BYTE * BufScanLine;
                BYTE * pix;
                BYTE * bufpix;
                BYTE r,g,b;
                BYTE al=255-((BYTE)(ARGBcolor>>24));
                r=GetRValue(ARGBcolor);
                g=GetGValue(ARGBcolor);
                b=GetBValue(ARGBcolor);      
                for (y=0+dtsy; y<heigh-dtey;y++)
                {
                    int a=y*(width<<2);
                    ScanLine=bits+a;
                    BufScanLine=bufbits+a;
                    for(x=0+dtsx; x<width-dtex; x++)
                    {
                        BYTE bx,rx,gx,mx;
                        pix=ScanLine+x*4;
                        bufpix=BufScanLine+(x<<2);
                        if (al!=255)
                        {
                            bx=pbGammaWeightAdv[pix[0]]*al/255;
                            gx=pbGammaWeightAdv[pix[1]]*al/255;
                            rx=pbGammaWeightAdv[pix[2]]*al/255;
                        }
                        else
                        {
                            bx=pbGammaWeightAdv[pix[0]];
                            gx=pbGammaWeightAdv[pix[1]];
                            rx=pbGammaWeightAdv[pix[2]];
                        }

                        bx=(pbGammaWeight[bx]*(255-b)+bx*(b))/255;
                        gx=(pbGammaWeight[gx]*(255-g)+gx*(g))/255;
                        rx=(pbGammaWeight[rx]*(255-r)+rx*(r))/255;

                        mx=(BYTE)(max(max(bx,rx),gx));

                        if (1) 
                        {
                            bx=(bx<mx)?(BYTE)(((WORD)bx*7+(WORD)mx)>>3):bx;
                            rx=(rx<mx)?(BYTE)(((WORD)rx*7+(WORD)mx)>>3):rx;
                            gx=(gx<mx)?(BYTE)(((WORD)gx*7+(WORD)mx)>>3):gx;
                            // reduce boldeness at white fonts
                        }
                        if (mx)                                      
                        {
                            short rrx,grx,brx;
                            BYTE axx=bufpix[3];
                            BYTE nx;
                            nx=pbGammaWeight[mx];
                            {


                                //Normalize components	to alpha level
                                bx=(nx*(255-axx)+bx*axx)/255;
                                gx=(nx*(255-axx)+gx*axx)/255;
                                rx=(nx*(255-axx)+rx*axx)/255;
                                mx=(nx*(255-axx)+mx*axx)/255;
                            }
                            {
                                brx=(short)((b-bufpix[0])*bx/255);
                                grx=(short)((g-bufpix[1])*gx/255);
                                rrx=(short)((r-bufpix[2])*rx/255);
                                bufpix[0]+=brx;
                                bufpix[1]+=grx;
                                bufpix[2]+=rrx;
                                bufpix[3]=(BYTE)(mx+(BYTE)(255-mx)*bufpix[3]/255);
                            }
                        }
                    }
                }

            }
            //Blend to destination
            {
                BitBlt(hDC,workRect.left+drx-2,workRect.top+dry-2,sz.cx,sz.cy,bufDC,0,0,SRCCOPY);
            }
            //free resources

            {
                SelectObject(memdc,holdbmp);
                DeleteObject(hbmp);
                SelectObject(bufDC,bufoldbmp);
                DeleteObject(bufbmp);
                mod_DeleteDC(bufDC);
            }	
        }
    }
    SelectObject(memdc,holdfnt);
    mod_DeleteDC(memdc); 
    if (noDIB) free(destBits);
    mir_free_and_nill(lpString);
    return 0;
}

BOOL SkinEngine_DrawTextA(HDC hdc, char * lpString, int nCount, RECT * lpRect, UINT format)
{
#ifdef UNICODE
    TCHAR *buf=a2u(lpString);
    BOOL res;
    res=SkinEngine_DrawText(hdc,buf,nCount,lpRect,format);
    mir_free_and_nill(buf);
    return res;
#else
    return SkinEngine_DrawText(hdc,lpString,nCount,lpRect,format);
#endif
}

BOOL SkinEngine_DrawText(HDC hdc, LPCTSTR lpString, int nCount, RECT * lpRect, UINT format)
{
    DWORD form=0, color=0;
    RECT r=*lpRect;
    OffsetRect(&r,1,1);
    if (format&DT_RTLREADING) SetTextAlign(hdc,TA_RTLREADING);
    if (format&DT_CALCRECT) return DrawText(hdc,lpString,nCount,lpRect,format);
    form=format;
    color=GetTextColor(hdc);
    if (!g_CluiData.fGDIPlusFail &&0) ///text via gdi+
    {
        TextOutWithGDIp(hdc,lpRect->left,lpRect->top,lpString,nCount);
        return 0;
    }
    return SkinEngine_AlphaTextOut(hdc,lpString,nCount,lpRect,form,color);
}

HICON SkinEngine_ImageList_GetIcon(HIMAGELIST himl, int i, UINT fStyle)
{
    IMAGEINFO imi={0};
    BITMAP bm={0};
	if (IsWinVerXPPlus()  && i!=-1)
	{
		ImageList_GetImageInfo(himl,i,&imi);
		GetObject(imi.hbmImage,sizeof(bm),&bm);
		if (bm.bmBitsPixel==32) //stupid bug of Microsoft 
								// Icons bitmaps are not premultiplied
								// So Imagelist_AddIcon - premultiply alpha
								// But incorrect - it is possible that alpha will
								// be less than color and
								// ImageList_GetIcon will return overflowed colors
								// TODO: Direct draw Icon from imagelist without 
								// extracting of icon
		{
			BYTE * bits=NULL;	
			bits=bm.bmBits;
			if (!bits)
			{
				bits=malloc(bm.bmWidthBytes*bm.bmHeight);
				GetBitmapBits(imi.hbmImage,bm.bmWidthBytes*bm.bmHeight,bits);
			} 
			{
				int iy;
				BYTE *bcbits;
				int wb=((imi.rcImage.right-imi.rcImage.left)*bm.bmBitsPixel>>3);
				bcbits=bits+(bm.bmHeight-imi.rcImage.bottom)*bm.bmWidthBytes+(imi.rcImage.left*bm.bmBitsPixel>>3);
				for (iy=0; iy<imi.rcImage.bottom-imi.rcImage.top; iy++)
				{
					int x;
					// Dummy microsoft fix - alpha can be less than r,g or b
					// Looks like color channels in icons should be non-premultiplied with alpha
					// But AddIcon store it premultiplied (incorrectly cause can be Alpha==7F, but R,G or B==80
					// So i check that alpha is 0x7F and set it to 0x80
					DWORD *c=((DWORD*)bcbits);
					for (x=0;x<imi.rcImage.right-imi.rcImage.left; x++)
					{		  
						DWORD val=*c;
						BYTE a= (BYTE)((val)>>24);
                        if (a!=0)
                        {
						    BYTE r= (BYTE)((val&0xFF0000)>>16);
						    BYTE g= (BYTE)((val&0xFF00)>>8);
						    BYTE b= (BYTE)(val&0xFF);
                            if (a<r || a<g || a<b)
						    {
							    a=max(max(r,g),b);
							    val=a<<24|r<<16|g<<8|b;
							    *c=val;
						    }
                        }
						c++;
					}
					bcbits+=bm.bmWidthBytes;
				}
			}  
			if (!bm.bmBits) 
			{ 
				SetBitmapBits(imi.hbmImage,bm.bmWidthBytes*bm.bmHeight,bits);
				free(bits);
			}
		}
	}
    return ImageList_GetIcon(himl,i,ILD_TRANSPARENT);
}

////////////////////////////////////////////////////////////////////////////
// This creates new dib image from Imagelist icon ready to alphablend it 

HBITMAP SkinEngine_ExtractDIBFromImagelistIcon( HIMAGELIST himl,int index, int * outWidth, int * outHeight)
{
    
    BITMAP      bmImg,
                bmMsk;
    
    IMAGEINFO   imi;

    HBITMAP     hOutBmp=NULL;


    int         iWidth,
                iHeight;

    int         iRowImgShift,
                iRowMskShift;

    BOOL        fDibImgBits,
                fDibMskBits;

    BYTE        *pImg=NULL,
                *pMsk=NULL,
                *pDib=NULL;

    BYTE        *pWorkImg=NULL,
                *pWorkMsk=NULL,
                *pWorkDib=NULL;

    BOOL        fHasMask,
                fHasAlpha;
    
    if (!ImageList_GetImageInfo(himl, index, &imi)) return NULL;
    
    iWidth=imi.rcImage.right-imi.rcImage.left;
    iHeight=imi.rcImage.bottom-imi.rcImage.top;

    if (iWidth<=0 && iHeight<=0) return NULL;
    
    GetObject(imi.hbmImage,sizeof(BITMAP),&bmImg);
    GetObject(imi.hbmMask,sizeof(BITMAP),&bmMsk);

    if (bmImg.bmBitsPixel!=32) 
    {
        // draw it on 32bit using native procedure
        HBITMAP hResultBmp=SkinEngine_CreateDIB32Point(iWidth,iHeight,&pDib);
        HDC hTempDC=CreateCompatibleDC(NULL);
        HBITMAP hOldBmp=SelectObject(hTempDC,hResultBmp);
        ImageList_DrawEx(himl,index,hTempDC,0,0,iWidth,iHeight,CLR_NONE,CLR_NONE,ILD_IMAGE);
        SelectObject(hTempDC,hOldBmp);
        DeleteDC(hTempDC);
        
        // and after tune up alpha layer via analyzing mask.       
        fDibMskBits = (bmMsk.bmBits!=NULL);
        if (!fDibMskBits)  //there is not dib section for mask
        {
            //lets create new pixel map for it
            DWORD dwSize=sizeof(BYTE)*bmMsk.bmHeight*bmMsk.bmWidthBytes;
            pMsk=(BYTE*)malloc(dwSize);
            // and fill it
            GetBitmapBits(imi.hbmMask,dwSize,pMsk);
        }
        else    
        {
            pMsk=bmMsk.bmBits;
        }

        if (!fDibMskBits)
        {
            iRowMskShift=bmMsk.bmWidthBytes;
            pWorkMsk=pMsk+((imi.rcImage.left*bmMsk.bmBitsPixel)>>3)+(imi.rcImage.top*iRowMskShift);  //top to bottom
        }
        else
        {
            iRowMskShift=-bmMsk.bmWidthBytes;
            pWorkMsk=pMsk+((imi.rcImage.left*bmMsk.bmBitsPixel)>>3)+((bmMsk.bmHeight-imi.rcImage.top-1)*bmMsk.bmWidthBytes); //bottom to top
        }
        
        if (hResultBmp)
        {   // lets analize it
            int x,y;
            BYTE *pRowDib;

            pWorkDib=pDib+(iHeight-1)*iWidth*4;
            //ok lets go...

            for (y=0; y<iHeight; y++)
            {
                pRowDib=pWorkDib;
                for (x=0; x<iWidth; x++)
                {
                    DWORD dwVal=*((DWORD*)pRowDib);
                    BOOL fMasked = SkinEngine_GetMaskBit(pWorkMsk,x);

                    if (fMasked) 
                        dwVal=0;                   // if mask bit is set - point have to be empty
                    else 
                        dwVal|=0xFF000000;         // if there not alpha channel let set it opaque

                    *((DWORD*)pRowDib)=dwVal;      // drop out if it is not zero

                    pRowDib+=4; 
                }
                pWorkMsk+=iRowMskShift;
                pWorkDib-=iWidth*sizeof(DWORD);
            }
        }
        //Cleanup
        if (!fDibMskBits && pMsk) 
            free(pMsk);

        // finally set output width and height
        if (outHeight) *outHeight=iHeight;
        if (outWidth)  *outWidth=iWidth;
        return hResultBmp; 
    } //end of non32bit mode
    
    // get bytes...
    fDibImgBits = (bmImg.bmBits!=NULL);
    fDibMskBits = (bmMsk.bmBits!=NULL);

    if (!fDibImgBits)  //there is not dib section for image
    {
        //lets create new pixel map for it
        DWORD dwSize=sizeof(BYTE)*bmImg.bmHeight*bmImg.bmWidthBytes;
        pImg=(BYTE*)malloc(dwSize);
        // and fill it
        GetBitmapBits(imi.hbmImage,dwSize,pImg);
    }
    else    
    {
        pImg=bmImg.bmBits;
    }

    if (!fDibMskBits)  //there is not dib section for mask
    {
        //lets create new pixel map for it
        DWORD dwSize=sizeof(BYTE)*bmMsk.bmHeight*bmMsk.bmWidthBytes;
        pMsk=(BYTE*)malloc(dwSize);
        // and fill it
        GetBitmapBits(imi.hbmMask,dwSize,pMsk);
    }
    else    
    {
        pMsk=bmMsk.bmBits;
    }
    
    // ok lets shift pointers according required image reference.
    {
        if (!fDibImgBits)
        {
            iRowImgShift=bmImg.bmWidthBytes; //top to bottom
            pWorkImg=pImg+((imi.rcImage.left*bmImg.bmBitsPixel)>>3)+(imi.rcImage.top*iRowImgShift);
        }
        else
        {
            iRowImgShift=-bmImg.bmWidthBytes;
            pWorkImg=pImg+((imi.rcImage.left*bmImg.bmBitsPixel)>>3)+((bmImg.bmHeight-imi.rcImage.top-1)*bmImg.bmWidthBytes); //bottom to top
        }
    
        if (!fDibMskBits)
        {
            iRowMskShift=bmMsk.bmWidthBytes;
            pWorkMsk=pMsk+((imi.rcImage.left*bmMsk.bmBitsPixel)>>3)+(imi.rcImage.top*iRowMskShift);  //top to bottom
        }
        else
        {
            iRowMskShift=-bmMsk.bmWidthBytes;
            pWorkMsk=pMsk+((imi.rcImage.left*bmMsk.bmBitsPixel)>>3)+((bmMsk.bmHeight-imi.rcImage.top-1)*bmMsk.bmWidthBytes); //bottom to top
        }
    }
    //pWork* are poited to start of mask and image area

    // lets check if required image really has non empty mask
    {
        BYTE x, y;
        BYTE * pCheckMsk=pWorkMsk;
        fHasMask=FALSE;
        for (y=0; !fHasMask && y<iHeight; y++)
        {
            for (x=0; !fHasMask && x<iWidth/8; x++)
            {
                if (pCheckMsk[x])
                {
                    fHasMask=TRUE;
                    break;
                }
            }
            pCheckMsk+=iRowMskShift;
        }
    }

    // lets check if required image really has not empty alpha channel
    {
        BYTE x, y;
        BYTE * pCheckAlpha=pWorkImg+3;
        fHasAlpha=FALSE;
        for (y=0; !fHasAlpha && y<iHeight; y++)
        {
            for (x=0; !fHasAlpha && x<iWidth; x++)
            {
                if (pCheckAlpha[x<<2])
                {
                    fHasAlpha=TRUE;
                    break;
                }
            }
            pCheckAlpha+=iRowImgShift;
        }
    }

    // We are ready to create output DIB image
    hOutBmp=SkinEngine_CreateDIB32Point(iWidth, iHeight, &pDib);
    if (hOutBmp)
    {        
        int x,y;
        
        BYTE *pRowImg;
        BYTE *pRowDib;
        
        pWorkDib=pDib+(iHeight-1)*iWidth*4;
        //ok lets go...
        
        for (y=0; y<iHeight; y++)
        {
            pRowImg=pWorkImg;
            pRowDib=pWorkDib;
            for (x=0; x<iWidth; x++)
            {
                DWORD dwVal=*((DWORD*)pRowImg);
                BOOL fMasked = fHasMask?SkinEngine_GetMaskBit(pWorkMsk,x):FALSE;

                if (fMasked) 
                    dwVal=0;                   // if mask bit is set - point have to be empty
                else if (!fHasAlpha)
                    dwVal|=0xFF000000; // if there not alpha channel let set it opaque
                
                if (dwVal!=0) *((DWORD*)pRowDib)=dwVal; // drop out if it is not zero

                pRowImg+=4;                             //shift to next pixel
                pRowDib+=4; 
            }
            pWorkImg+=iRowImgShift;
            pWorkMsk+=iRowMskShift;
            pWorkDib-=iWidth*sizeof(DWORD);
        }

        // finally set output width and height
        if (outHeight) *outHeight=iHeight;
        if (outWidth)  *outWidth=iWidth;
    }
    //Cleanup
    {
        if (!fDibImgBits && pImg)
            free(pImg);
        if (!fDibMskBits && pMsk) 
            free(pMsk);
    }
    return hOutBmp; 
}

BOOL SkinEngine_ImageList_DrawEx( HIMAGELIST himl,int i,HDC hdcDst,int x,int y,int dx,int dy,COLORREF rgbBk,COLORREF rgbFg,UINT fStyle)
{    
    //the routine to directly draw icon from image list without creating icon from there - should be some faster
    int iWidth, iHeight;
    BYTE alpha;

    HDC hDC;
    HBITMAP hBitmap;
    HBITMAP hOldBitmap;    

    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    
    if (i<0) return FALSE;

    hBitmap=SkinEngine_ExtractDIBFromImagelistIcon(himl, i, &iWidth, &iHeight);
    
    if (fStyle&ILD_BLEND25) alpha=64;
    else if (fStyle&ILD_BLEND50) alpha=128;
    else alpha=255;

    if (!hBitmap) 
    {        
        HICON hIcon=SkinEngine_ImageList_GetIcon(himl,i,ILD_NORMAL);        
        if (hIcon) 
        {
            SkinEngine_DrawIconEx(hdcDst,x,y,hIcon,dx?dx:iWidth,dy?dy:iHeight,0,NULL,DI_NORMAL|(alpha<<24));
            DestroyIcon(hIcon);
            return TRUE;
        }        
        return FALSE;
    }
    
    // ok looks like al fine lets draw it
    hDC=CreateCompatibleDC(hdcDst);
    hOldBitmap=SelectObject(hDC,hBitmap);

    bf.SourceConstantAlpha=alpha;    
    SkinEngine_AlphaBlend(hdcDst,x,y,dx?dx:iWidth,dy?dy:iHeight,hDC,0,0,iWidth,iHeight,bf);

    SelectObject(hDC,hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hDC);    
    return TRUE;
}


static BOOL SkinEngine_Service_DrawIconEx(WPARAM wParam,LPARAM lParam)
{
    DrawIconFixParam *p=(DrawIconFixParam*)wParam;
    if (!p) return 0;
    return SkinEngine_DrawIconEx(p->hdc,p->xLeft,p->yTop,p->hIcon,p->cxWidth,p->cyWidth,p->istepIfAniCur,p->hbrFlickerFreeDraw,p->diFlags);
}
BOOL SkinEngine_DrawIconEx(HDC hdcDst,int xLeft,int yTop,HICON hIcon,int cxWidth,int cyWidth, UINT istepIfAniCur, HBRUSH hbrFlickerFreeDraw, UINT diFlags)
{

    ICONINFO ici;
    BYTE alpha=(BYTE)((diFlags&0xFF000000)>>24);


    HDC imDC;
    HBITMAP oldBmp, imBmp,tBmp;
    BITMAP imbt,immaskbt;
    BYTE * imbits;
    BYTE * imimagbits;
    BYTE * immaskbits;
    DWORD cx,cy,icy;
    BYTE *t1, *t2, *t3;

    BOOL NoDIBImage=FALSE;
    //lockimagelist
    BYTE hasmask=FALSE;
    BYTE no32bit=FALSE;
    BYTE noMirrorMask=FALSE;
    BYTE hasalpha=FALSE;
    alpha=alpha?alpha:255;
    //return DrawIconEx(hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,DI_NORMAL);
    if (!GetIconInfo(hIcon,&ici))  return 0;

    GetObject(ici.hbmColor,sizeof(BITMAP),&imbt);
    if (imbt.bmWidth*imbt.bmHeight==0)
    {
        DeleteObject(ici.hbmColor);
        DeleteObject(ici.hbmMask);
        return 0;
    }
    GetObject(ici.hbmMask,sizeof(BITMAP),&immaskbt);
    cy=imbt.bmHeight;

    if (imbt.bmBitsPixel!=32)
    {
        HDC tempDC1;
        HBITMAP otBmp;
        no32bit=TRUE;
        tempDC1=CreateCompatibleDC(hdcDst);
        tBmp=SkinEngine_CreateDIB32(imbt.bmWidth,imbt.bmHeight);
        if (tBmp) 
        {
            GetObject(tBmp,sizeof(BITMAP),&imbt);
            otBmp=SelectObject(tempDC1,tBmp);
            DrawIconEx(tempDC1,0,0,hIcon,imbt.bmWidth,imbt.bmHeight,istepIfAniCur,hbrFlickerFreeDraw,DI_IMAGE);   
            noMirrorMask=TRUE;
        }
        SelectObject(tempDC1,otBmp);
        mod_DeleteDC(tempDC1);
    }

    if (imbt.bmBits==NULL)
    {
        NoDIBImage=TRUE;
        imimagbits=(BYTE*)malloc(cy*imbt.bmWidthBytes);
        GetBitmapBits(ici.hbmColor,cy*imbt.bmWidthBytes,(void*)imimagbits);
    }
    else imimagbits=imbt.bmBits;


    if (immaskbt.bmBits==NULL)
    {
        immaskbits=(BYTE*)malloc(cy*immaskbt.bmWidthBytes);
        GetBitmapBits(ici.hbmMask,cy*immaskbt.bmWidthBytes,(void*)immaskbits);
    }
    else immaskbits=immaskbt.bmBits;
    icy=imbt.bmHeight;
    cx=imbt.bmWidth;
    imDC=CreateCompatibleDC(hdcDst);
    imBmp=SkinEngine_CreateDIB32Point(cx,icy,&imbits);
    oldBmp=SelectObject(imDC,imBmp);
    if (imbits!=NULL && imimagbits!=NULL && immaskbits!=NULL)
    {
        int x; int y;
        int bottom,right,top,h;
        int mwb,mwb2;
        mwb=immaskbt.bmWidthBytes;
        mwb2=imbt.bmWidthBytes;
        bottom=icy;
        right=cx;   
        top=0;
        h=icy;
        for (y=top;(y<bottom)&&!hasmask; y++)
        { 
            t1=immaskbits+y*mwb;
            for (x=0; (x<mwb)&&!hasmask; x++)
                hasmask|=(*(t1+x)!=0);
        }

        for (y=top;(y<bottom)&&!hasalpha; y++)
        {
            t1=imimagbits+(cy-y-1)*mwb2;
            for (x=0; (x<right)&&!hasalpha; x++)
                hasalpha|=(*(t1+(x<<2)+3)!=0);
        }

        for (y=0; y<(int)icy; y++)
        {
            t1=imimagbits+(h-y-1-top)*mwb2;
            t2=imbits+(!no32bit?y:(icy-y-1))*mwb2;
            t3=immaskbits+(noMirrorMask?y:(h-y-1-top))*mwb;
            for (x=0; x<right; x++)
            {
                DWORD * src, *dest;               
                BYTE mask=0;
                BYTE a; 
                src=(DWORD*)(t1+(x<<2));
                dest=(DWORD*)(t2+(x<<2));              
                if (hasalpha && !hasmask)  
                    a=((BYTE*)src)[3];
                else
                { 
                    mask=((1<<(7-x%8))&(*(t3+(x>>3))))!=0;
                    if (mask)// && !hasalpha)		
                    {
                        if (!hasalpha) 
                        { *dest=0; continue; }
                        else 
                            a=((BYTE*)src)[3]>0?((BYTE*)src)[3]:0;//255;
                    }
                    else if (hasalpha || hasmask)
                        a=(((BYTE*)src)[3]>0?((BYTE*)src)[3]:255);
                    else if (!hasalpha && !hasmask)
                        a=255;
                    else {  *dest=0; continue; }
                }
                if (a>0)
                {
                    ((BYTE*)dest)[3]=a;
                    ((BYTE*)dest)[0]=((BYTE*)src)[0]*a/255;
                    ((BYTE*)dest)[1]=((BYTE*)src)[1]*a/255;
                    ((BYTE*)dest)[2]=((BYTE*)src)[2]*a/255;
                }
                else 
                    *dest=0;
            }
        }
    }
    {
        BLENDFUNCTION bf={AC_SRC_OVER, diFlags&128, alpha, AC_SRC_ALPHA };   
        SkinEngine_AlphaBlend(hdcDst,xLeft,yTop,cxWidth, cyWidth, imDC,0,0, cx,icy,bf);
    }

    if (immaskbt.bmBits==NULL) free(immaskbits);
    if (imbt.bmBits==NULL) free(imimagbits);
    SelectObject(imDC,oldBmp);
    DeleteObject(imBmp);
    if(no32bit)DeleteObject(tBmp);
    DeleteObject(ici.hbmColor);
    DeleteObject(ici.hbmMask);
    SelectObject(imDC,GetStockObject(DEFAULT_GUI_FONT));
    mod_DeleteDC(imDC);
    return 1;// DrawIconExS(hdc,xLeft,yTop,hIcon,cxWidth,cyWidth,istepIfAniCur,hbrFlickerFreeDraw,diFlags);
}

static int SkinEngine_Service_RegisterFramePaintCallbackProcedure(WPARAM wParam, LPARAM lParam)
{
    if (!wParam) return 0;
    {
        wndFrame *frm=FindFrameByItsHWND((HWND)wParam);
        if (!frm) return 0;
        if (lParam)
            frm->PaintCallbackProc=(tPaintCallbackProc)lParam;
        else
            frm->PaintCallbackProc=NULL;
        return 1;
    }
}

int SkinEngine_PrepeareImageButDontUpdateIt(RECT * r)
{
    if (g_CluiData.fLayered)
    {
        mutex_bLockUpdate=1;
        SkinEngine_DrawNonFramedObjects(TRUE,r);
        SkinEngine_ValidateFrameImageProc(r);
        mutex_bLockUpdate=0;
        return 0;
    }
    else
    {
        return SkinEngine_ReCreateBackImage(FALSE,r);
    }
    return 0;
}

int SkinEngine_RedrawCompleteWindow()
{   
    if (g_CluiData.fLayered)
    { 
        SkinEngine_DrawNonFramedObjects(TRUE,0);
        SkinEngine_Service_InvalidateFrameImage(0,0);   
    }
    else
    {
        RedrawWindow(pcli->hwndContactList,NULL,NULL,RDW_ALLCHILDREN|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
    }
    return 0;
}
// Request to repaint frame or change/drop callback data
// wParam = hWnd of called frame
// lParam = pointer to sPaintRequest (or NULL to redraw all)
// return 2 - already queued, data updated, 1-have been queued, 0 - failure

int SkinEngine_Service_UpdateFrameImage(WPARAM wParam, LPARAM lParam)           // Immideately recall paint routines for frame and refresh image
{
    RECT wnd;
    wndFrame *frm;
    BOOL NoCancelPost=0;
    BOOL IsAnyQueued=0;
    if (!g_CluiData.mutexOnEdgeSizing)
        GetWindowRect(pcli->hwndContactList,&wnd);
    else
        wnd=g_rcEdgeSizingRect;
    if (!g_CluiData.fLayered)
    {
        RedrawWindow((HWND)wParam,NULL,NULL,RDW_UPDATENOW|RDW_ERASE|RDW_INVALIDATE|RDW_FRAME);
        return 0;
    }
    if (g_pCachedWindow==NULL) SkinEngine_ValidateFrameImageProc(&wnd);
    else if (g_pCachedWindow->Width!=wnd.right-wnd.left || g_pCachedWindow->Height!=wnd.bottom-wnd.top) SkinEngine_ValidateFrameImageProc(&wnd);
    else if (wParam==0) SkinEngine_ValidateFrameImageProc(&wnd);
    else // all Ok Update Single Frame
    {
        frm=FindFrameByItsHWND((HWND)wParam);
        if (!frm)  SkinEngine_ValidateFrameImageProc(&wnd);
        // Validate frame, update window image and remove it from queue
        else 
        {
            if(frm->UpdateRgn)
            {
                DeleteObject(frm->UpdateRgn);
                frm->UpdateRgn=0;
            }
            SkinEngine_ValidateSingleFrameImage(frm,0);
            SkinEngine_UpdateWindowImage();
            NoCancelPost=1;
            //-- Remove frame from queue
            if (flag_bUpdateQueued)
            {
                int i;
                frm->bQueued=0;
                for(i=0;i<nFramescount;i++)
                    if(IsAnyQueued|=Frames[i].bQueued) break;
            }
        }
    }       
    if ((!NoCancelPost || !IsAnyQueued) && flag_bUpdateQueued) // no any queued updating cancel post or need to cancel post
    {
        flag_bUpdateQueued=0;
        g_flag_bPostWasCanceled=1;
    }
    return 1;   
}
int SkinEngine_Service_InvalidateFrameImage(WPARAM wParam, LPARAM lParam)       // Post request for updating
{

    if (wParam)
    {
        wndFrame *frm=FindFrameByItsHWND((HWND)wParam);
        sPaintRequest * pr=(sPaintRequest*)lParam;
        if (!g_CluiData.fLayered || (frm && frm->floating)) return InvalidateRect((HWND)wParam,pr?(RECT*)&(pr->rcUpdate):NULL,FALSE);
        if (frm) 
        {
            if (frm->PaintCallbackProc!=NULL)
            {
                frm->PaintData=(sPaintRequest *)pr;
                frm->bQueued=1;
                if (pr)
                {
                    HRGN r2;
                    if (!IsRectEmpty(&pr->rcUpdate))
                        r2=CreateRectRgn(pr->rcUpdate.left,pr->rcUpdate.top,pr->rcUpdate.right,pr->rcUpdate.bottom);
                    else
                    {
                        RECT r;
                        GetClientRect(frm->hWnd,&r);
                        r2=CreateRectRgn(r.left,r.top,r.right,r.bottom);
                    }
                    if(!frm->UpdateRgn)
                    {
                        frm->UpdateRgn=CreateRectRgn(0,0,1,1);
                        CombineRgn(frm->UpdateRgn,r2,0,RGN_COPY);                                            
                    }
                    else CombineRgn(frm->UpdateRgn,frm->UpdateRgn,r2,RGN_OR);
                    DeleteObject(r2);
                }   

            }
        }      
        else
        {
            QueueAllFramesUpdating(1);
        }
    }
    else QueueAllFramesUpdating(1);
    if (!flag_bUpdateQueued||g_flag_bPostWasCanceled)
        if (PostMessage(pcli->hwndContactList,UM_UPDATE,0,0))
        {            
            flag_bUpdateQueued=1;
            g_flag_bPostWasCanceled=0;
        }
        return 1;
}


static int SkinEngine_ValidateSingleFrameImage(wndFrame * Frame, BOOL SkipBkgBlitting)                              // Calling frame paint proc
{
    if (!g_pCachedWindow) { TRACE("SkinEngine_ValidateSingleFrameImage calling without cached\n"); return 0;}
    if (Frame->hWnd==(HWND)-1 && !Frame->PaintCallbackProc)  { TRACE("SkinEngine_ValidateSingleFrameImage calling without FrameProc\n"); return 0;}
    { // if ok update image 
        HDC hdc;
        HBITMAP o,n;
        RECT rcPaint,wnd;
        RECT ru={0};
        int w,h,x,y;
        int w1,h1,x1,y1;

        CLUI_SizingGetWindowRect(pcli->hwndContactList,&wnd);
        rcPaint=Frame->wndSize;
        {
            int dx,dy,bx,by;
            if (g_CluiData.mutexOnEdgeSizing)
            {
                dx=rcPaint.left-wnd.left;
                dy=rcPaint.top-wnd.top;
                bx=rcPaint.right-wnd.right;
                by=rcPaint.bottom-wnd.bottom;
                wnd=g_rcEdgeSizingRect;
                rcPaint.left=wnd.left+dx;
                rcPaint.top=wnd.top+dy;
                rcPaint.right=wnd.right+bx;
                rcPaint.bottom=wnd.bottom+by;
            }
        }
        //OffsetRect(&rcPaint,-wnd.left,-wnd.top);
        w=rcPaint.right-rcPaint.left;
        h=rcPaint.bottom-rcPaint.top;
        if (w<=0 || h<=0) 
        { 
            TRACE("Frame size smaller than 0\n");
            return 0;
        }
        x=rcPaint.left;
        y=rcPaint.top;
        hdc=CreateCompatibleDC(g_pCachedWindow->hImageDC);
        n=SkinEngine_CreateDIB32(w,h);
        o=SelectObject(hdc,n);
        {
            HRGN rgnUpdate=0;

            if (Frame->UpdateRgn && !SkipBkgBlitting)
            {

                rgnUpdate=Frame->UpdateRgn;
                GetRgnBox(rgnUpdate,&ru);
                {
                    RECT rc;
                    GetClientRect(Frame->hWnd,&rc);
                    if (ru.top<0) ru.top=0;
                    if (ru.left<0) ru.left=0;
                    if (ru.right>rc.right) ru.right=rc.right;
                    if (ru.bottom>rc.bottom) ru.bottom=rc.bottom;
                } 
                if (!IsRectEmpty(&ru))
                {
                    x1=ru.left;
                    y1=ru.top;
                    w1=ru.right-ru.left;
                    h1=ru.bottom-ru.top;
                }
                else
                {x1=0; y1=0; w1=w; h1=h;}
                // copy image at hdc
                if (SkipBkgBlitting)  //image already at foreground
                {
                    BitBlt(hdc,x1,y1,w1,h1,g_pCachedWindow->hImageDC,x+x1,y+y1,SRCCOPY);  
                }
                else
                {
                    BitBlt(hdc,x1,y1,w1,h1,g_pCachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);  
                }
                Frame->PaintCallbackProc(Frame->hWnd,hdc,&ru,rgnUpdate, Frame->dwFlags,Frame->PaintData);
            }
            else
            {
                RECT r;
                GetClientRect(Frame->hWnd,&r);
                rgnUpdate=CreateRectRgn(r.left,r.top,r.right,r.bottom); 
                ru=r;
                if (!IsRectEmpty(&ru))
                {
                    x1=ru.left;
                    y1=ru.top;
                    w1=ru.right-ru.left;
                    h1=ru.bottom-ru.top;
                }
                else
                {x1=0; y1=0; w1=w; h1=h;}
                // copy image at hdc
                if (SkipBkgBlitting)  //image already at foreground
                {
                    BitBlt(hdc,x1,y1,w1,h1,g_pCachedWindow->hImageDC,x+x1,y+y1,SRCCOPY);  
                }
                else
                {
                    BitBlt(hdc,x1,y1,w1,h1,g_pCachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);  
                }
                Frame->PaintCallbackProc(Frame->hWnd,hdc,&r,rgnUpdate, Frame->dwFlags,Frame->PaintData);
                ru=r;
            }
            DeleteObject(rgnUpdate);
            Frame->UpdateRgn=0;
        }
        if (!IsRectEmpty(&ru))
        {
            x1=ru.left;
            y1=ru.top;
            w1=ru.right-ru.left;
            h1=ru.bottom-ru.top;
        }
        else
        {x1=0; y1=0; w1=w; h1=h;}
        /*  if (!SkipBkgBlitting)
        {
        BitBlt(g_pCachedWindow->hImageDC,x+x1,y+y1,w1,h1,g_pCachedWindow->hBackDC,x+x1,y+y1,SRCCOPY);
        }

        */  
        {
            //BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            BitBlt(g_pCachedWindow->hImageDC,x+x1,y+y1,w1,h1,hdc,x1,y1,SRCCOPY);
            //BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
            //MyAlphaBlend(g_pCachedWindow->hImageDC,x+x1,y+y1,w1,h1,hdc,x1,y1,w1,h1,bf);  
        }
        {
            if (GetWindowLong(Frame->hWnd,GWL_STYLE)&WS_VSCROLL)
            {
                //Draw vertical scroll bar
                //
                RECT rThumb;
                RECT rUpBtn;
                RECT rDnBtn;
                RECT rLine;
                int dx,dy;
                SCROLLBARINFO si={0};
                si.cbSize=sizeof(SCROLLBARINFO);
                GetScrollBarInfo(Frame->hWnd,OBJID_VSCROLL,&si);
                rLine=(si.rcScrollBar);
                rLine.left=(rLine.left>0)?rLine.left:rLine.right-20;
                rUpBtn=rLine;
                rDnBtn=rLine;
                rThumb=rLine;

                rUpBtn.bottom=rUpBtn.top+si.dxyLineButton;
                rDnBtn.top=rDnBtn.bottom-si.dxyLineButton;
                rThumb.top=rLine.top+si.xyThumbTop;
                rThumb.bottom=rLine.top+si.xyThumbBottom;
                {
                    dx=Frame->wndSize.right-rLine.right;
                    dy=-rLine.top+Frame->wndSize.top;
                }
                OffsetRect(&rLine,dx,dy);
                OffsetRect(&rUpBtn,dx,dy);
                OffsetRect(&rDnBtn,dx,dy);
                OffsetRect(&rThumb,dx,dy);
                BitBlt(g_pCachedWindow->hImageDC,rLine.left,rLine.top,rLine.right-rLine.left,rLine.bottom-rLine.top,g_pCachedWindow->hBackDC,rLine.left,rLine.top,SRCCOPY);
                {
                    char req[255];
                    _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=Back",Frame->name);
                    SkinDrawGlyph(g_pCachedWindow->hImageDC,&rLine,&rLine,req);
                    _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=Thumb",Frame->name);
                    SkinDrawGlyph(g_pCachedWindow->hImageDC,&rThumb,&rThumb,req);
                    _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=UpLineButton",Frame->name);
                    SkinDrawGlyph(g_pCachedWindow->hImageDC,&rUpBtn,&rUpBtn,req);
                    _snprintf(req,sizeof(req),"Main,ID=ScrollBar,Frame=%s,Part=DownLineButton",Frame->name);
                    SkinDrawGlyph(g_pCachedWindow->hImageDC,&rDnBtn,&rDnBtn,req);
                }
            }

        }
        SelectObject(hdc,o);
        DeleteObject(n);
        mod_DeleteDC(hdc);
    }
    return 1;
}

int SkinEngine_BltBackImage (HWND destHWND, HDC destDC, RECT * BltClientRect)
{
    POINT ptMainWnd={0};
    POINT ptChildWnd={0};
    RECT from={0};
    RECT w={0};
    SkinEngine_ReCreateBackImage(FALSE,NULL);
    if (BltClientRect) w=*BltClientRect;
    else GetClientRect(destHWND,&w);
    ptChildWnd.x=w.left;
    ptChildWnd.y=w.top;
    ClientToScreen(destHWND,&ptChildWnd);
    ClientToScreen(pcli->hwndContactList,&ptMainWnd);
    //TODO if main not relative to client area
    return BitBlt(destDC,w.left,w.top,(w.right-w.left),(w.bottom-w.top),g_pCachedWindow->hBackDC,(ptChildWnd.x-ptMainWnd.x),(ptChildWnd.y-ptMainWnd.y),SRCCOPY);

}
int SkinEngine_ReCreateBackImage(BOOL Erase,RECT *w)
{
    HBITMAP hb2;
    RECT wnd={0};
    BOOL IsNewCache=0;
    GetClientRect(pcli->hwndContactList,&wnd);
    if (w) wnd=*w;
    //-- Check cached.
    if (g_pCachedWindow==NULL)
    {
        //-- Create New Cache
        {
            g_pCachedWindow=(CURRWNDIMAGEDATA*)mir_alloc(sizeof(CURRWNDIMAGEDATA));
            memset(g_pCachedWindow,0,sizeof(CURRWNDIMAGEDATA));
            g_pCachedWindow->hScreenDC=GetDC(NULL);
            g_pCachedWindow->hBackDC=CreateCompatibleDC(g_pCachedWindow->hScreenDC);
            g_pCachedWindow->hImageDC=CreateCompatibleDC(g_pCachedWindow->hScreenDC);
            g_pCachedWindow->Width=wnd.right-wnd.left;
            g_pCachedWindow->Height=wnd.bottom-wnd.top;
            if (g_pCachedWindow->Width!=0 && g_pCachedWindow->Height!=0)
            {
                g_pCachedWindow->hImageDIB=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte));
                g_pCachedWindow->hBackDIB=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hBackDIBByte));
                g_pCachedWindow->hImageOld=SelectObject(g_pCachedWindow->hImageDC,g_pCachedWindow->hImageDIB);
                g_pCachedWindow->hBackOld=SelectObject(g_pCachedWindow->hBackDC,g_pCachedWindow->hBackDIB);
            }
        }
        IsNewCache=1;
    }   
    if (g_pCachedWindow->Width!=wnd.right-wnd.left || g_pCachedWindow->Height!=wnd.bottom-wnd.top)		
    {
        HBITMAP hb1=NULL,hb2=NULL;
        g_pCachedWindow->Width=wnd.right-wnd.left;
        g_pCachedWindow->Height=wnd.bottom-wnd.top;
        if (g_pCachedWindow->Width!=0 && g_pCachedWindow->Height!=0)
        {
            hb1=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte));
            hb2=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte)); 
            SelectObject(g_pCachedWindow->hImageDC,hb1);
            SelectObject(g_pCachedWindow->hBackDC,hb2);
        }
        else
        {
            SelectObject(g_pCachedWindow->hImageDC,g_pCachedWindow->hImageOld);
            SelectObject(g_pCachedWindow->hBackDC,g_pCachedWindow->hBackOld);
        }
        if (g_pCachedWindow->hImageDIB) DeleteObject(g_pCachedWindow->hImageDIB);
        if (g_pCachedWindow->hBackDIB) DeleteObject(g_pCachedWindow->hBackDIB);
        g_pCachedWindow->hImageDIB=hb1;
        g_pCachedWindow->hBackDIB=hb2;
        IsNewCache=1;
    }
    if ((Erase || IsNewCache )&& (g_pCachedWindow->Width!=0 && g_pCachedWindow->Height!=0))
    {

        hb2=SkinEngine_CreateDIB32(g_pCachedWindow->Width,g_pCachedWindow->Height); 
        SelectObject(g_pCachedWindow->hBackDC,hb2);
        DeleteObject(g_pCachedWindow->hBackDIB);
        g_pCachedWindow->hBackDIB=hb2;
        FillRect(g_pCachedWindow->hBackDC,&wnd,GetSysColorBrush(COLOR_BTNFACE));
        SkinDrawGlyph(g_pCachedWindow->hBackDC,&wnd,&wnd,"Main,ID=Background,Opt=Non-Layered");
        SkinEngine_SetRectOpaque(g_pCachedWindow->hBackDC,&wnd);
    }
    return 1;
}
int SkinEngine_DrawNonFramedObjects(BOOL Erase,RECT *r)
{
    RECT w,wnd;
    if (r) w=*r;
    else CLUI_SizingGetWindowRect(pcli->hwndContactList,&w);
    if (!g_CluiData.fLayered) return SkinEngine_ReCreateBackImage(FALSE,0);
    if (g_pCachedWindow==NULL)
        return SkinEngine_ValidateFrameImageProc(&w);

    wnd=w;
    OffsetRect(&w, -w.left, -w.top);
    if (Erase)
    {
        HBITMAP hb2;
        hb2=SkinEngine_CreateDIB32(g_pCachedWindow->Width,g_pCachedWindow->Height); 
        SelectObject(g_pCachedWindow->hBackDC,hb2);
        DeleteObject(g_pCachedWindow->hBackDIB);
        g_pCachedWindow->hBackDIB=hb2;
    }

    SkinDrawGlyph(g_pCachedWindow->hBackDC,&w,&w,"Main,ID=Background");
    //--Draw frames captions
    {
        int i;
        for(i=0;i<nFramescount;i++)
            if (Frames[i].TitleBar.ShowTitleBar && Frames[i].visible && !Frames[i].floating)
            {
                RECT rc;
                SetRect(&rc,Frames[i].wndSize.left,Frames[i].wndSize.top-g_nTitleBarHeight-g_nGapBetweenTitlebar,Frames[i].wndSize.right,Frames[i].wndSize.top-g_nGapBetweenTitlebar);
                //GetWindowRect(Frames[i].TitleBar.hwnd,&rc);
                //OffsetRect(&rc,-wnd.left,-wnd.top);
                DrawTitleBar(g_pCachedWindow->hBackDC,rc,Frames[i].id);
            }
    }
    g_mutex_bLockUpdating=1;

    flag_bJustDrawNonFramedObjects=1;
    return 0;
}
int SkinEngine_ValidateFrameImageProc(RECT * r)                                // Calling queued frame paint procs and refresh image
{
    RECT wnd={0};
    BOOL IsNewCache=0;
    BOOL IsForceAllPainting=0;
    if (r) wnd=*r;
    else GetWindowRect(pcli->hwndContactList,&wnd);
    if (wnd.right-wnd.left==0 || wnd.bottom-wnd.top==0) return 0;
    g_mutex_bLockUpdating=1;
    //-- Check cached.
    if (g_pCachedWindow==NULL)
    {
        //-- Create New Cache
        {
            g_pCachedWindow=(CURRWNDIMAGEDATA*)mir_alloc(sizeof(CURRWNDIMAGEDATA));
            g_pCachedWindow->hScreenDC=GetDC(NULL);
            g_pCachedWindow->hBackDC=CreateCompatibleDC(g_pCachedWindow->hScreenDC);
            g_pCachedWindow->hImageDC=CreateCompatibleDC(g_pCachedWindow->hScreenDC);
            g_pCachedWindow->Width=wnd.right-wnd.left;
            g_pCachedWindow->Height=wnd.bottom-wnd.top;
            g_pCachedWindow->hImageDIB=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte));
            g_pCachedWindow->hBackDIB=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hBackDIBByte));
            g_pCachedWindow->hImageOld=SelectObject(g_pCachedWindow->hImageDC,g_pCachedWindow->hImageDIB);
            g_pCachedWindow->hBackOld=SelectObject(g_pCachedWindow->hBackDC,g_pCachedWindow->hBackDIB);
        }
        IsNewCache=1;
    }   
    if (g_pCachedWindow->Width!=wnd.right-wnd.left || g_pCachedWindow->Height!=wnd.bottom-wnd.top)
    {
        HBITMAP hb1,hb2;
        g_pCachedWindow->Width=wnd.right-wnd.left;
        g_pCachedWindow->Height=wnd.bottom-wnd.top;
        hb1=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte));
        hb2=SkinEngine_CreateDIB32Point(g_pCachedWindow->Width,g_pCachedWindow->Height,&(g_pCachedWindow->hImageDIBByte)); 
        SelectObject(g_pCachedWindow->hImageDC,hb1);
        SelectObject(g_pCachedWindow->hBackDC,hb2);
        DeleteObject(g_pCachedWindow->hImageDIB);
        DeleteObject(g_pCachedWindow->hBackDIB);
        g_pCachedWindow->hImageDIB=hb1;
        g_pCachedWindow->hBackDIB=hb2;
        IsNewCache=1;
    }
    if (IsNewCache)
    {
        SkinEngine_DrawNonFramedObjects(0,&wnd);       
        IsForceAllPainting=1;
    }
    if (flag_bJustDrawNonFramedObjects)
    {
        IsForceAllPainting=1;
        flag_bJustDrawNonFramedObjects=0;
    }
    if (IsForceAllPainting) 
    { 
        BitBlt(g_pCachedWindow->hImageDC,0,0,g_pCachedWindow->Width,g_pCachedWindow->Height,g_pCachedWindow->hBackDC,0,0,SRCCOPY);
        QueueAllFramesUpdating(1);
    }
    //-- Validating frames
    { 
        int i;
        for(i=0;i<nFramescount;i++)
            if (Frames[i].PaintCallbackProc && Frames[i].visible && !Frames[i].floating )
                if (Frames[i].bQueued || IsForceAllPainting)
                    SkinEngine_ValidateSingleFrameImage(&Frames[i],IsForceAllPainting);
    }
    g_mutex_bLockUpdating=1;
    RedrawButtons(0);
    g_mutex_bLockUpdating=0;
    if (!mutex_bLockUpdate)  SkinEngine_UpdateWindowImageRect(&wnd);
    //-- Clear queue
    {
        QueueAllFramesUpdating(0);
        flag_bUpdateQueued=0;
        g_flag_bPostWasCanceled=0;
    }
    return 1;
}

int SkinEngine_UpdateWindowImage()
{
    if (MirandaExiting()) 
        return 0;
    if (g_CluiData.fLayered)
    {
        RECT r;
        GetWindowRect(pcli->hwndContactList,&r);
        return SkinEngine_UpdateWindowImageRect(&r);
    }
    else
        SkinEngine_ReCreateBackImage(FALSE,0);
    SkinEngine_ApplyTransluency();
    return 0;
}


int SkinEngine_UpdateWindowImageRect(RECT * r)                                     // Update window with current image and 
{
    //if not validity -> ValidateImageProc
    //else Update using current alpha
    RECT wnd=*r;
    if (!g_CluiData.fLayered) return SkinEngine_ReCreateBackImage(FALSE,0);
    if (g_pCachedWindow==NULL) return SkinEngine_ValidateFrameImageProc(&wnd); 
    if (g_pCachedWindow->Width!=wnd.right-wnd.left || g_pCachedWindow->Height!=wnd.bottom-wnd.top) return SkinEngine_ValidateFrameImageProc(&wnd);
    if (g_flag_bFullRepaint) 
    {
        g_flag_bFullRepaint=0; 
        return SkinEngine_ValidateFrameImageProc(&wnd);
    }
    SkinEngine_JustUpdateWindowImageRect(&wnd);
    return 0;
}

void SkinEngine_ApplyTransluency()
{
    int IsTransparancy;
    HWND hwnd=pcli->hwndContactList;
    BOOL layered=(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED)?TRUE:FALSE;

    IsTransparancy=g_CluiData.fSmoothAnimation || g_bTransparentFlag;
    if (!g_bTransparentFlag && !g_CluiData.fSmoothAnimation && g_CluiData.bCurrentAlpha!=0)
        g_CluiData.bCurrentAlpha=255;
    if (!g_CluiData.fLayered && (/*(g_CluiData.bCurrentAlpha==255)||*/(g_proc_SetLayeredWindowAttributesNew && IsTransparancy)))
    {
        if (!layered) SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        if (g_proc_SetLayeredWindowAttributesNew) g_proc_SetLayeredWindowAttributesNew(hwnd, RGB(0,0,0), (BYTE)g_CluiData.bCurrentAlpha, LWA_ALPHA);
    }
    return;
}

int SkinEngine_JustUpdateWindowImage()
{
    RECT r;
    if (!g_CluiData.fLayered)
    {
        SkinEngine_ApplyTransluency();
        return 0;
    }
    GetWindowRect(pcli->hwndContactList,&r);
    return SkinEngine_JustUpdateWindowImageRect(&r);
}
int SkinEngine_JustUpdateWindowImageRect(RECT * rty)
//Update window image
{
    BLENDFUNCTION bf={AC_SRC_OVER, 0,g_CluiData.bCurrentAlpha, AC_SRC_ALPHA };
    POINT dest={0}, src={0};
    int res;
    RECT wnd=*rty;

    RECT rect;
    SIZE sz={0};

    if (!g_CluiData.fLayered)
    {
        SkinEngine_ApplyTransluency();
        return 0;
    }
    if (!pcli->hwndContactList) return 0;
    rect=wnd;
    dest.x=rect.left;
    dest.y=rect.top;
    sz.cx=rect.right-rect.left;
    sz.cy=rect.bottom-rect.top;
    if (g_proc_UpdateLayeredWindow && g_CluiData.fLayered)
    {
        if (!(GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE)&WS_EX_LAYERED))
            SetWindowLong(pcli->hwndContactList,GWL_EXSTYLE, GetWindowLong(pcli->hwndContactList, GWL_EXSTYLE) |WS_EX_LAYERED);
        SetAlpha(g_CluiData.bCurrentAlpha);

        res=g_proc_UpdateLayeredWindow(pcli->hwndContactList,g_pCachedWindow->hScreenDC,&dest,&sz,g_pCachedWindow->hImageDC,&src,RGB(1,1,1),&bf,ULW_ALPHA);
    }
    else InvalidateRect(pcli->hwndContactList,NULL,TRUE);
    return 0;
}

int SkinEngine_DrawImageAt(HDC hdc, RECT *rc)
{
    BLENDFUNCTION bf={AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    BitBlt(g_pCachedWindow->hImageDC,rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,g_pCachedWindow->hBackDC,rc->left,rc->top,SRCCOPY);
    SkinEngine_AlphaBlend(g_pCachedWindow->hImageDC,rc->left,rc->top,rc->right-rc->left,rc->bottom-rc->top,hdc,0,0,rc->right-rc->left,rc->bottom-rc->top,bf);    
    if (!g_mutex_bLockUpdating)
        SkinEngine_UpdateWindowImage();
    return 0;
}

HBITMAP SkinEngine_GetCurrentWindowImage()
{ 
    return g_pCachedWindow->hImageDIB;
}

/*
*  Glyph text routine
*/


static int SkinEngine_SortTextGlyphObjectFunc(void * first, void * second)
{
    return strcmp(((GLYPHTEXT*)(((int*)first)[0]))->szGlyphTextID,((GLYPHTEXT*)(((int*)second)[0]))->szGlyphTextID);
}

static DWORD SkinEngine_HexToARGB(char * Hex)
{
    char buf[10]={0};
    char buf2[11]={0};
    char * st;
    BYTE alpha;
    DWORD AARRGGBB=0;
    _snprintf(buf,10,"%s\n",Hex);
    if (buf[1]=='x' || buf[1]=='X')
        _snprintf(buf2,11,"0x%s\n",buf+2);
    else
        _snprintf(buf2,11,"0x%s\n",buf);
    buf2[10]='\0';
    AARRGGBB=strtoul(buf2,&st,16); 
    alpha=(BYTE)((AARRGGBB&0xFF000000)>>24);
    alpha=255-((alpha==0)?255:alpha); 
    AARRGGBB=(alpha<<24)+((AARRGGBB&0x00FF0000)>>16)+((AARRGGBB&0x000000FF)<<16)+(AARRGGBB&0x0000FF00);
    return AARRGGBB;
}

static TCHAR *SkinEngine_ReAppend(TCHAR *lfirst, TCHAR * lsecond, int len)
{ 
    int l1=lfirst?lstrlen(lfirst)*sizeof(TCHAR):0;
    int l2=(len?len:lstrlen(lsecond))*sizeof(TCHAR);
    int size=l1+l2+sizeof(TCHAR);
    TCHAR *buf=mir_alloc(size);
    if (lfirst) memmove(buf,lfirst,l1);
    memmove(((BYTE*)buf)+l1,lsecond,l2+sizeof(TCHAR));
    if (lfirst) mir_free_and_nill(lfirst);
    if (len) buf[(l1+l2+1)/sizeof(TCHAR)]=(TCHAR)'\0';
    return buf;
}

TCHAR* SkinEngine_ReplaceVar(TCHAR *var)
{
    if (!var) return mir_tstrdup(_T(""));
    if (!lstrcmpi(var,TEXT("Profile")))
    {
        char buf[MAX_PATH]={0};
        CallService(MS_DB_GETPROFILENAME,(WPARAM)MAX_PATH,(LPARAM)buf);
        {
            int i=strlen(buf);
            while (buf[i]!='.' && i>0) i--;
            buf[i]='\0';
        }
        mir_free_and_nill(var);
#ifdef UNICODE
        return a2u(buf);
#else
        return mir_strdup(buf);
#endif
    } 

    mir_free_and_nill(var);
    return mir_tstrdup(_T(""));
}
TCHAR *SkinEngine_ParseText(TCHAR *stzText)
{
    int len=lstrlen(stzText);
    TCHAR *result=NULL;
    int stpos=0;
    int curpos=0;  

    while(curpos<len)
    {
        //1 find first %
        while(curpos<len && stzText[curpos]!=(TCHAR)'%') curpos++;
        if (curpos<len) //% found
        {
            if (curpos-stpos>0) 
                result=SkinEngine_ReAppend(result,stzText+stpos,curpos-stpos);
            stpos=curpos+1;
            curpos++;
            //3 find second % 
            while(curpos<len && stzText[curpos]!=(TCHAR)'%') curpos++;
            if (curpos<len)
            {
                if (curpos-stpos>0) 
                {
                    TCHAR *var=mir_alloc((curpos-stpos+1)*sizeof(TCHAR));
                    memmove(var,stzText+stpos,(curpos-stpos)*sizeof(TCHAR));
                    var[curpos-stpos]=(TCHAR)'\0';
                    var=SkinEngine_ReplaceVar(var);
                    result=SkinEngine_ReAppend(result,var,0);
                    mir_free_and_nill(var);
                }
                else
                    result=SkinEngine_ReAppend(result,_T("%"),0);
                curpos++;
                stpos=curpos;
            }
            else
            {
                //  if (curpos-stpos>0) 
                //    result=SkinEngine_ReAppend(result,stzText+stpos,curpos-stpos);
                break;
            }
        }
        else
        {
            if (curpos-stpos>0) 
                result=SkinEngine_ReAppend(result,stzText+stpos,curpos-stpos);
            break;
        }   
    }
    return result;
}
/*
*   Parse text object string, find glyph object and add text to it.
*   szGlyphTextID and Define string is:
*   t[szGlyphTextID]=s[HostObjectID],[Left],[Top],[Right],[Bottom],[LTRBHV],[FontID],[Color1],[reservedforColor2],[Text]
*/
static void SkinEngine_AddParseTextGlyphObject(char * szGlyphTextID,char * szDefineString,SKINOBJECTSLIST *Skin)
{

    GLYPHOBJECT *globj=NULL;
    {     
        char buf[255]={0};
        GetParamN(szDefineString,buf,sizeof(buf),0,',',TRUE);
        if (strlen(buf))
        {
            SKINOBJECTDESCRIPTOR * lpobj;
            lpobj=SkinEngine_FindObjectByName(buf,OT_GLYPHOBJECT,Skin);
            if (lpobj)
                globj=(GLYPHOBJECT*)lpobj->Data;
        }
        if (globj)
        {
            GLYPHTEXT * glText;

            if (!globj->plTextList)
            {
                globj->plTextList=li.List_Create(0,1);
                globj->plTextList->sortFunc=SkinEngine_SortTextGlyphObjectFunc;
            }
            glText=(GLYPHTEXT*)mir_alloc(sizeof(GLYPHTEXT));
            memset(glText,0,sizeof(GLYPHTEXT));
            glText->szGlyphTextID=mir_strdup(szGlyphTextID);

            glText->iLeft=atoi(GetParamN(szDefineString,buf,sizeof(buf),1,',',TRUE));
            glText->iTop=atoi(GetParamN(szDefineString,buf,sizeof(buf),2,',',TRUE));
            glText->iRight=atoi(GetParamN(szDefineString,buf,sizeof(buf),3,',',TRUE));
            glText->iBottom=atoi(GetParamN(szDefineString,buf,sizeof(buf),4,',',TRUE));
            {
                memset(buf,0,6);
                GetParamN(szDefineString,buf,sizeof(buf),5,',',TRUE);
                buf[0]&=95; buf[1]&=95; buf[2]&=95; buf[3]&=95; buf[4]&=95; buf[5]&=95;   //to uppercase: &01011111 (0-95)
                glText->RelativeFlags=
                    (buf[0]=='C'?1:((buf[0]=='R')?2:0))       //[BC][RC][BC][RC] --- Left relative
                    |(buf[1]=='C'?4:((buf[1]=='B')?8:0))       //  |   |   |--------- Top relative   
                    |(buf[2]=='C'?16:((buf[2]=='R')?32:0))     //  |   |--------------Right relative 
                    |(buf[3]=='C'?64:((buf[3]=='B')?128:0));   //  |------------------Bottom relative          
                glText->dwFlags=(buf[4]=='C'?DT_CENTER:((buf[4]=='R')?DT_RIGHT:DT_LEFT))
                    |(buf[5]=='C'?DT_VCENTER:((buf[5]=='B')?DT_BOTTOM:DT_TOP));
            }
            glText->szFontID=mir_strdup(GetParamN(szDefineString,buf,sizeof(buf),6,',',TRUE));

            glText->dwColor=SkinEngine_HexToARGB(GetParamN(szDefineString,buf,sizeof(buf),7,',',TRUE));
            glText->dwShadow=SkinEngine_HexToARGB(GetParamN(szDefineString,buf,sizeof(buf),8,',',TRUE));
#ifdef _UNICODE
            glText->stValueText=a2u(GetParamN(szDefineString,buf,sizeof(buf),9,',',TRUE));
            glText->stText=SkinEngine_ParseText(glText->stValueText);
#else
            glText->stValueText=mir_strdup(GetParamN(szDefineString,buf,sizeof(buf),9,',',TRUE));
            glText->stText=SkinEngine_ParseText(glText->stValueText);
#endif
            li.List_Insert(globj->plTextList,(void*)glText,globj->plTextList->realCount);     
            qsort(globj->plTextList->items,globj->plTextList->realCount,sizeof(void*),(int(*)(const void*, const void*))globj->plTextList->sortFunc);
        }
    }
}


/*
*   Parse font definition string.
*   szGlyphTextID and Define string is:
*   f[szFontID]=s[FontTypefaceName],[size],[BIU]
*/
static void SkinEngine_AddParseSkinFont(char * szFontID,char * szDefineString,SKINOBJECTSLIST *Skin)
{
    //SortedList * gl_plSkinFonts=NULL;
    SKINFONT * sf =NULL;
    sf=(SKINFONT*)mir_alloc(sizeof(SKINFONT));
    if (sf)
    {
        memset(sf,0,sizeof(SKINFONT));
        {
            char buf[255];    
            int fntSize=0;
            BOOL fntBold=FALSE, fntItalic=FALSE, fntUnderline=FALSE;
            LOGFONTA logfont={0};    
            logfont.lfCharSet=DEFAULT_CHARSET;
            logfont.lfOutPrecision=OUT_DEFAULT_PRECIS;
            logfont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
            logfont.lfQuality=DEFAULT_QUALITY;
            logfont.lfPitchAndFamily=DEFAULT_PITCH|FF_DONTCARE;    

            strncpy(logfont.lfFaceName,GetParamN(szDefineString,buf,sizeof(buf),0,',',TRUE),32);
            logfont.lfHeight=atoi(GetParamN(szDefineString,buf,sizeof(buf),1,',',TRUE));
            if (logfont.lfHeight<0)
            {
                HDC hdc=CreateCompatibleDC(NULL);        
                logfont.lfHeight=(long)-MulDiv(logfont.lfHeight, GetDeviceCaps(hdc, LOGPIXELSY), 72);
                mod_DeleteDC(hdc);
            }
            logfont.lfHeight=-logfont.lfHeight;
            GetParamN(szDefineString,buf,sizeof(buf),2,',',TRUE);
            buf[0]&=95; buf[1]&=95; buf[2]&=95;
            logfont.lfWeight=(buf[0]=='B')?FW_BOLD:FW_NORMAL;
            logfont.lfItalic=(buf[1]=='I')?1:0;
            logfont.lfUnderline=(buf[2]=='U')?1:0;

            sf->hFont=CreateFontIndirectA(&logfont);
            if (sf->hFont)
            {
                sf->szFontID=mir_strdup(szFontID);
                if (!gl_plSkinFonts)
                    gl_plSkinFonts=li.List_Create(0,1);
                if (gl_plSkinFonts)
                {
                    li.List_Insert(gl_plSkinFonts,(void*)sf,gl_plSkinFonts->realCount);
                }
            }

        }
    }

}

/*
HICON SkinEngine_CreateJoinedIcon_Old(HICON hBottom, HICON hTop,BYTE alpha)
{
HDC tempDC;
HICON res=NULL;
HBITMAP oImage,nImage;
HBITMAP nMask;
ICONINFO iNew={0};
ICONINFO iciBottom={0};
ICONINFO iciTop={0};
BITMAP bmp={0};
SIZE sz={0};
{
if (!GetIconInfo(hBottom,&iciBottom)) return NULL;
GetObject(iciBottom.hbmColor,sizeof(BITMAP),&bmp);
sz.cx=bmp.bmWidth; sz.cy=bmp.bmHeight;
if(iciBottom.hbmColor) DeleteObject(iciBottom.hbmColor);
if(iciBottom.hbmMask) DeleteObject(iciBottom.hbmMask);
}
if (sz.cx==0 || sz.cy==0) return NULL;
tempDC=CreateCompatibleDC(NULL);
nImage=SkinEngine_CreateDIB32(sz.cx,sz.cy);
oImage=SelectObject(tempDC,nImage);
SkinEngine_DrawIconEx(tempDC,0,0,hBottom,sz.cx,sz.cy,0,NULL,DI_NORMAL);
SkinEngine_DrawIconEx(tempDC,0,0,hTop,sz.cx,sz.cy,0,NULL,DI_NORMAL|(alpha<<24));
SelectObject(tempDC,oImage);
DeleteDC(tempDC);
{
BYTE * p=malloc(sz.cx*sz.cy/8+10);
nMask=CreateBitmap(sz.cx,sz.cy,1,1,(void*)p);
iNew.fIcon=TRUE;
iNew.hbmColor=nImage;
iNew.hbmMask=nMask;
res=CreateIconIndirect(&iNew);
if (!res) 
TRACE_ERROR();
DeleteObject(nImage);
DeleteObject(nMask);
free(p);
}
return res;
}
*/


/*
*   SkinEngine_CheckHasAlfaChannel - checks if image has at least one BYTE in alpha chennel
*                  that is not a 0. (is image real 32 bit or just 24 bit)
*/
static BOOL SkinEngine_CheckHasAlfaChannel(BYTE * from, int widthByte, int height)
{
    int i=0,j=0;
    DWORD * pt=(DWORD*)from;
    while (j<height)
    {
        BYTE * add=(BYTE*)pt+widthByte;
        while (pt<(DWORD*)add)
        {
            if ((*pt&0xFF000000)!=0) return TRUE;
            pt++;
        }
        pt=(DWORD*)(from+widthByte*j);
        j++;
    }
    return FALSE;
}

/*
*   SkinEngine_CheckIconHasMask - checks if mask image has at least one that is not a 0.
*                  Not sure is ir required or not
*/
static BOOL SkinEngine_CheckIconHasMask(BYTE * from)
{
    int i=0;
    for (i=0; i<16*16/8; i++)
    {
        if (from[i]!=0) return TRUE;
    }
    return FALSE;
}

/*
*   SkinEngine_GetMaskBit - return value of apropriate mask bit in line at x position
*/
static BOOL SkinEngine_GetMaskBit(BYTE *line, int x)
{
    return ((*(line+(x>>3)))&(0x01<<(7-(x&0x07))))!=0;
}
/*
*    SkinEngine_Blend  - alpha SkinEngine_Blend ARGB values of 2 pixels. X1 - underlaying,
*            X2 - overlaying points.
*/

static DWORD SkinEngine_Blend(DWORD X1,DWORD X2, BYTE alpha)
{
    BYTE a1=(BYTE)(X1>>24);
    BYTE a2=(BYTE)(((X2>>24)*alpha)>>8);
    BYTE r1=(BYTE)(X1>>16);
    BYTE r2=(BYTE)(X2>>16);
    BYTE g1=(BYTE)(X1>>8);
    BYTE g2=(BYTE)(X2>>8);
    BYTE b1=(BYTE)(X1);
    BYTE b2=(BYTE)(X2);

    BYTE a_1=~a1;
    BYTE a_2=~a2;
	WORD am=(WORD)a1*a_2;

    /*  it is possible to use >>8 instead of /255 but it is require additional
    *   checking of alphavalues
    */
    WORD ar=a1+(((WORD)a_1*a2)/255);
    // if a2 more than 0 than result should be more
    // or equal (if a1==0) to a2, else in combination
    // with mask we can get here black points

    ar=(a2>ar)?a2:ar;

    if (ar==0) return 0;

    //else
    {
		WORD arm=ar*255;
		WORD rr=(((WORD)r1*am+(WORD)r2*a2*255))/arm;
		WORD gr=(((WORD)g1*am+(WORD)g2*a2*255))/arm;
		WORD br=(((WORD)b1*am+(WORD)b2*a2*255))/arm;
		return (ar<<24)|(rr<<16)|(gr<<8)|br;
    }

}
/*
*    CreateJoinedIcon  - creates new icon by drawing hTop over hBottom.
*/
HICON SkinEngine_CreateJoinedIcon(HICON hBottom, HICON hTop, BYTE alpha)
{
    HDC tempDC;
    HICON res=NULL;
    HBITMAP oImage,nImage;
    HBITMAP nMask;
    BITMAP bmp={0};
    BYTE *ptPixels;
    ICONINFO iNew={0};
    ICONINFO iciBottom={0};
    ICONINFO iciTop={0};

    BITMAP bmp_top={0};
    BITMAP bmp_top_mask={0};

    BITMAP bmp_bottom={0};
    BITMAP bmp_bottom_mask={0};

    tempDC=CreateCompatibleDC(NULL);
    nImage=SkinEngine_CreateDIB32Point(16,16,(void**)&ptPixels);
    oImage=SelectObject(tempDC,nImage);

    GetIconInfo(hBottom,&iciBottom);
    GetObject(iciBottom.hbmColor,sizeof(BITMAP),&bmp_bottom);
    GetObject(iciBottom.hbmMask,sizeof(BITMAP),&bmp_bottom_mask);

    GetIconInfo(hTop,&iciTop);
    GetObject(iciTop.hbmColor,sizeof(BITMAP),&bmp_top);
    GetObject(iciTop.hbmMask,sizeof(BITMAP),&bmp_top_mask);

    if (bmp_bottom.bmBitsPixel==32 &&bmp_top.bmBitsPixel==32 && IsWinVerXPPlus())
    {
        BYTE * BottomBuffer, * TopBuffer, * BottomMaskBuffer, * TopMaskBuffer;
        BYTE * bb, * tb, * bmb, * tmb;
        BYTE * db=ptPixels;
        int vstep_d=16*4;
        int vstep_b=bmp_bottom.bmWidthBytes;
        int vstep_t=bmp_top.bmWidthBytes;
        int vstep_bm=bmp_bottom_mask.bmWidthBytes;
        int vstep_tm=bmp_top_mask.bmWidthBytes;
        alpha=alpha?alpha:255;
        if (bmp_bottom.bmBits) bb=BottomBuffer=(BYTE*)bmp_bottom.bmBits;
        else
        {
            BottomBuffer=(BYTE*)malloc(bmp_bottom.bmHeight*bmp_bottom.bmWidthBytes);
            GetBitmapBits(iciBottom.hbmColor,bmp_bottom.bmHeight*bmp_bottom.bmWidthBytes,BottomBuffer);
            bb=BottomBuffer+vstep_b*(bmp_bottom.bmHeight-1);
            vstep_b=-vstep_b;
        }
        if (bmp_top.bmBits) tb=TopBuffer=(BYTE*)bmp_top.bmBits;
        else
        {
            TopBuffer=(BYTE*)malloc(bmp_top.bmHeight*bmp_top.bmWidthBytes);
            GetBitmapBits(iciTop.hbmColor,bmp_top.bmHeight*bmp_top.bmWidthBytes,TopBuffer);
            tb=TopBuffer+vstep_t*(bmp_top.bmHeight-1);
            vstep_t=-vstep_t;
        }
        if (bmp_bottom_mask.bmBits)
        {
            BottomMaskBuffer=(BYTE*)bmp_bottom_mask.bmBits;
            bmb=BottomMaskBuffer;         
        }
        else
        {
            BottomMaskBuffer=(BYTE*)malloc(bmp_bottom_mask.bmHeight*bmp_bottom_mask.bmWidthBytes);            
			GetBitmapBits(iciBottom.hbmMask,bmp_bottom_mask.bmHeight*bmp_bottom_mask.bmWidthBytes,BottomMaskBuffer);
			bmb=BottomMaskBuffer+vstep_bm*(bmp_bottom_mask.bmHeight-1);
			vstep_bm=-vstep_bm;

        }
        if (bmp_top_mask.bmBits)
        {
            TopMaskBuffer=(BYTE*)bmp_top_mask.bmBits;
            tmb=TopMaskBuffer;
            
        }
        else
        {
            TopMaskBuffer=(BYTE*)malloc(bmp_top_mask.bmHeight*bmp_top_mask.bmWidthBytes);
            GetBitmapBits(iciTop.hbmMask,bmp_top_mask.bmHeight*bmp_top_mask.bmWidthBytes,TopMaskBuffer);
			tmb=TopMaskBuffer+vstep_tm*(bmp_top_mask.bmHeight-1);
			vstep_tm=-vstep_tm;
        }
        {
            int x=0; int y=0;
            BOOL topHasAlpha=SkinEngine_CheckHasAlfaChannel(TopBuffer,bmp_top.bmWidthBytes,bmp_top.bmHeight);
            BOOL bottomHasAlpha=SkinEngine_CheckHasAlfaChannel(BottomBuffer,bmp_bottom.bmWidthBytes,bmp_bottom.bmHeight);
            BOOL topHasMask=SkinEngine_CheckIconHasMask(TopMaskBuffer);
            BOOL bottomHasMask=SkinEngine_CheckIconHasMask(BottomMaskBuffer);
            for (y=0; y<16; y++)
            {
                for (x=0; x<16; x++)
                {
                    BOOL mask_b=SkinEngine_GetMaskBit(bmb,x);
                    BOOL mask_t=SkinEngine_GetMaskBit(tmb,x);
                    DWORD bottom_d=((DWORD*)bb)[x];
                    DWORD top_d=((DWORD*)tb)[x];
                    if (topHasMask)
                    {
                        if (mask_t==1 && !topHasAlpha )  top_d&=0xFFFFFF;
                        else if (!topHasAlpha) top_d|=0xFF000000;
                    }
                    if (bottomHasMask)
                    {
                        if (mask_b==1 && !bottomHasAlpha) bottom_d&=0xFFFFFF;
                        else if (!bottomHasAlpha) bottom_d|=0xFF000000;
                    }
                    ((DWORD*)db)[x]=SkinEngine_Blend(bottom_d,top_d,alpha);
                }
                bb+=vstep_b;
                tb+=vstep_t;
                bmb+=vstep_bm;
                tmb+=vstep_tm;
                db+=vstep_d;
            }
        }
        if (!bmp_bottom.bmBits) free(BottomBuffer);
        if (!bmp_top.bmBits) free(TopBuffer);
        if (!bmp_bottom_mask.bmBits) free(BottomMaskBuffer);
        if (!bmp_top_mask.bmBits) free(TopMaskBuffer);
    }
    else
    {
        SkinEngine_DrawIconEx(tempDC,0,0,hBottom,16,16,0,NULL,DI_NORMAL);
        SkinEngine_DrawIconEx(tempDC,0,0,hTop,16,16,0,NULL,DI_NORMAL|(alpha<<24));
    }
    DeleteObject(iciBottom.hbmColor);
    DeleteObject(iciTop.hbmColor);
    DeleteObject(iciBottom.hbmMask);
    DeleteObject(iciTop.hbmMask);

    SelectObject(tempDC,oImage);
    DeleteDC(tempDC);
    {
        //BYTE *p=malloc(32);
        //memset(p,0,32);
        BYTE p[32] = {0};
        nMask=CreateBitmap(16,16,1,1,(void*)&p);
        {
            HDC tempDC2=CreateCompatibleDC(NULL);
            HDC tempDC3=CreateCompatibleDC(NULL);
            HBITMAP hbm=CreateCompatibleBitmap(tempDC3,16,16);
            HBITMAP obmp=SelectObject(tempDC2,nMask);
            HBITMAP obmp2=SelectObject(tempDC3,hbm);
            DrawIconEx(tempDC2,0,0,hBottom,16,16,0,NULL,DI_MASK);
            DrawIconEx(tempDC3,0,0,hTop,16,16,0,NULL,DI_MASK);
            BitBlt(tempDC2,0,0,16,16,tempDC3,0,0,SRCAND);
            SelectObject(tempDC2,obmp);
            SelectObject(tempDC3,obmp2);
            DeleteObject(hbm);
            DeleteDC(tempDC2);
            DeleteDC(tempDC3);
        }
        iNew.fIcon=TRUE;
        iNew.hbmColor=nImage;
        iNew.hbmMask=nMask;
        res=CreateIconIndirect(&iNew);
        DeleteObject(nImage);
        DeleteObject(nMask);
    }
    return res;
}
