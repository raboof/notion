##
## Ion Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

SOURCES=ion.c

TARGETS=ion3

INCLUDES += $(X11_INCLUDES) 
INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES)
INCLUDES += -I..

LIBS += $(X11_LIBS)
LIBS += $(WHOLEA) $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(NO_WHOLEA)
LIBS += $(LUA_LIBS) $(DL_LIBS)
LIBS += -lm

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
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\" \
	   -DION3_LOCATION=\"$(BINDIR)/ion3\"

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

######################################

include $(TOPDIR)/build/rules.mk

######################################

ion3: $(OBJS) $(EXT_OBJS)
	$(CC) $(LINKOPTS) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) $(LDFLAGS) -o $@

preload.c:
	$(LUA) ../build/mkpreload.lua $(MODULE_LIST) > preload.c

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ion3 $(BINDIR)
