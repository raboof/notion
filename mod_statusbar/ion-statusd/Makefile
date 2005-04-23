##
## Ion-statusd Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBEXTL_INCLUDES) $(LIBTU_INCLUDES)
LIBS += $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

SOURCES = ion-statusd.c exec.c extlrx.c

TARGETS = ion-statusd

LUA_SOURCES = statusd_mail.lua statusd_load.lua

MAKE_EXPORTS = statusd

include $(TOPDIR)/libmainloop/rx.mk

######################################

include $(TOPDIR)/rules.mk

######################################

ion-statusd: $(OBJS) $(EXT_OBJS)
	$(CC) $(LINKOPTS) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install: lc_install
	$(INSTALLDIR) $(EXTRABINDIR)
	$(INSTALL) -s -m $(BIN_MODE) ion-statusd $(EXTRABINDIR)
