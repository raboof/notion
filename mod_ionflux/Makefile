##
## Ionflux remote control module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/system.mk

######################################

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=mod_ionflux.c

MODULE=mod_ionflux

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
