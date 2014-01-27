##
## Query module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=complete.c edln.c input.c listing.c main.c wedln.c \
        wedln-wrappers.c wmessage.c query.c fwarn.c history.c

MAKE_EXPORTS=mod_query

MODULE=mod_query
MODULE_STUB=mod_query.lua
LUA_SOURCES=mod_query_chdir.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

main.o: exports.h

_install: module_install lc_install
