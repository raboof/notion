##
## Ion Makefile
##

# System-specific configuration is in system.mk
include system-inc.mk

# List of modules
include modulelist.mk

######################################

EXT_LIST=ext_statusbar

INSTALL_SUBDIRS=\
	ioncore ion pwm etc utils man po \
	$(MODULE_LIST) $(EXT_LIST)

SUBDIRS = $(LIBS_SUBDIRS) $(INSTALL_SUBDIRS)

DOCS = README LICENSE ChangeLog RELNOTES

TO_REALCLEAN = system-ac.mk

POTFILE=po/ion.pot

######################################

include rules.mk

######################################

_install:
	$(INSTALLDIR) $(DOCDIR)
	for i in $(DOCS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DOCDIR); \
	done
