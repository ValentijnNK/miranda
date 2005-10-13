# Microsoft Developer Studio Generated NMAKE File, Based on clist.dsp
!IF "$(CFG)" == ""
CFG=clist - Win32 Release Unicode
!MESSAGE No configuration specified. Defaulting to clist - Win32 Release Unicode.
!ENDIF

!IF "$(CFG)" != "clist - Win32 Release" && "$(CFG)" != "clist - Win32 Debug" && "$(CFG)" != "clist - Win32 Release Unicode" && "$(CFG)" != "clist - Win32 Debug Unicode"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "clist.mak" CFG="clist - Win32 Debug Unicode"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "clist - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist - Win32 Release Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "clist - Win32 Debug Unicode" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "clist - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release

ALL : "..\..\bin\release\plugins\clist_classic.dll"


CLEAN :
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\clist_classic.exp"
	-@erase "$(OUTDIR)\clist_classic.map"
	-@erase "..\..\bin\release\plugins\clist_classic.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "NDEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib /nologo /base:"0x6590000" /dll /incremental:no /pdb:"$(OUTDIR)\clist_classic.pdb" /map:"$(INTDIR)\clist_classic.map" /machine:I386 /out:"../../bin/release/plugins/clist_classic.dll" /implib:"$(OUTDIR)\clist_classic.lib"
LINK32_OBJS= \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\utf.obj"

"..\..\bin\release\plugins\clist_classic.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "..\..\bin\debug\plugins\clist_classic.dll"


CLEAN :
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\clist_classic.exp"
	-@erase "$(OUTDIR)\clist_classic.lib"
	-@erase "$(OUTDIR)\clist_classic.pdb"
	-@erase "..\..\bin\debug\plugins\clist_classic.dll"
	-@erase "..\..\bin\debug\plugins\clist_classic.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib /nologo /base:"0x6590000" /dll /incremental:yes /pdb:"$(OUTDIR)\clist_classic.pdb" /debug /machine:I386 /out:"../../bin/debug/plugins/clist_classic.dll" /implib:"$(OUTDIR)\clist_classic.lib" /pdbtype:sept
LINK32_OBJS= \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\utf.obj"

"..\..\bin\debug\plugins\clist_classic.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

OUTDIR=.\Release_Unicode
INTDIR=.\Release_Unicode

ALL : "..\..\bin\Release Unicode\plugins\clist_classic.dll"


CLEAN :
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clist.pch"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\clist_classic.exp"
	-@erase "$(OUTDIR)\clist_classic.map"
	-@erase "..\..\bin\Release Unicode\plugins\clist_classic.dll"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "NDEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib /nologo /base:"0x6590000" /dll /incremental:no /pdb:"$(OUTDIR)\clist_classic.pdb" /map:"$(INTDIR)\clist_classic.map" /machine:I386 /out:"../../bin/Release Unicode/plugins/clist_classic.dll" /implib:"$(OUTDIR)\clist_classic.lib"
LINK32_OBJS= \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\utf.obj"

"..\..\bin\Release Unicode\plugins\clist_classic.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

OUTDIR=.\Debug_Unicode
INTDIR=.\Debug_Unicode

ALL : "..\..\bin\Debug Unicode\plugins\clist_classic.dll"


CLEAN :
	-@erase "$(INTDIR)\clc.obj"
	-@erase "$(INTDIR)\clcfiledrop.obj"
	-@erase "$(INTDIR)\clcidents.obj"
	-@erase "$(INTDIR)\clcitems.obj"
	-@erase "$(INTDIR)\clcmsgs.obj"
	-@erase "$(INTDIR)\clcopts.obj"
	-@erase "$(INTDIR)\clcpaint.obj"
	-@erase "$(INTDIR)\clcutils.obj"
	-@erase "$(INTDIR)\clistevents.obj"
	-@erase "$(INTDIR)\clistmenus.obj"
	-@erase "$(INTDIR)\clistmod.obj"
	-@erase "$(INTDIR)\clistopts.obj"
	-@erase "$(INTDIR)\clistsettings.obj"
	-@erase "$(INTDIR)\clisttray.obj"
	-@erase "$(INTDIR)\clui.obj"
	-@erase "$(INTDIR)\cluiopts.obj"
	-@erase "$(INTDIR)\cluiservices.obj"
	-@erase "$(INTDIR)\commonheaders.obj"
	-@erase "$(INTDIR)\contact.obj"
	-@erase "$(INTDIR)\Docking.obj"
	-@erase "$(INTDIR)\forkthread.obj"
	-@erase "$(INTDIR)\groups.obj"
	-@erase "$(INTDIR)\init.obj"
	-@erase "$(INTDIR)\keyboard.obj"
	-@erase "$(INTDIR)\resource.res"
	-@erase "$(INTDIR)\utf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\clist_classic.exp"
	-@erase "$(OUTDIR)\clist_classic.lib"
	-@erase "$(OUTDIR)\clist_classic.pdb"
	-@erase "..\..\bin\Debug Unicode\plugins\clist_classic.dll"
	-@erase "..\..\bin\Debug Unicode\plugins\clist_classic.ilk"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32
RSC_PROJ=/l 0x809 /fo"$(INTDIR)\resource.res" /i "../../include/" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\clist.bsc"
BSC32_SBRS= \

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib comdlg32.lib /nologo /base:"0x6590000" /dll /incremental:yes /pdb:"$(OUTDIR)\clist_classic.pdb" /debug /machine:I386 /out:"../../bin/Debug Unicode/plugins/clist_classic.dll" /implib:"$(OUTDIR)\clist_classic.lib" /pdbtype:sept
LINK32_OBJS= \
	"$(INTDIR)\clc.obj" \
	"$(INTDIR)\clcfiledrop.obj" \
	"$(INTDIR)\clcidents.obj" \
	"$(INTDIR)\clcitems.obj" \
	"$(INTDIR)\clcmsgs.obj" \
	"$(INTDIR)\clcopts.obj" \
	"$(INTDIR)\clcpaint.obj" \
	"$(INTDIR)\clcutils.obj" \
	"$(INTDIR)\clistevents.obj" \
	"$(INTDIR)\clistmenus.obj" \
	"$(INTDIR)\clistmod.obj" \
	"$(INTDIR)\clistopts.obj" \
	"$(INTDIR)\clistsettings.obj" \
	"$(INTDIR)\clisttray.obj" \
	"$(INTDIR)\clui.obj" \
	"$(INTDIR)\cluiopts.obj" \
	"$(INTDIR)\cluiservices.obj" \
	"$(INTDIR)\commonheaders.obj" \
	"$(INTDIR)\contact.obj" \
	"$(INTDIR)\Docking.obj" \
	"$(INTDIR)\forkthread.obj" \
	"$(INTDIR)\groups.obj" \
	"$(INTDIR)\init.obj" \
	"$(INTDIR)\keyboard.obj" \
	"$(INTDIR)\resource.res" \
	"$(INTDIR)\utf.obj"

"..\..\bin\Debug Unicode\plugins\clist_classic.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $<
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("clist.dep")
!INCLUDE "clist.dep"
!ELSE
!MESSAGE Warning: cannot find "clist.dep"
!ENDIF
!ENDIF


!IF "$(CFG)" == "clist - Win32 Release" || "$(CFG)" == "clist - Win32 Debug" || "$(CFG)" == "clist - Win32 Release Unicode" || "$(CFG)" == "clist - Win32 Debug Unicode"
SOURCE=.\clc.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcfiledrop.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcfiledrop.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcidents.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcidents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcitems.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcitems.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcmsgs.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcmsgs.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcopts.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcpaint.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcpaint.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clcutils.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clcutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistevents.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistevents.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistmenus.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmenus.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistmod.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistmod.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistopts.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clistsettings.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clistsettings.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clisttray.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clisttray.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\clui.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\clui.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\cluiopts.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiopts.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\cluiservices.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\cluiservices.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\commonheaders.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\commonheaders.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yc"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\commonheaders.obj"	"$(INTDIR)\clist.pch" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\commonheaders.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\contact.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\contact.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\Docking.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\Docking.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\forkthread.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\forkthread.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\groups.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\groups.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\init.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\init.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=.\keyboard.c

!IF  "$(CFG)" == "clist - Win32 Release"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Release Unicode"

CPP_SWITCHES=/nologo /MD /W3 /O1 /I "../../include/" /I "../../include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /Yu"commonheaders.h" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c

"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ELSEIF  "$(CFG)" == "clist - Win32 Debug Unicode"

CPP_SWITCHES=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "CLIST_EXPORTS" /D "UNICODE" /Fp"$(INTDIR)\clist.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c

"$(INTDIR)\keyboard.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) @<<
  $(CPP_SWITCHES) $(SOURCE)
<<


!ENDIF

SOURCE=..\db3x\utf.c

"$(INTDIR)\utf.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\clist.pch"
	$(CPP) $(CPP_PROJ) $(SOURCE)


SOURCE=.\resource.rc

"$(INTDIR)\resource.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF
