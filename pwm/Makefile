##
## PWM Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

# List of modules to possibly preload
include $(TOPDIR)/modulelist.mk

######################################

ifndef PWM_ETCDIR
PWM_ETCDIR = $(ETCDIR)/pwm
endif

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
LDFLAGS += $(foreach mod, $(PWM_MODULE_LIST), -dlpreopen ../$(mod)/$(mod).la)
CFLAGS += -DCF_PRELOAD_MODULES
LINKOPTS = -R$(MODULEDIR)
else
LINKOPTS = -export-dynamic -R$(MODULEDIR)
endif

EXT_OBJS += ../ioncore/ioncore.a ../luaextl/luaextl.a

SOURCES=pwm.c

TARGETS=pwm

######################################

include $(TOPDIR)/rules.mk

######################################

pwm: $(OBJS) $(EXT_OBJS)
	$(LIBTOOL) --mode=link $(CC) $(LINKOPTS) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) pwm $(BINDIR)
	# $(STRIP) $(BINDIR)/pwm
	$(INSTALLDIR) $(PWM_ETCDIR)
	$(INSTALL) -m $(DATA_MODE) ioncore.lua $(PWM_ETCDIR)
	$(INSTALLDIR) $(MANDIR)/man1
	$(INSTALL) -m $(DATA_MODE) pwm.1 $(MANDIR)/man1
