
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

#DIST: LIBS += -L../libtu -ltu -lm
LIBS += -ltu -lm
LIBS += $(X11_LIBS) $(XINERAMA_LIBS)
LIBS += $(LUA_LIBS)
LIBS += $(LTDL_LIBS)

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\"

CFLAGS += $(XOPEN_SOURCE)

ifeq ($(PRELOAD_MODULES),1)
LDFLAGS += $(foreach mod, $(MODULE_LIST), -dlpreopen ../$(mod)/$(mod).la)
CFLAGS += -DCF_PRELOAD_MODULES
LINKOPTS = -R$(MODULEDIR)
else
LINKOPTS = -export-dynamic -R$(MODULEDIR)
endif

EXT_OBJS += ../ioncore/ioncore.a ../luaextl/luaextl.a

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
	# $(STRIP) $(BINDIR)/ion
