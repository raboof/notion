##
## Notionflux Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/system.mk

######################################

CFLAGS += $(XOPEN_SOURCE)

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)

LIBS += $(X11_LIBS) 

SOURCES=notionflux.c

TARGETS=notionflux

######################################

include $(TOPDIR)/build/rules.mk

######################################

notionflux: $(SOURCES)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -s -m $(BIN_MODE) notionflux $(BINDIR)
