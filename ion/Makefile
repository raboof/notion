##
## Ion Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

INCLUDES += $(X11_INCLUDES)
#DIST: INCLUDES += -I.. -I../libtu/include 
INCLUDES += -I..

LIBS += -lm
LIBS += $(X11_LIBS) $(XINERAMA_LIBS)
LIBS += $(LUA_LIBS)
LIBS += $(LTDL_LIBS)
EXT_OBJS += ../ioncore/ioncore.a ../luaextl/luaextl.a
#DIST: EXT_OBJS += ../libtu/libtu.a
LIBS += -ltu

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\"

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

ifeq ($(PRELOAD_MODULES),1)
LDFLAGS += $(foreach mod, $(MODULE_LIST), -dlpreopen ../$(mod)/$(mod).la)
LINKOPTS = -R$(MODULEDIR)
else
LINKOPTS = -export-dynamic -R$(MODULEDIR)
endif

SOURCES=ion.c

TARGETS=ion

######################################

include $(TOPDIR)/rules.mk

######################################

ion: $(OBJS) $(EXT_OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LINKOPTS) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ion $(BINDIR)
