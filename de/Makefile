##
## Default drawing engine module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE)

SOURCES=init.c draw.c font.c colour.c brush.c

MODULE=de

MAKE_EXPORTS=de_module

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(SHAREDIR)
	$(INSTALL) -m $(DATA_MODE) delib.lua $(SHAREDIR)
