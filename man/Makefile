##
## Ion manual page Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

TRANSLATIONS=fi cs de

WELCOME_TARGETS=\
	welcome.txt \
	$(foreach tr, $(TRANSLATIONS), welcome.$(tr).txt)

TARGETS=ion3.1 $(foreach tr, $(TRANSLATIONS), ion3.$(tr).1) \
	pwm3.1 $(foreach tr, $(TRANSLATIONS), pwm3.$(tr).1) \
	$(WELCOME_TARGETS)

MKMAN=$(LUA) ../build/mkman.lua
NROFF=nroff -man -Tlatin1
#FILTERCRAP=perl -p -i -e 's/.\10//g'
FILTERCRAP=$(LUA) -e 'io.write((string.gsub(io.read("*a"), ".\8", "")))'

CONFIGS=../etc/cfg_ioncore.lua \
	../etc/cfg_ionws.lua \
	../etc/cfg_query.lua \
	../etc/cfg_menu.lua

# TODO: PWM configuration file is undocumented
PWM_CONFIGS=\
	../etc/cfg_ioncore.lua \
	../etc/cfg_menu.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

ion3.1: ion3.in $(CONFIGS)
	$(MKMAN) -i $< -o $@ -D ETCDIR $(ETCDIR) -D DOCDIR $(DOCDIR) $(CONFIGS)

pwm3.1: pwm3.in $(PWM_CONFIGS)
	$(MKMAN) -i $< -o $@ -D ETCDIR $(ETCDIR) -D DOCDIR $(DOCDIR) $(PWM_CONFIGS)

ion3.%.1: ion3.%.in $(CONFIGS) ../po/%.po
	$(MKMAN) -po ../po/$*.po -i $< -o $@ -D ETCDIR $(ETCDIR) -D DOCDIR $(DOCDIR) $(CONFIGS)

pwm3.%.1: pwm3.%.in $(PWM_CONFIGS) ../po/%.po
	$(MKMAN) -po ../po/$*.po -i $< -o $@ -D ETCDIR $(ETCDIR) -D DOCDIR $(DOCDIR) $(PWM_CONFIGS)

welcome%txt: welcome%head ion3%1
	(cat welcome$*head; \
	$(NROFF) ion3$*1 | $(FILTERCRAP)) > $@

_install:
	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) ion3.1 $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) pwm3.1 $(MANDIR)/man1
	for tr in $(TRANSLATIONS); do \
		$(INSTALLDIR) $(MANDIR)/$$tr/man1 ; \
		$(INSTALL) -m $(DATA_MODE) ion3.$$tr.1 $(MANDIR)/$$tr/man1/ion3.1 ; \
		$(INSTALL) -m $(DATA_MODE) pwm3.$$tr.1 $(MANDIR)/$$tr/man1/pwm3.1 ; \
	done
	$(INSTALLDIR) $(SHAREDIR)
	for i in $(WELCOME_TARGETS); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(SHAREDIR) ; \
	done
