##
## Notion dock module Makefile
##
##

# System specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=dock.c
DOCS=LICENSE README.dock

MAKE_EXPORTS=mod_dock

MODULE=mod_dock

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install

######################################

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)
