##
## Ion manual page Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

TARGETS=ion.1x

######################################

include $(TOPDIR)/rules.mk

######################################

ion.1x: ion.1x.in
	sed 's#PREFIX#$(PREFIX)#g' ion.1x.in > ion.1x

_install:
	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) ion.1x $(MANDIR)/man1
