
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
DEFINES += -DETCDIR=\"$(ETCDIR)\" -DLIBDIR=\"$(LIBDIR)\"

ifeq ($(STATIC_MODULES),1)
DEFINES += -DCF_STATIC_MODULES
EXT_OBJS := $(foreach mod, $(MODULE_LIST), ../$(mod)/$(mod).a)
TO_CLEAN := $(TO_CLEAN) static-modules.h
endif

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT_CFLAGS)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

# Lua support setup
EXT_OBJS := $(EXT_OBJS) ../luaextl/luaextl.a
LIBS := $(LIBS) $(LUA_LIBS)

SOURCES=binding.c conf-bindings.c conf-draw.c cursor.c draw.c event.c	\
	exec.c focus.c font.c key.c modules.c mwmhints.c	\
	obj.c pointer.c property.c readconfig.c screen.c signal.c	\
	sizehint.c window.c ioncore.c xic.c selection.c			\
	completehelp.c symlist.c clientwin.c colormap.c region.c	\
	eventh.c winprops.c targetid.c attach.c resize.c defer.c grab.c \
	wsreg.c readfds.c regbind.c viewport.c 				\
	tags.c names.c saveload.c genws.c genframe.c genframe-pointer.c	\
	conf.c reginfo.c

MAKE_EXPORTS=ioncore

TARGETS=ioncore

######################################

include $(TOPDIR)/rules.mk

######################################

ioncore: $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

ifeq ($(STATIC_MODULES),1)

modules.c: static-modules.h

.PHONY: static-modules.h

static-modules.h:
	for i in $(MODULE_LIST); do \
		echo "#include \"../$$i/main.h\""; \
	done > static-modules.h; \
	echo "static StatModInfo available_modules[]={" >> static-modules.h; \
	for i in $(MODULE_LIST); do \
		echo "{\"$$i\", $${i}_module_init, $${i}_module_deinit, FALSE},"; \
	done >> static-modules.h; \
	echo "{NULL, NULL, NULL, FALSE}," >> static-modules.h;
	echo "};" >> static-modules.h
endif

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ioncore $(BINDIR)
	# $(STRIP) $(BINDIR)/ioncore

