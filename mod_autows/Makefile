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

SOURCES=main.c autows.c placement.c splitext.c

MAKE_EXPORTS=mod_autows

MODULE=mod_autows
MODULE_STUB=mod_autows.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
