##
## Notionflux Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

SOURCES=notionflux.c

TARGETS=notionflux

CFLAGS += $(XOPEN_SOURCE) -std=c99

INCLUDES += $(X11_INCLUDES) -I$(TOPDIR)

LIBS += $(X11_LIBS)

EXTRA_EXECUTABLE = notionflux

######################################

include $(TOPDIR)/build/rules.mk

######################################

notionflux: $(OBJS) $(EXT_OBJS)
	$(CC) $(LINKOPTS) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) $(LDFLAGS) -o $@

_install: notionflux
	$(INSTALLDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m $(BIN_MODE) notionflux $(DESTDIR)$(BINDIR)
