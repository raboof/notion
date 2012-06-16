##
## Notion Makefile
##

# System-specific configuration is in system.mk
include build/system-inc.mk

# List of modules
include modulelist.mk

######################################

INSTALL_SUBDIRS=\
	$(MODULE_LIST) \
	ioncore notion etc utils man po

SUBDIRS = $(LIBS_SUBDIRS) $(INSTALL_SUBDIRS)

DOCS = README LICENSE ChangeLog RELNOTES

TO_REALCLEAN = build/ac/system-ac.mk

POTFILE=po/ion.pot

######################################

include build/rules.mk

######################################

_install:
	$(INSTALLDIR) $(DESTDIR)$(DOCDIR)
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(DOCDIR); \
	done
