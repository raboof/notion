##
## Notion utils/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

SUBDIRS=ion-completefile ion-statusd
INSTALL_SUBDIRS=$(SUBDIRS)

SHELLSCRIPTS = ion-runinxterm ion-completeman

TARGETS = ion-completeman

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install:
	$(INSTALLDIR) $(SHAREDIR)
	for i in $(SHELLSCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(SHAREDIR); \
	done

%: %.in
	sed 's#@SHAREDIR@#$(SHAREDIR)#g' $< | \
	sed 's#@VARDIR@#$(VARDIR)#g' > $@
	chmod $(BIN_MODE) $@
