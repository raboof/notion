##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system-inc.mk

######################################

#DIST: SUBDIRS = libtu $(MODULE_LIST) ioncore man
SUBDIRS = $(MODULE_LIST) ioncore man
INSTALL_SUBDIRS = $(MODULE_LIST) ioncore scripts man etc

DOCS = README LICENSE ChangeLog

######################################

include rules.mk

######################################

_install:
	$(INSTALLDIR) $(DOCDIR)/ion
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR)/ion; \
	done
