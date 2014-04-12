##
## Notionflux Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

CFLAGS += $(XOPEN_SOURCE)

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)

LIBS += $(X11_LIBS) 

EXTRA_EXECUTABLE = notionflux

SOURCES=notionflux.c

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: executable_install
