##
## Menu module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c menu.c mkmenu.c

MAKE_EXPORTS=mod_menu

MODULE=mod_menu
MODULE_STUB=mod_menu.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
