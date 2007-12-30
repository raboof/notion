##
## PWM Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

SOURCES=pwm.c

ETC = cfg_pwm.lua

TARGETS = pwm3

INCLUDES += $(X11_INCLUDES) 
INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES)
INCLUDES += -I..

LIBS += $(X11_LIBS)
LIBS += $(WHOLEA) $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(NO_WHOLEA)
LIBS += $(LUA_LIBS) $(DL_LIBS)
LIBS += -lm

ifeq ($(PRELOAD_MODULES),1)
EXT_OBJS += $(foreach mod, $(PWM_MODULE_LIST), ../$(mod)/$(mod).a)
DEPEND_DEPENDS += preload.c
SOURCES += preload.c
TO_CLEAN += preload.c
LIBS += -lSM -lICE
else
LDFLAGS += $(EXPORT_DYNAMIC)
WHOLEA = -Wl,-whole-archive
NO_WHOLEA = -Wl,-no-whole-archive
endif

ifeq ($(RELOCATABLE),1)
DEFINES += -DCF_RELOCATABLE_PWM3_LOCATION=\"$(BINDIR)/pwm3\"
endif

EXT_OBJS += ../ioncore/ioncore.a

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
	   -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

ifndef PWM_ETCDIR
PWM_ETCDIR = $(ETCDIR)
else
DEFINES += -DPWM_ETCDIR=\"$(PWM_ETCDIR)\"
endif

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

######################################

include $(TOPDIR)/build/rules.mk

######################################

pwm3: $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) $(LDFLAGS) -o $@

preload.c:
	$(LUA) ../build/mkpreload.lua $(PWM_MODULE_LIST) > preload.c

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALLBIN) pwm3 $(BINDIR)
	$(INSTALLDIR) $(PWM_ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(PWM_ETCDIR); \
	done
