##
## Default drawing engine module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=init.c draw.c font.c colour.c brush.c fontset.c style.c

MODULE=de

MAKE_EXPORTS=de

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
