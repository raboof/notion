##
## Notion po Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

XGETTEXT = xgettext
#MSGFMT = msgfmt -c
MSGFMT = msgfmt
MSGMERGE = msgmerge
MSGCAT = msgcat
LXGETTEXT = lua-xgettext

TRANSLATIONS := fi cs
MO_FILES := $(patsubst %,%.mo, $(TRANSLATIONS))
PO_FILES := $(patsubst %,%.po, $(TRANSLATIONS))

POTDIRS=../ioncore \
	../mod_tiling \
	../mod_query \
	../mod_menu \
	../mod_sm \
	../mod_sp \
	../mod_statusbar \
	../mod_mgmtmode \
	../de \
	../notion \
	../pwm \
	../etc \
	$(LIBEXTL_DIR)

EXTRA_POTFILES_LUA=../build/mkman.lua

POTFILE=notion.pot

TARGETS = $(MO_FILES)

ifndef LOCALEDIR
LOCALEDIR := $(PREFIX)/share/locale
endif

######################################

include $(TOPDIR)/build/rules.mk

######################################

potfiles: potdirs_potfiles
	(for i in $(POTDIRS); do cat $$i/potfiles_c|sed "s:\w\+:$$i/&:"; done) > potfiles_c
	(for i in $(POTDIRS); do cat $$i/potfiles_lua|sed "s:\w\+:$$i/&:"; done; \
	 for i in $(EXTRA_POTFILES_LUA); do echo $$i; done) > potfiles_lua

potdirs_potfiles:
	for i in $(POTDIRS); do make -C $$i _potfiles; done

pot: $(POTFILE)

$(POTFILE)_c: potfiles
	$(XGETTEXT) -kTR -kDUMMY_TR -o $@ -f potfiles_c

$(POTFILE)_lua: potfiles
	$(LXGETTEXT) -k TR -k bdoc -k submenu -k menuentry -o $@ \
		`cat potfiles_lua`

$(POTFILE): $(POTFILE)_c $(POTFILE)_lua
	#
	# GNU gettext sucks bigtime, and refuses to work on POT
	# files without encoding set. Therefore we'll just have to
	# use plain old cat and hope that there aren't dupes.
	#
	#msgcat -o $@ $+
	#
	cat $+ > $@

%.mo: %.po
	$(MSGFMT) -o $@ $<

_install:
	for i in $(TRANSLATIONS); do \
		$(INSTALLDIR) $(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES ; \
		$(INSTALL) -m $(DATA_MODE) $$i.mo $(DESTDIR)$(LOCALEDIR)/$$i/LC_MESSAGES/notion.mo ; \
	done

update_fi: pot
	$(MSGMERGE) -U fi.po $(POTFILE)

update_cs: pot
	$(MSGMERGE) -U cs.po $(POTFILE)

