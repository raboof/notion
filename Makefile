##
## Notion Makefile
##

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

RELEASE_TARGETS := $(addsuffix -release,major minor patch)
$(RELEASE_TARGETS):
	@./nextversion.sh $(@) >/dev/null 2>/dev/null
	$(eval RELEASE_VERSION=$(shell ./nextversion.sh $(@)))
	git tag -s -m "Release $(RELEASE_VERSION)" $(RELEASE_VERSION)
	@echo 'Use "git push --tags" to push the newly tagged release.'

.PHONY: test $(RELEASE_TARGETS)

test:
	$(MAKE) -C mod_xrandr test
	$(MAKE) -C mod_xinerama test
	$(MAKE) -C libtu test
	$(MAKE) -C libextl test
	$(MAKE) -C test
