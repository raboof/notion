##
## frame_xkb module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I$(TOPDIR)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE) -Wall
LDFLAGS += $(X11_LIBS)

SOURCES=mod_xkbevents.c

MAKE_EXPORTS=mod_xkbevents

MODULE=mod_xkbevents

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
