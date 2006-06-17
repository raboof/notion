##
## Ion workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c ionws.c placement.c split.c split-stdisp.c \
	splitfloat.c panehandle.c

MAKE_EXPORTS=mod_ionws

MODULE=mod_ionws

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install



