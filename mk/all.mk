#
# OS/3 (osFree) project
# common make macros.
# (c) osFree project
# valerius, 2006/10/30
#

ROOT=$(%ROOT)

#
# Preprocessor defines
#
C_DEFS    = -zq -d__OS2__ -d__WATCOM__
ASM_DEFS  = -d__WATCOM__

#
# ADD_COPT and ADD_ASMOPT are defined in
# a file which includes this file.
#
!ifdef 32_BITS
COPT      = $(C_DEFS) -i=. &
                      -i=.. &
                      -i=$(ROOT)$(SEP)include$(SEP)os3 &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)os2 &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)pm &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)GDlib &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)zlib &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)lpng &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)jpeglib &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)libtiff &
                      -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)gbm &
                      $(ADD_COPT)
ASMOPT    = $(ASM_DEFS)  $(ADD_ASMOPT) -bt=OS2
!else
COPT      = -ms $(C_DEFS) -i=$(ROOT)$(SEP)include$(SEP)os3 -i=. -i=.. -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)pm -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)GDlib -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)zlib -i=$(ROOT)$(SEP)include$(SEP)os3$(SEP)gbm $(ADD_COPT)
ASMOPT    = -bt=OS2 -ms $(ASM_DEFS)  $(ADD_ASMOPT)
!endif

#
# Tools:
#
!ifdef 32_BITS
CC        = @wcc386
CPPC      = @wpp386
!else
CC        = @wcc
CPPC      = @wpp
!endif

ASM       = @wasm

LINKER    = @wlink
LINKOPT   = op q $(ADD_LINKOPT)

LIB       = @wlib
LIBOPT    = -q

# Don't add @ sign here. Will break build system
MAKE      = wmake
MAKEOPT   = -h

PC        = ppc386
PCOPT     = -Sg2h

DD        = dd

SED       = sed
AWK       = @awk
DOX       = doxygen

RC        = @wrc -q
RCOPT = -bt=OS2

MC        = mkmsgf

HC        = ipfc

GENE2FS   = genext2fs

SYS       = sys


# A command to add a list of object
# files to a linker script
ADDFILES_CMD = @for %%i in ($(OBJS)) do @%append $^&.lnk FILE %%i

#
# Extensions to clean up
#
CLEANMASK = *.lnk *.map *.obj *.err *.log *.bak *.lib *.com *.sym *.bin *.exe *.dll *.wmp

!ifeq UNIX FALSE                 # Non-unix

!ifeq ENV OS/2
COMSPEC   = $(OS_SHELL)          # Shell
OS2_SHELL = $(OS_SHELL)          #
RN  = @move                      # Rename command
!else ifeq ENV Windows
RN  = @ren                       # Rename command
!endif

SEP       = \                  # dir components separator
PS        = ;                  # paths separator
O         = obj                  # Object Extension differs from Linux to OS/2
DC        = @del                 # Delete command is rm on linux and del on OS/2
CP        = @copy                # Copy command
SAY       = @echo                # Echo message
MKBIN     = $(REXX) mkbin.cmd
GENHDD    = $(REXX) genhdd.cmd
GENFDD    = $(REXX) genfdd.cmd
FINDFILE  = $(REXX) findfile.cmd
if_not_exist_mkdir = if_not_exist_mkdir.cmd

!ifeq ENV Windows
NULL      = nul
!else
NULL      = \dev\nul
!endif
BLACKHOLE = 2>&1 >$(NULL)

CLEAN_CMD    = @for %%i in ($(CLEANMASK)) do $(DC) %%i $(BLACKHOLE)

!else ifeq UNIX TRUE             # UNIX

SEP       = /                  # dir components separator
PS        = :                  # paths separator
O         = obj                  # Object Extension differs from Linux to OS/2
DC        = rm -f                # Delete command is rm on linux and del on OS/2
CP        = cp                   # Copy command
RN        = mv                   # Rename command
SAY       = echo                 # Echo message
MKBIN     = mkbin
GENHDD    = genhdd
GENFDD    = genfdd
FINDFILE  = findfile

if_not_exist_mkdir = ./if_not_exist_mkdir.sh
NULL      = /dev/null
BLACKHOLE = 2>&1 >$(NULL)

CLEAN_CMD    = $(DC) $(CLEANMASK) $(BLACKHOLE)

!endif

MKDIR     = @mkdir
MAPSYM    = @mapsym

TOOLS     = $(ROOT)$(SEP)tools$(SEP)bin
LOG       =  # 2>&1 >> $(ROOT)$(SEP)compile.log


.SUFFIXES:
.SUFFIXES:  .sym .exe .dll .lib .$(O) .res .inf .c .cpp .asm .h .hpp .inc .rc .pas .ipf .map .wmp .rexx

.c.$(O): .AUTODEPEND
 $(SAY) Compiling $< $(LOG)
 $(CC) $(COPT) -fo=$^&.$(O) $< $(LOG)

.cpp.$(O): .AUTODEPEND
 $(SAY) Compiling $< $(LOG)
 $(CPPC) $(COPT) -fo=$^&.$(O) $< $(LOG)

.asm.$(O): .AUTODEPEND
 $(SAY) Assembling $< $(LOG)
 $(ASM) $(ASMOPT) -fo=$^&.$(O) $< $(LOG)

.wmp.map: .AUTODEPEND
 $(SAY) Converting Watcom MAP to VAC MAP $< $(LOG)
 $(AWK) -f $(TOOLS)$(SEP)mapsym.awk <$^&.wmp >$^&.map

.map.sym: .AUTODEPEND
 $(SAY) Converting VAC MAP to OS/2 SYM $< $(LOG)
 $(MAPSYM) $^&.map

.ipf.inf: .symbolic
  $(SAY) Compiling $<
  $(HC) -i $< $@

.rc.res: .AUTODEPEND
  $(RC) $(RCOPT) $<

.pas.exe: .AUTODEPEND
  $(SAY) Compiling $<
  $(PC) $(PCOPT) $<
  $(CP) $^. $^:
  $(DC) $^.

.rexx.exe: .AUTODEPEND
  $(SAY) Wrapping REXX code $<
  rexxwrapper -program=$^& -rexxfiles=$^&.rexx -srcdir=$(%ROOT)$(SEP)tools$(SEP)rexxwrap -compiler=wcc -interpreter=os2rexx -intlib=rexx.lib -intincdir=$(%WATCOM)$(SEP)h$(SEP)os2 -compress

#
# "$(MAKE) subdirs" enters each dir in $(DIRS)
# and does $(MAKE) $(TARGET) in each dir:
#
subdirs: .SYMBOLIC
 @for %%i in ($(DIRS)) do @cd %%i && cd && $(MAKE) $(MAKEOPT) $(TARGET) && cd ..

.ERROR
 @echo Error
 @exit
