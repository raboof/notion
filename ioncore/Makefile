##
## wmcore Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

#DIST: LIBS += -L../libtu -ltu -lm $(X11_LIBS) -lX11
LIBS += -ltu -lm $(X11_LIBS) -lX11
#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
#DIST: DEFINES += -DETCDIR=\"$(ETCDIR)\"
DEFINES += -DETCDIR=\"$(ETCDIR)\" -DLIBDIR=\"$(LIBDIR)\"

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

OBJS=	binding.o conf-bindings.o conf-draw.o cursor.o draw.o event.o	\
	exec.o focus.o font.o function.o key.o modules.o mwmhints.o	\
	obj.o pointer.o property.o readconfig.o screen.o signal.o	\
	sizehint.o thing.o window.o wmcore.o xic.o selection.o		\
	completehelp.o symlist.o clientwin.o colormap.o region.o	\
	eventh.o winprops.o targetid.o attach.o resize.o defer.o grab.o \
	wsreg.o commandsq.o readfds.o regbind.o	funtabs.o viewport.o 	\
	tags.o names.o saveload.o


TARGETS=wmcore.a

######################################

include ../rules.mk

######################################

wmcore.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

_install:
