##
## Ion scripts Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

SCRIPTS=ion-edit ion-man ion-runinxterm ion-ssh ion-view

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(BINDIR)
	for i in $(SCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(BINDIR); \
	done
