##
## Floating workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=main.c floatws.c floatframe.c placement.c

MAKE_EXPORTS=floatwsmod

MODULE=floatwsmod

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
