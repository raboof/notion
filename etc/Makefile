##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

IONETCDIR = $(ETCDIR)/ion-devel

ETC = common-frame-bindings.lua floatws.lua ioncore-bindings.lua \
      ionws.lua kludges.lua look-blackwheat.lua look-brownsteel.lua \
      look-clean.lua look-dusky.lua look-greyviolet.lua look-ios.lua \
      look-simpleblue.lua look-wheat.lua query.lua ioncore-example.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(IONETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(IONETCDIR); \
	done

	@ if test -f $(IONETCDIR)/ioncore.lua ; then \
		echo ""; \
		echo "*** Not installing $(IONETCDIR)/ioncore.lua."; \
	else \
		echo ""; \
		echo "*** Installing $(IONETCDIR)/ioncore.lua"; \
		echo "*** This file will not be overwritten by subsequent 'make installs', but"; \
		echo "*** other files in this directory, excluding draw.lua, will be"; \
		echo "*** overwritten. Remember that when modifying system-wide configurations."; \
		cp $(IONETCDIR)/ioncore-example.lua $(IONETCDIR)/ioncore.lua; \
	fi

	@ if test ! -f $(IONETCDIR)/draw.lua ; then \
		echo ""; \
		echo "*** Linking draw.lua to look-clean.lua"; \
		ln -s look-clean.lua $(IONETCDIR)/draw.lua; \
	fi

	@ if test -f $(IONETCDIR)/ion.lua ; then \
		echo ""; \
		echo "*** You seem to have an old ion.lua file in $(IONETCDIR)."; \
		echo "*** Configuration files have changed drastically lately and old"; \
		echo "*** configuration files will for the most part not be usable. The new"; \
		echo "*** main configuration file is ioncore.lua now."; \
	fi

	@ echo ""
