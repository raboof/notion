##
## Ion workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
MODULEDIR = $(LIBDIR)/ion-devel
CFLAGS += $(XOPEN_SOURCE)
DEFINES += -DETCDIR=\"$(ETCDIR)\"

SOURCES=bindmaps.c conf.c ionframe.c main.c placement.c \
	split.c splitframe.c ionws.c resize.c

MAKE_EXPORTS=ionws_module

MODULE=ionws

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install



