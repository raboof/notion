##
## Ion share/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

LUA_SOURCES = ioncorelib.lua ioncore-mplexfns.lua compat.lua \
	      ioncore-efbb.lua

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
	for i in $(LUA_SOURCES) $(LUA_COMPILED); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(SHAREDIR); \
	done

%: %.in
	sed 's#SHAREDIR#$(SHAREDIR)#g' $< > $@
	chmod $(BIN_MODE) $@
