
## Ion-completefile Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: LIBS += -L../../libtu -ltu
LIBS += -ltu
#DIST: INCLUDES += -I../../libtu/include -I../..
INCLUDES += -I../..
CFLAGS += $(XOPEN_SOURCE)

SOURCES=ion-completefile.c

TARGETS=ion-completefile

######################################

include $(TOPDIR)/rules.mk

######################################

ion-completefile: $(SOURCES)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ion-completefile $(BINDIR)
	# $(STRIP) $(BINDIR)/ion-completefile
