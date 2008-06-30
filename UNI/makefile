#
# A Makefile for mkmsgf
# (c) osFree project,
# valerius, 2006/10/30
#
# $Id: makefile,v 1.1 2004/08/16 06:27:30 prokushev Exp $
#

!include $(%ROOT)/build.conf
!include $(%ROOT)/mk/site.mk

32_BITS  = 1

!include $(%ROOT)/mk/all.mk

OUT = $(%ROOT)$(SEP)INCLUDE$(SEP)OS3

all: h .SYMBOLIC

h: workaround $(OUT)$(SEP)osfree.h os2
  @rem

$(OUT)$(SEP)osfree.h: osfree.uni
# Main osFree file
  uni2h osfree.uni $(OUT)$(SEP)osfree.h
  $(REXX) cut.cmd $(OUT)$(SEP)osfree.h
  $(DC) $(OUT)$(SEP)osfree.h
  $(CP) tmp $(OUT)$(SEP)osfree.h
  $(DC) tmp

workaround: cdeftypes.h cdeftypes2.h
# Workaround of uniplemented features of uni2h tool
  $(CP) cdeftypes.h $(OUT)$(SEP)cdeftypes.h
  $(CP) cdeftypes2.h $(OUT)$(SEP)cdeftypes2.h

## OS/2 Personality files
os2: $(OUT)$(SEP)OS2$(SEP)os2.h $(OUT)$(SEP)OS2$(SEP)os2def.h $(OUT)$(SEP)OS2$(SEP)bse.h
  @rem

$(OUT)$(SEP)OS2$(SEP)os2.h: os2$(SEP)os2.uni
  uni2h os2$(SEP)os2.uni $(OUT)$(SEP)OS2$(SEP)os2.h
  $(REXX) cut.cmd $(OUT)$(SEP)OS2$(SEP)os2.h
  $(DC) $(OUT)$(SEP)OS2$(SEP)os2.h
  $(CP) tmp $(OUT)$(SEP)OS2$(SEP)os2.h
  $(DC) tmp

### OS/2 Personality base type and macros definitions
$(OUT)$(SEP)OS2$(SEP)os2def.h: os2$(SEP)os2def.uni
  uni2h os2$(SEP)os2def.uni $(OUT)$(SEP)OS2$(SEP)os2def.h

### OS/2 Personality base API
$(OUT)$(SEP)OS2$(SEP)bse.h: os2$(SEP)bse.uni
  uni2h os2$(SEP)bse.uni $(OUT)$(SEP)OS2$(SEP)bse.h

# Clean all

.IGNORE
clean: .SYMBOLIC
 $(SAY) Making clean... $(LOG)
 $(CLEAN_CMD)