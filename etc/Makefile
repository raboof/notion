##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

ETC =	floatws.lua ioncore-bindings.lua ionws.lua kludges.lua \
	look-brownsteel.lua look-clean.lua look-dusky.lua \
	look-greyviolet.lua look-ios.lua look-cleanviolet.lua \
	look-simpleblue.lua look-wheat2.lua query.lua menu.lua \
	ioncore.lua ioncore-menus.lua look-cleanios.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done
	rm -f $(ETCDIR)/draw.lua
	ln -s look-cleanviolet.lua $(ETCDIR)/draw.lua
