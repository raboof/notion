##
## Ion share/ Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

SUBDIRS=ion-completefile
INSTALL_SUBDIRS=$(SUBDIRS)

LUA_SOURCES = ioncore-ext.lua ioncore-luaext.lua ioncore-bindings.lua \
	      ioncore-winprops.lua ioncore-misc.lua ioncore-efbb.lua

SHELLSCRIPTS = ion-edit ion-man ion-runinxterm ion-ssh ion-view \
	       ion-completeman

TARGETS=ion-ssh ion-man ion-completeman

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(SHAREDIR)
	for i in $(SHELLSCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(SHAREDIR); \
	done
	$(INSTALLDIR) $(LCDIR)
	for i in $(LUA_COMPILED); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(LCDIR); \
	done

%: %.in
	sed 's#@SHAREDIR@#$(SHAREDIR)#g' $< | \
	sed 's#@VARDIR@#$(VARDIR)#g' > $@
	chmod $(BIN_MODE) $@
