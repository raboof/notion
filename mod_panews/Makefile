##
## mod_panews workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c panews.c placement.c splitext.c \
	panehandle.c unusedwin.c

MAKE_EXPORTS=mod_panews

MODULE=mod_panews
MODULE_STUB=mod_panews.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
