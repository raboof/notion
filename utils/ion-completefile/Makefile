##
## Ion-completefile Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

LIBS += $(LIBTU_LIBS)
INCLUDES += $(LIBTU_INCLUDES)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=ion-completefile.c

TARGETS=ion-completefile

######################################

include $(TOPDIR)/build/rules.mk

######################################

ion-completefile: $(SOURCES)
	$(CC) $< $(CFLAGS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(EXTRABINDIR)
	$(INSTALLBIN) ion-completefile $(EXTRABINDIR)
