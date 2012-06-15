##
## Ion etc Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

ETC =	cfg_notion.lua cfg_notioncore.lua cfg_kludges.lua cfg_defaults.lua \
	cfg_tiling.lua cfg_query.lua cfg_menu.lua \
	cfg_statusbar.lua cfg_dock.lua cfg_layouts.lua \
	look_brownsteel.lua look_clean.lua look_dusky.lua \
	look_greyviolet.lua look_ios.lua look_cleanviolet.lua \
	look_simpleblue.lua look_cleanios.lua look_newviolet.lua \
        look_greenlight.lua \
        lookcommon_clean.lua lookcommon_clean_stdisp.lua \
        lookcommon_clean_tab.lua lookcommon_clean_frame.lua \
        lookcommon_emboss.lua lookcommon_emboss_stdisp.lua \
        lookcommon_emboss_tab.lua lookcommon_emboss_frame.lua

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: etc_install
	rm -f $(ETCDIR)/look.lua
	ln -s look_newviolet.lua $(ETCDIR)/look.lua
