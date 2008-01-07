##
## PWM Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

EXECUTABLE = pwm3

SOURCES = pwm.c

ETC = cfg_pwm.lua

INCLUDES += $(X11_INCLUDES) 
INCLUDES += $(LIBMAINLOOP_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES)
INCLUDES += -I..

LIBS += $(X11_LIBS) -lSM -lICE
LIBS += $(WHOLEA) $(LIBMAINLOOP_LIBS) $(LIBEXTL_LIBS) $(LIBTU_LIBS) $(NO_WHOLEA)
LIBS += $(LUA_LIBS) $(DL_LIBS)
LIBS += -lm

MODULE_PATH = $(TOPDIR)

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

_install: executable_install
	$(INSTALLDIR) $(PWM_ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(PWM_ETCDIR); \
	done
