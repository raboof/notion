##
## Notionflux Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
USE_READLINE := 1
include $(TOPDIR)/build/system-inc.mk

######################################

SOURCES=notionflux.c

TARGETS=notionflux

CFLAGS += $(POSIX_SOURCE)
CFLAGS += $(XOPEN_SOURCE)
CFLAGS += $(C99_SOURCE)

INCLUDES += $(X11_INCLUDES) $(LUA_INCLUDES) $(READLINE_INCLUDES) -I$(TOPDIR)

LIBS += $(X11_LIBS) $(LUA_LIBS) $(READLINE_LIBS)

EXTRA_EXECUTABLE = notionflux

######################################

include $(TOPDIR)/build/rules.mk

######################################

notionflux: $(OBJS) $(EXT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LINKOPTS) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) -o $@ $(LIBS)

_install: notionflux
	$(INSTALLDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) $(INSTALL_STRIP) -m $(BIN_MODE) notionflux $(DESTDIR)$(BINDIR)
