##
## Ion po Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

XGETTEXT = xgettext
#MSGFMT = msgfmt -c
MSGFMT = msgfmt
MSGMERGE = msgmerge
LXGETTEXT = $(LUA) $(TOPDIR)/build/lxgettext.lua

TRANSLATIONS := fi cs de ru
MO_FILES := $(patsubst %,%.mo, $(TRANSLATIONS))
PO_FILES := $(patsubst %,%.po, $(TRANSLATIONS))

POTDIRS=../ioncore \
	../mod_ionws \
	../mod_floatws \
	../mod_panews \
	../mod_query \
	../mod_menu \
	../mod_sm \
	../mod_sp \
	../mod_statusbar \
	../mod_mgmtmode \
	../de \
	../ion \
	../pwm \
	../etc \
	$(LIBEXTL_DIR)

EXTRA_POTFILES_LUA=../build/mkman.lua

POTFILE=ion3.pot

TARGETS = $(MO_FILES)

ifndef LOCALEDIR
LOCALEDIR := $(PREFIX)/share/locale
endif

######################################

include $(TOPDIR)/rules.mk

######################################

potfiles: potdirs_potfiles
	(for i in $(POTDIRS); do cat $$i/potfiles_c|sed "s:\w\+:$$i/&:"; done) > potfiles_c
	(for i in $(POTDIRS); do cat $$i/potfiles_lua|sed "s:\w\+:$$i/&:"; done; \
	 for i in $(EXTRA_POTFILES_LUA); do echo $$i; done) > potfiles_lua

potdirs_potfiles:
	for i in $(POTDIRS); do make -C $$i _potfiles; done

pot: $(POTFILE)

$(POTFILE): potfiles
	$(XGETTEXT) -kTR -kDUMMY_TR -o $(POTFILE).tmp -f potfiles_c
	$(LXGETTEXT) -k TR -k bdoc -k submenu -k menuentry \
		-o $(POTFILE) -a $(POTFILE).tmp -f potfiles_lua
	$(RM) $(POTFILE).tmp

%.mo: %.po
	$(MSGFMT) -o $@ $<

_install:
	for i in $(TRANSLATIONS); do \
		$(INSTALLDIR) $(LOCALEDIR)/$$i/LC_MESSAGES ; \
		$(INSTALL) -m $(DATA_MODE) $$i.mo $(LOCALEDIR)/$$i/LC_MESSAGES/ion3.mo ; \
	done

update_translations: pot
	for i in $(PO_FILES); do \
		$(MSGMERGE) -U $$i $(POTFILE); \
	done
