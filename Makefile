##
## Notion Makefile
##
REL?=p 

export NOTION_RELEASE = $(shell ./version.sh)

# Include system-specific configuration: auto-generated and optionally local
include build/system-inc.mk

# List of modules
include modulelist.mk

######################################

INSTALL_SUBDIRS=\
	$(MODULE_LIST) \
	ioncore notion etc utils man po contrib/scripts

SUBDIRS = $(LIBS_SUBDIRS) $(INSTALL_SUBDIRS)

DOCS = README.md LICENSE AUTHORS CONTRIBUTING.md

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

snapshot:
	PWD=`pwd` ;\
	DIR=`basename "$$PWD"` ;\
	RELEASE=`./nextversion.sh`-snapshot ;\
	perl -p -i -e "s/^#define NOTION_RELEASE.*/#define NOTION_RELEASE \"$$RELEASE\"/" version.h ;\
	cd .. ;\
	tar --exclude-vcs -czf notion-$$RELEASE-src.tar.gz $$DIR ;\
	tar --exclude-vcs -cjf notion-$$RELEASE-src.tar.bz2 $$DIR ;\
	cd $$DIR ;\
	git checkout version.h

dist:
	RELEASE=`./nextversion.sh -$(REL)` ;\
	git tag -s -m "Release $$RELEASE" $$RELEASE ; git push --tags

.PHONY: test

test:
	$(MAKE) -C mod_xrandr test
	$(MAKE) -C mod_xinerama test
	$(MAKE) -C libtu test
	$(MAKE) -C libextl test
	$(MAKE) -C test
