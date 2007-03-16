##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

ETC =	cfg_ion.lua cfg_ioncore.lua cfg_kludges.lua cfg_defaults.lua \
	cfg_tiling.lua cfg_query.lua cfg_menu.lua \
	cfg_statusbar.lua cfg_dock.lua cfg_layouts.lua \
	look_brownsteel.lua look_clean.lua look_dusky.lua \
	look_greyviolet.lua look_ios.lua look_cleanviolet.lua \
	look_simpleblue.lua look_cleanios.lua \
        lookcommon_clean.lua lookcommon_emboss.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: etc_install
	rm -f $(ETCDIR)/look.lua
	ln -s look_cleanviolet.lua $(ETCDIR)/look.lua
