##
## Ion share/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

LUACODE = ioncorelib.lua querylib.lua compat.lua
SHELLSCRIPTS = ion-edit ion-man ion-runinxterm ion-ssh ion-view 

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
