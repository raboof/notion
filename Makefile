##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system-inc.mk

######################################

#DIST: SUBDIRS = libtu $(MODULE_LIST) luaextl ioncore man scripts share
SUBDIRS = $(MODULE_LIST) luaextl ioncore man scripts share
INSTALL_SUBDIRS = $(MODULE_LIST) ioncore scripts man etc share

DOCS = README LICENSE ChangeLog

######################################

include rules.mk

######################################

_install:
	$(INSTALLDIR) $(DOCDIR)
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR); \
	done
