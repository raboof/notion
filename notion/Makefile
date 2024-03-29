##
## Notion Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

LD_START_GROUP := -Wl,--start-group
LD_END_GROUP := -Wl,--end-group

SOURCES=notion.c

TARGETS=notion

INCLUDES += $(X11_INCLUDES)
INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES)
INCLUDES += -I..

LIBS += $(LD_START_GROUP) $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(LD_END_GROUP)
LIBS += $(X11_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm

ifeq ($(PRELOAD_MODULES),1)
EXT_OBJS += $(foreach mod, $(MODULE_LIST), ../$(mod)/$(mod).a)
DEPEND_DEPENDS += preload.c
SOURCES += preload.c
TO_CLEAN += preload.c
LIBS += -lSM -lICE
else
LINKOPTS = $(EXPORT_DYNAMIC)
WHOLEA = -Wl,-whole-archive
NO_WHOLEA = -Wl,-no-whole-archive
endif

EXT_OBJS += ../ioncore/ioncore.a

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
           -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

######################################

include $(TOPDIR)/build/rules.mk

######################################

notion: $(OBJS) $(EXT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LINKOPTS) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) -o $@ $(LIBS)

preload.c:
	$(LUA) ../build/mkpreload.lua $(MODULE_LIST) > preload.c

_install:
	$(INSTALLDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) $(INSTALL_STRIP) -m $(BIN_MODE) notion $(DESTDIR)$(BINDIR)
