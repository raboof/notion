##
## Ion manual page Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

TARGETS=ion3.1 pwm3.1

######################################

include $(TOPDIR)/rules.mk

######################################

%.1: %.1.in
	sed 's#ETCDIR#$(ETCDIR)#g; s#DOCDIR#$(DOCDIR)#g;' $< > $@

_install:
	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) ion3.1 $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) pwm3.1 $(MANDIR)/man1

#
# Awful hack that may not work on the user's system. This is rule is
# supposed to be executed before packaging Ion.
#

ion3.1.in.txt: ion3.1.in
	man ./$< | perl -p -i -e 's/.\10//g' > $@

