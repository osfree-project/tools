#
# OS/3 (osFree) boot sequence project
# common make macros.
# (c) osFree project
# valerius, 2006/10/30
#

!ifndef __bootseq_mk__
__bootseq_mk__ = 1

ADD_COPT   = $(ADD_COPT)   -bt=OS2
ADD_ASMOPT = $(ADD_ASMOPT) -bt=OS2

!include $(%ROOT)/mk/all.mk

RIP          = $(REXX) ripzeroes.cmd
GENREL       = $(REXX) genrel.cmd

#
# ...
#

!endif
