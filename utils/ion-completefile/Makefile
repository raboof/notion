##
## Notion ion-completefile Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

EXTRA_EXECUTABLE = ion-completefile

SOURCES=ion-completefile.c

LIBS += $(LIBTU_LIBS)
INCLUDES += $(LIBTU_INCLUDES)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: executable_install
