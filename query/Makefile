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

SOURCES=complete.c complete_file.c edln.c input.c listing.c main.c \
	wedln.c wedln-wrappers.c wmessage.c query.c fwarn.c

MAKE_EXPORTS=query_module

MODULE=query

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
