##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

ETC =	cfg_ion.lua cfg_bindings.lua cfg_menus.lua cfg_kludges.lua \
	cfg_ionws.lua cfg_floatws.lua cfg_query.lua cfg_menu.lua \
	look-brownsteel.lua look-clean.lua look-dusky.lua \
	look-greyviolet.lua look-ios.lua look-cleanviolet.lua \
	look-simpleblue.lua look-wheat2.lua look-cleanios.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done
	rm -f $(ETCDIR)/look.lua
	ln -s look-cleanviolet.lua $(ETCDIR)/look.lua
