##
## ion-devel-dock module Makefile
##
## $Header: /home/twp/cvsroot/twp/ion/ion-devel-dock/Makefile,v 1.11 2003/12/02 18:40:33 twp Exp $
##

# System specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

# If you do not want to use the Shape extension then comment out the following.
DEFINES += -DCF_HAVE_SHAPE_EXTENSION

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=dock.c
ETC=dock.lua dock-draw.lua
DOCS=LICENSE README
PATCHES=

MAKE_EXPORTS=dock_module

MODULE=dock

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done

######################################

tarball:
	sh -c 'DIR=ion-devel-$(MODULE)-`date +%Y%m%d`; \
		mkdir $$DIR; \
		cp $(SOURCES) $(ETC) $(DOCS) $(PATCHES) Makefile $$DIR; \
		tar -cvjf $$DIR.tar.bz2 $$DIR/*; \
		rm -Rf $$DIR'
