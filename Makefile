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

relocatable_build:
	$(MAKE) RELOCATABLE=1 PREFIX=

dist:
	PWD=`pwd` ;\
	DIR=`basename "$$PWD"` ;\
	RELEASE=`./nextversion.sh` ;\
	perl -p -i -e "s/^#define NOTION_RELEASE.*/#define NOTION_RELEASE \"$$RELEASE\"/" version.h ;\
	git tag $$RELEASE ; git push --tags ;\
	cd .. ;\
	tar --exclude-vcs -czf notion-$$RELEASE-src.tar.gz $$DIR ;\
	tar --exclude-vcs -cjf notion-$$RELEASE-src.tar.bz2 $$DIR ;\
	cd $$DIR ;\
	git checkout version.h

.PHONY: test

test:
	$(MAKE) -C test
