##
## Ion manual page Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

MAN_TARGETS=ion3.1 pwm3.1 
WELCOME_TARGETS=welcome.txt
TARGETS=$(MAN_TARGETS) $(WELCOME_TARGETS)

MKMAN=$(LUA) ../build/mkman.lua
GROFF=groff -man -Tascii
#FILTERCRAP=perl -p -i -e 's/.\10//g'
FILTERCRAP=$(LUA) -e 'io.write((string.gsub(io.read("*a"), ".\8", "")))'

CONFIGS=../etc/cfg_bindings.lua \
	../etc/cfg_ionws.lua \
	../etc/cfg_floatws.lua \
	../etc/cfg_query.lua \
	../etc/cfg_menu.lua

######################################

include $(TOPDIR)/rules.mk

######################################

%.1: %.1.in $(CONFIGS)
	$(MKMAN) -i $< -o $@ -D ETCDIR $(ETCDIR) -D DOCDIR $(DOCDIR) $(CONFIGS)

welcome%txt: welcome%head ion3%1
	(cat welcome$*head; \
	$(GROFF) ion3$*1 | $(FILTERCRAP)) > $@

_install:
	$(INSTALLDIR) $(MANDIR)/man1
	for i in $(MAN_TARGETS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(MANDIR)/man1 ; \
	done
	for i in $(WELCOME_TARGETS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(SHAREDIR) ; \
	done
