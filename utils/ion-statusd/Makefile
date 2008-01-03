##
## Ion-statusd Makefile
##

# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBEXTL_INCLUDES) $(LIBTU_INCLUDES)
LIBS += $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"
           
SOURCES = ion-statusd.c exec.c extlrx.c

TARGETS = ion-statusd

LUA_SOURCES = statusd_date.lua statusd_mail.lua statusd_load.lua

MAKE_EXPORTS = statusd

ifeq ($(RELOCATABLE),1)
DEFINES += -DCF_RELOCATABLE_STATUSD_LOCATION=\"$(EXTRABINDIR)/ion-statusd\"
endif

include $(TOPDIR)/libmainloop/rx.mk

######################################

include $(TOPDIR)/build/rules.mk

######################################

ion-statusd: $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install: lc_install
	$(INSTALLDIR) $(EXTRABINDIR)
	$(INSTALLBIN) ion-statusd $(EXTRABINDIR)
