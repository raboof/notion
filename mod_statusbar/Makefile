##
## Ion ext_statusbar Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES = main.c statusbar.c draw.c

SUBDIRS = ion-statusd
INSTALL_SUBDIRS = $(SUBDIRS)

MAKE_EXPORTS = mod_statusbar

MODULE = mod_statusbar
MODULE_STUB = mod_statusbar.lua

ETC = cfg_statusbar.lua

######################################

include $(TOPDIR)/rules.mk

######################################

_install: module_install etc_install symlink_install

symlink_install:
	rm -f $(LCDIR)/ext_statusbar.lc
	ln -s mod_statusbar.lc $(LCDIR)/ext_statusbar.lc
