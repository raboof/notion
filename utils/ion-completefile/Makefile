##
## Ion-completefile Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/system-inc.mk

######################################

LIBS += $(LIBTU_LIBS)
INCLUDES += $(LIBTU_INCLUDES)
CFLAGS += $(XOPEN_SOURCE)

SOURCES=ion-completefile.c

TARGETS=ion-completefile

######################################

include $(TOPDIR)/rules.mk

######################################

ion-completefile: $(SOURCES)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(EXTRABINDIR)
	$(INSTALL) -s -m $(BIN_MODE) ion-completefile $(EXTRABINDIR)
