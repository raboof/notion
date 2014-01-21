##
## Scratchpad module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c
ETC=cfg_sp.lua

MAKE_EXPORTS=mod_sp

MODULE=mod_sp

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install etc_install
