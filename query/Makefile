##
## Query module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
MODULEDIR = $(LIBDIR)/ion-devel
CFLAGS += $(XOPEN_SOURCE)

OBJS= complete.o complete_file.o edln.o input.o listing.o main.o \
      wedln.o wmessage.o query.o fwarn.o

MODULE=query

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
