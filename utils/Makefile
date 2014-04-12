##
## Notion utils/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

SUBDIRS=ion-completefile
INSTALL_SUBDIRS=$(SUBDIRS)

SHELLSCRIPTS = ion-runinxterm ion-completeman notion-lock

TARGETS = ion-completeman

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install:
	$(INSTALLDIR) $(DESTDIR)$(SHAREDIR)
	for i in $(SHELLSCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(DESTDIR)$(SHAREDIR); \
	done

%: %.in
	sed 's#@SHAREDIR@#$(SHAREDIR)#g' $< | \
	sed 's#@VARDIR@#$(VARDIR)#g' > $@
	chmod $(BIN_MODE) $@
