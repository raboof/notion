
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
CLEAN_TARGETS := static-modules-clean
endif

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT_CFLAGS)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

OBJS=	binding.o conf-bindings.o conf-draw.o cursor.o draw.o event.o	\
	exec.o focus.o font.o function.o key.o modules.o mwmhints.o	\
	obj.o pointer.o property.o readconfig.o screen.o signal.o	\
	sizehint.o window.o ioncore.o xic.o selection.o			\
	completehelp.o symlist.o clientwin.o colormap.o region.o	\
	eventh.o winprops.o targetid.o attach.o resize.o defer.o grab.o \
	wsreg.o commandsq.o readfds.o regbind.o	funtabs.o viewport.o 	\
	tags.o names.o saveload.o genws.o genframe.o genframe-pointer.o	\
	conf.o conf-winprops.o reginfo.o

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

static-modules-clean:
	rm -f static-modules.h

endif

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ioncore $(BINDIR)
	# $(STRIP) $(BINDIR)/ioncore
