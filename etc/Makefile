##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

IONETCDIR = $(ETCDIR)/ion-devel

ETC = common-frame-bindings.conf floatws.conf ioncore-bindings.conf \
      ionws.conf kludges.conf look-blackwheat.conf look-brownsteel.conf \
      look-clean.conf look-dusky.conf look-greyviolet.conf look-ios.conf \
      look-simpleblue.conf look-wheat.conf query.conf ioncore-example.conf

######################################

include $(TOPDIR)/rules.mk

######################################

_install:
	$(INSTALLDIR) $(IONETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(IONETCDIR); \
	done

	@ if uname -s -p|grep "SunOS sparc" > /dev/null; then \
		echo ""; \
		echo "*** Patching common-frame-bindings.conf with common-frame-bindings-sun.diff"; \
		patch $(IONETCDIR)/common-frame-bindings.conf < common-frame-bindings-sun.diff; \
	fi

	@ if test -f $(IONETCDIR)/ioncore.conf ; then \
		echo ""; \
		echo "*** Not installing $(IONETCDIR)/ioncore.conf."; \
	else \
		echo ""; \
		echo "*** Installing $(IONETCDIR)/ioncore.conf"; \
		echo "*** This file will not be overwritten by subsequent 'make installs', but"; \
		echo "*** other files in this directory, excluding draw.conf, will be"; \
		echo "*** overwritten. Remember that when modifying system-wide configurations."; \
		cp $(IONETCDIR)/ioncore-example.conf $(IONETCDIR)/ioncore.conf; \
	fi

	@ if test ! -f $(IONETCDIR)/draw.conf ; then \
		echo ""; \
		echo "*** Linking draw.conf to look-clean.conf"; \
		ln -s look-clean.conf $(IONETCDIR)/draw.conf; \
	fi

	@ if test -f $(IONETCDIR)/ion.conf ; then \
		echo ""; \
		echo "*** You seem to have an old ion.conf file in $(IONETCDIR)."; \
		echo "*** Configuration files have changed drastically lately and old"; \
		echo "*** configuration files will for the most part not be usable. The new"; \
		echo "*** main configuration file is ioncore.conf now."; \
	fi

	@ echo ""
