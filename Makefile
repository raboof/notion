##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system.mk

######################################

#DIST: SUBDIRS=libtu wmcore query src
SUBDIRS=wmcore query src
INSTALL_SUBDIRS=src

TARGETS=man/ion.1x

SCRIPTS=scripts/ion-edit scripts/ion-man scripts/ion-runinxterm \
	scripts/ion-ssh scripts/ion-view

ETC=	etc/bindings-default.conf etc/kludges.conf \
	etc/look-brownsteel.conf etc/look-greyviolet.conf \
	etc/look-simpleblue.conf etc/look-wheat.conf etc/sample.conf \
	etc/query.conf etc/look-dusky.conf

DOCS= 	README LICENSE ChangeLog \
	doc/config.txt doc/functions.txt doc/draw.txt doc/query.txt

IONETCDIR=$(ETCDIR)/ion-devel

######################################

include rules.mk

######################################

man/ion.1x: man/ion.1x.in
	sed 's#PREFIX#$(PREFIX)#g' man/ion.1x.in > man/ion.1x

_install:
	$(INSTALLDIR) $(BINDIR)
	for i in $(SCRIPTS); do \
		$(INSTALL) -m $(BIN_MODE) $$i $(BINDIR); \
	done

	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) man/ion.1x $(MANDIR)/man1

	$(INSTALLDIR) $(DOCDIR)/ion
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR)/ion; \
	done

	$(INSTALLDIR) $(IONETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(IONETCDIR); \
	done

	if uname -s -p|grep "SunOS sparc" > /dev/null; then \
		echo "Patching bindings-default.conf as bindings-sun.conf"; \
		patch -o $(IONETCDIR)/bindings-sun.conf etc/bindings-default.conf < etc/bindings-sun.diff; \
	fi

	@ if test -f $(IONETCDIR)/ion.conf ; then \
		echo "$(IONETCDIR)/ion.conf already exists. Not installing one."; \
	else \
		echo "Installing configuration file $(IONETCDIR)/ion.conf"; \
		if uname -s -p|grep "SunOS sparc" > /dev/null; then \
			echo "Using bindings-sun.conf"; \
			sed s/bindings-default/bindings-sun/ \
			$(IONETCDIR)/sample.conf > $(IONETCDIR)/ion.conf; \
			chmod $(DATA_MODE) $(IONETCDIR)/ion.conf; \
		else \
			echo "Using bindings-default.conf"; \
			cp $(IONETCDIR)/sample.conf $(IONETCDIR)/ion.conf; \
		fi; \
	fi

	@ if test ! -f $(IONETCDIR)/draw.conf ; then \
		echo "Linking draw.conf to look-dusky.conf"; \
		ln -s look-dusky.conf $(IONETCDIR)/draw.conf; \
	fi

