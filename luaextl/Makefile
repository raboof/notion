##
## Ion Lua support Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I.. $(LUA_INCLUDES)
INCLUDES += $(X11_INCLUDES) -I.. $(LUA_INCLUDES)
MODULEDIR = $(LIBDIR)/ion-devel
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=luaextl.c

TARGETS=luaextl.a

######################################

include $(TOPDIR)/rules.mk

######################################

luaextl.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

_install: module_install
