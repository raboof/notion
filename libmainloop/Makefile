##
## Libmainloop Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES = select.c defer.c signal.c hooks.c exec.c

#MAKE_EXPORTS=mainloop

TARGETS = libmainloop.a

######################################

include $(TOPDIR)/build/rules.mk

######################################

libmainloop.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+

_install:
