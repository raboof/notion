##
## Query module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

SUBDIRS=ion-completefile
INSTALL_SUBDIRS=ion-completefile

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE)

SOURCES=complete.c edln.c input.c listing.c main.c wedln.c \
        wedln-wrappers.c wmessage.c query.c fwarn.c

LUA_SOURCES=querylib.lua

MAKE_EXPORTS=query_module

MODULE=query

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(SHAREDIR)
	$(INSTALL) -m $(DATA_MODE) querylib.lua querylib.lc $(SHAREDIR)
