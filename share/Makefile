##
## Ion share/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

LUACODE = ioncorelib.lua querylib.lua compat.lua
SHELLSCRIPTS = ion-edit ion-man ion-runinxterm ion-ssh ion-view 

TARGETS=ion-ssh ion-man

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(SHAREDIR)
	for i in $(SHELLSCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(SHAREDIR); \
	done
	for i in $(LUACODE); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(SHAREDIR); \
	done

ion-ssh: ion-ssh.in
	sed 's#SHAREDIR#$(SHAREDIR)#g' $< > $@
	chmod $(BIN_MODE) $@

ion-man: ion-man.in
	sed 's#SHAREDIR#$(SHAREDIR)#g' $< > $@
	chmod $(BIN_MODE) $@
