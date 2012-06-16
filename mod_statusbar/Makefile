##
## Notion mod_statusbar Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES = main.c statusbar.c draw.c

SUBDIRS = ion-statusd
INSTALL_SUBDIRS = $(SUBDIRS)

MAKE_EXPORTS = mod_statusbar

MODULE = mod_statusbar
MODULE_STUB = mod_statusbar.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install 

