##
## Floating workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c floatws.c floatframe.c placement.c

MAKE_EXPORTS=mod_floatws

MODULE=mod_floatws

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
