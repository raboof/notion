##
## Ion dock module Makefile
##
## $Header: /home/twp/cvsroot/twp/ion/ion-devel-dock/Makefile,v 1.15 2003/12/21 18:08:43 twp Exp $
##

# System specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=dock.c
ETC=cfg_dock.lua dock-draw.lua
DOCS=LICENSE README.dock

MAKE_EXPORTS=mod_dock

MODULE=mod_dock

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done

######################################

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)
