##
## Ion po Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

XGETTEXT = xgettext
MSGFMT = msgfmt -c
MSGMERGE = msgmerge

TRANSLATIONS := fi
MO_FILES := $(patsubst %,%.mo, $(TRANSLATIONS))

POTDIRS=../ioncore \
	../luaextl \
	../mod_ionws \
	../mod_floatws \
	../mod_autows \
	../mod_query \
	../mod_menu \
	../mod_sm \
	../mod_sp \
	../de \
	../ext_statusbar \
	../ion \
	../pwm \
	../share

POTFILE=ion.pot

TARGETS = $(MO_FILES)

ifndef LOCALEDIR
LOCALEDIR := $(PREFIX)/share/locale
endif

######################################

include $(TOPDIR)/rules.mk

######################################

potfiles: potdirs_potfiles
	(for i in $(POTDIRS); do cat $$i/potfiles_c|sed "s:\w\+:$$i/&:"; done) > potfiles_c
	(for i in $(POTDIRS); do cat $$i/potfiles_lua|sed "s:\w\+:$$i/&:"; done) > potfiles_lua

potdirs_potfiles:
	for i in $(POTDIRS); do make -C $$i _potfiles; done

pot: $(POTFILE)

$(POTFILE): potfiles
	$(XGETTEXT) -kTR -kDUMMY_TR -o $(POTFILE).tmp -f potfiles_c
	$(LUA) $(TOPDIR)/build/lxgettext.lua -out $(POTFILE) -add $(POTFILE).tmp -f potfiles_lua
	$(RM) $(POTFILE).tmp

%.mo: %.po
	$(MSGFMT) -o $@ $<

_install:
	for i in $(TRANSLATIONS); do \
		$(INSTALL) -m $(DATA_MODE) $$i.mo $(LOCALEDIR)/$$i/LC_MESSAGES/ion3.mo; \
	done

update_translations: pot
	for i in $(TRANSLATIONS); do \
		$(MSGMERGE) -U $$i.po $(POTFILE); \
	done
