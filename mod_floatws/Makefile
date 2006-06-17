##
## Floating workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c floatws.c floatframe.c placement.c \
	floatwspholder.c floatwsrescueph.c

MAKE_EXPORTS=mod_floatws

MODULE=mod_floatws

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
