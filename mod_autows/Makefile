##
## mod_autows workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c autows.c placement.c

LUA_SOURCES=templates.lua

MAKE_EXPORTS=mod_autows

MODULE=mod_autows

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(SHAREDIR)
	$(INSTALL) -m $(DATA_MODE) templates.lua $(SHAREDIR)
	$(INSTALLDIR) $(LCDIR)
	$(INSTALL) -m $(DATA_MODE) templates.lc $(LCDIR)
