##
## Ion workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c tiling.c placement.c split.c split-stdisp.c \
	splitfloat.c panehandle.c ops.c

MAKE_EXPORTS=mod_tiling

MODULE=mod_tiling

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install



