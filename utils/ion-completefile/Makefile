##
## Notion ion-completefile Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

LIBS += $(LIBTU_LIBS)
INCLUDES += $(LIBTU_INCLUDES)
CFLAGS += $(XOPEN_SOURCE) $(INCLUDES)

SOURCES=ion-completefile.c

TARGETS=ion-completefile

######################################

include $(TOPDIR)/build/rules.mk

######################################

ion-completefile: $(SOURCES)
	$(CC) $< $(CFLAGS) $(LDFLAGS) $(LIBS) -o $@

_install:
	$(INSTALLDIR) $(DESTDIR)$(EXTRABINDIR)
	$(INSTALL) -s -m $(BIN_MODE) ion-completefile $(DESTDIR)$(EXTRABINDIR)
