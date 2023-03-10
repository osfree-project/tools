#
# A Makefile for OS/3 build tools
# (c) osFree project,
# valerius, 2006/10/30
#

# Notes:
# 1. UniAPI must come first here because used to produce API headers

# Note II: Do not list 'scripts' dir here, in this case you'll encounter the dead loop
DIRS = sed uniapi UNI yacc lex jwasm awk &
       mkmsgf mkctxt critstrs freeinst genext2fs winrc &
       shared qemu-img hlldump mapsym renmodul &
# rexxwrap ltools lxlite & 
       target
PLATFORM = host$(SEP)$(%HOST)$(SEP)

!include $(%ROOT)tools/mk/all.mk
