##
## Session management support module Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)
LIBS += $(X11_LIBS) -lSM -lICE

SOURCES=sm.c sm_matchwin.c sm_session.c

MAKE_EXPORTS=mod_sm

MODULE=mod_sm

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(ETCDIR)
