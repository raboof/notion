##
## Ion Lua support Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES)
INCLUDES += $(LUA_INCLUDES)
#DIST: INCLUDES += -I../libtu/include -I.. 
INCLUDES += -I..

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=luaextl.c

TARGETS=luaextl.a

######################################

include $(TOPDIR)/rules.mk

######################################

luaextl.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

