##
## PWM Makefile
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
LIBS += $(DL_LIBS)
EXT_OBJS += ../ioncore/ioncore.a ../libextl/libextl.a
#DIST: EXT_OBJS += ../libtu/libtu.a
LIBS += -ltu

DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
	   -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\" \
	   -DLCDIR=\"$(LCDIR)\" -DLOCALEDIR=\"$(LOCALEDIR)\"

ifndef PWM_ETCDIR
PWM_ETCDIR = $(ETCDIR)
else
DEFINES += -DPWM_ETCDIR=\"$(PWM_ETCDIR)\"
endif

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=pwm.c

ifeq ($(PRELOAD_MODULES),1)
EXT_OBJS += $(foreach mod, $(PWM_MODULE_LIST), ../$(mod)/$(mod).a)
DEPEND_DEPENDS += preload.c
SOURCES += preload.c
TO_CLEAN += preload.c
LIBS += -lSM -lICE
else
LINKOPTS = -export-dynamic
endif

ETC = cfg_pwm.lua cfg_pwm_menus.lua cfg_pwm_bindings.lua
TARGETS = pwm3

######################################

include $(TOPDIR)/rules.mk

######################################

pwm3: $(OBJS) $(EXT_OBJS)
	$(CC) $(LINKOPTS) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

preload.c:
	$(LUA) ../build/mkpreload.lua $(PWM_MODULE_LIST) > preload.c

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) pwm3 $(BINDIR)
	$(INSTALLDIR) $(PWM_ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(PWM_ETCDIR); \
	done
