##
## Session management support module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)
LIBS += $(X11_LIBS) -lSM -lICE

SOURCES=sm.c sm_matchwin.c sm_session.c

MAKE_EXPORTS=mod_sm

MODULE=mod_sm

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
