##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system-inc.mk

######################################

#DIST: SUBDIRS = libtu ioncore ionws floatws query man
SUBDIRS = ioncore ionws floatws query man
INSTALL_SUBDIRS = ioncore ionws floatws query scripts man etc

DOCS = README LICENSE ChangeLog

######################################

include rules.mk

######################################

_install:
	$(INSTALLDIR) $(DOCDIR)/ion
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR)/ion; \
	done
