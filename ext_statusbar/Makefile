##
## Ion ext_statusbar Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INSTALL_SUBDIRS=$(SUBDIRS)

LUA_SOURCES = ext_statusbar.lua
ETC = cfg_statusbar.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(LCDIR)
	for i in $(LUA_COMPILED); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(LCDIR); \
	done
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done
