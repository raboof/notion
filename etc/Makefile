##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

ETC =	cfg_ion.lua cfg_bindings.lua cfg_menus.lua cfg_kludges.lua \
	cfg_ionws.lua cfg_floatws.lua cfg_query.lua cfg_menu.lua \
	look_brownsteel.lua look_clean.lua look_dusky.lua \
	look_greyviolet.lua look_ios.lua look_cleanviolet.lua \
	look_simpleblue.lua look_wheat2.lua look_cleanios.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install: etc_install
	rm -f $(ETCDIR)/look.lua
	ln -s look_cleanviolet.lua $(ETCDIR)/look.lua
