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

OBJS =	bindmaps.o conf.o ionframe.o funtabs.o main.o placement.o \
	split.o splitframe.o ionws.o resize.o

MODULE=ionws

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install



