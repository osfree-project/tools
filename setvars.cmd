@echo off

set WATCOM=f:\dev\watcom
set ROOT=f:\var\src\os2\svn\osfree

set TKPATH=f:\os2tk45
set FPPATH=f:\dev\pp

set TOOLS=%ROOT%\tools\bin
set PATH=%WATCOM%\binp;%TOOLS%;%TKPATH%\bin;%FPPATH%\bin\os2;%PATH%
set INCLUDE=%WATCOM%\h;%WATCOM%\h\dos;%WATCOM%\h\os2
set FINCLUDE=%WATCOM%\src\fortran
set EDPATH=%WATCOM%\eddat
set HELP=%WATCOM%\binp\help;%HELP%
set BOOKSHELF=%WATCOM%\binp\help;%BOOKSHELF%
set BEGINLIBPATH=%WATCOM%\binp\dll
set LIBOS2=%WATCOM%\lib386\os2
set LIB=%WATCOM%\lib286;%WATCOM%\lib286\dos
