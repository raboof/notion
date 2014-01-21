##
## Menu module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c menu.c mkmenu.c grabmenu.c

MAKE_EXPORTS=mod_menu

MODULE=mod_menu
MODULE_STUB=mod_menu.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
