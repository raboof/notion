
## Ioncore Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: LIBS += -L../libtu -ltu -lm $(X11_LIBS) $(XINERAMA_LIBS)
LIBS += -ltu -lm $(X11_LIBS) $(XINERAMA_LIBS)
#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
DEFINES += -DETCDIR=\"$(ETCDIR)\" -DSHAREDIR=\"$(SHAREDIR)\" \
           -DEXTRABINDIR=\"$(EXTRABINDIR)\" -DMODULEDIR=\"$(MODULEDIR)\"
CFLAGS += $(XOPEN_SOURCE)

# libtool
INCLUDES += $(LTDL_INCLUDES)
LIBS += $(LTDL_LIBS)
CFLAGS += $(LTDL_CFLAGS)
ifeq ($(PRELOAD_MODULES),1)
LDFLAGS += $(foreach mod, $(MODULE_LIST), -dlpreopen ../$(mod)/$(mod).la)
CFLAGS += -DCF_PRELOAD_MODULES
LINKOPTS = -R$(MODULEDIR)
else
LINKOPTS = -export-dynamic -R$(MODULEDIR)
endif

# Lua support setup
EXT_OBJS := $(EXT_OBJS) ../luaextl/luaextl.a
ICNLUDES += $(LUA_INCLUDES)
LIBS += $(LUA_LIBS)

SOURCES=binding.c conf-bindings.c cursor.c event.c     \
	exec.c focus.c strings.c key.c modules.c mwmhints.c obj.c pointer.c  \
	property.c readconfig.c screen.c signal.c sizehint.c window.c     \
	ioncore.c xic.c selection.c symlist.c clientwin.c colormap.c      \
	region.c eventh.c  attach.c resize.c defer.c grab.c     	  \
	manage.c readfds.c regbind.c rootwin.c tags.c names.c saveload.c  \
	genws.c genframe.c genframe-pointer.c conf.c reginfo.c errorlog.c \
	stacking.c extlconv.c fullscreen.c mplex.c bindmaps.c gr.c \
	infowin.o

MAKE_EXPORTS=ioncore

TARGETS=ioncore

######################################

include $(TOPDIR)/rules.mk

######################################

ioncore: $(OBJS) $(EXT_OBJS)
	$(LIBTOOL) $(CC) $(LINKOPTS) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ioncore $(BINDIR)
	# $(STRIP) $(BINDIR)/ioncore

