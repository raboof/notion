##
## Menu module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c menu.c mkmenu.c

LUA_SOURCES=menulib.lua

MAKE_EXPORTS=menumod

MODULE=menumod

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(SHAREDIR)
	$(INSTALL) -m $(DATA_MODE) menulib.lua menulib.lc $(SHAREDIR)

