##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

ETC =	floatws.lua ioncore-bindings.lua ionws.lua kludges.lua \
	look-blackwheat.lua look-brownsteel.lua look-clean.lua \
	look-dusky.lua look-greyviolet.lua look-ios.lua look-cleanviolet.lua \
	look-simpleblue.lua look-wheat.lua query.lua ioncore-example.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(ETCDIR); \
	done

	@ if test -f $(ETCDIR)/ioncore.lua ; then \
		echo ""; \
		echo "*** Not installing $(ETCDIR)/ioncore.lua."; \
	else \
		echo ""; \
		echo "*** Installing $(ETCDIR)/ioncore.lua"; \
		echo "*** This file will not be overwritten by subsequent 'make installs', but"; \
		echo "*** other files in this directory, excluding draw.lua, will be"; \
		echo "*** overwritten. Remember that when modifying system-wide configurations."; \
		cp $(ETCDIR)/ioncore-example.lua $(ETCDIR)/ioncore.lua; \
	fi

	@ if test ! -f $(ETCDIR)/draw.lua ; then \
		echo ""; \
		echo "*** Linking draw.lua to look-clean.lua"; \
		ln -s look-clean.lua $(ETCDIR)/draw.lua; \
	fi

	@ if test -f $(ETCDIR)/ion.lua ; then \
		echo ""; \
		echo "*** You seem to have an old ion.lua file in $(ETCDIR)."; \
		echo "*** Configuration files have changed drastically lately and old"; \
		echo "*** configuration files will for the most part not be usable. The new"; \
		echo "*** main configuration file is ioncore.lua now."; \
	fi

	@ echo ""
