##
## mod_notionflux Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INSTALL_SUBDIRS = notionflux
SUBDIRS = $(INSTALL_SUBDIRS)

######################################

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=mod_notionflux.c
MAKE_EXPORTS=mod_notionflux

MODULE_STUB=mod_notionflux.lua
MODULE=mod_notionflux

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
