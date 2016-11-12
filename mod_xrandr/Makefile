##
## Ion xrandr module Makefile
##
##

# System specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) $(X11_INCLUDES) -I$(TOPDIR) $(shell pkg-config --cflags-only-I xrandr)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE) $(shell $(PKG_CONFIG) --cflags-only-other xrandr)

SOURCES=mod_xrandr.c

MAKE_EXPORTS=mod_xrandr
LIBS = $(X11_LIBS) $(shell $(PKG_CONFIG) --libs xrandr)
MODULE=mod_xrandr
MODULE_STUB=mod_xrandr.lua
ETC=cfg_xrandr.lua


######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(DESTDIR)$(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(ETCDIR); \
	done

######################################

.PHONY: tarball
tarball: $(SOURCES) $(ETC) $(DOCS) Makefile
	sh -c 'BASENAME=ion-devel-$(MODULE)-`date -r \`ls -t $+ | head -n 1\` +%Y%m%d`; \
		mkdir $$BASENAME; \
		cp $+ $$BASENAME; \
		tar -cvjf $$BASENAME.tar.bz2 $$BASENAME/*; \
		rm -Rf $$BASENAME'

.PHONY: test
test: $(SOURCES)
	for i in test_xrandr*.lua ; do echo 'Testing' $$i ; $(LUA) $$i ; done

######################################

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)

mod_xrandr.o: exports.h
