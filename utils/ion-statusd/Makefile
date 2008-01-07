##
## Ion-statusd Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

EXTRA_EXECUTABLE = ion-statusd

SOURCES = ion-statusd.c exec.c extlrx.c

INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBEXTL_INCLUDES) $(LIBTU_INCLUDES)
LIBS += $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"
           
LUA_SOURCES = statusd_date.lua statusd_mail.lua statusd_load.lua

MAKE_EXPORTS = statusd

include $(TOPDIR)/libmainloop/rx.mk

######################################

include $(TOPDIR)/build/rules.mk

######################################

_install: lc_install executable_install
