##
## Ion workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c placement.c split.c ionws.c ionframe.c

MAKE_EXPORTS=mod_ionws

MODULE=mod_ionws

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install



