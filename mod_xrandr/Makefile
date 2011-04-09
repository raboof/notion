##
## Ion xrandr module Makefile
##
##

# System specific configuration is in system.mk
TOPDIR=../notion
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) $(X11_INCLUDES) -I$(TOPDIR)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=mod_xrandr.c

MAKE_EXPORTS=xrandr_module
LIBS = $(X11_LIBS) -lXrandr
MODULE=mod_xrandr

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: module_install
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done

######################################

.PHONY: tarball
tarball: $(SOURCES) $(ETC) $(DOCS) Makefile
	sh -c 'BASENAME=ion-devel-$(MODULE)-`date -r \`ls -t $+ | head -n 1\` +%Y%m%d`; \
		mkdir $$BASENAME; \
		cp $+ $$BASENAME; \
		tar -cvjf $$BASENAME.tar.bz2 $$BASENAME/*; \
		rm -Rf $$BASENAME'

######################################

.PHONY: tags
tags:
	exuberant-ctags -R . $(TOPDIR)
