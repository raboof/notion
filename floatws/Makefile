##
## Floating workspace module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
MODULEDIR = $(LIBDIR)/ion-devel
CFLAGS += $(XOPEN_SOURCE)

SOURCES=main.c floatws.c floatframe.c

MAKE_EXPORTS=floatws_module

MODULE=floatws

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
