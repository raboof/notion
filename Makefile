##
## Notion Makefile
##

# Include system-specific configuration: auto-generated and optionally local 
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

POTFILE=po/notion.pot

######################################

include build/rules.mk

######################################

_install:
	$(INSTALLDIR) $(DESTDIR)$(DOCDIR)
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(DOCDIR); \
	done

.PHONY: test

test:
	$(MAKE) -C test
