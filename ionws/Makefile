##
## Ion Makefile
##

# System-specific configuration is in system.mk
include ../system.mk

######################################

#DIST: LIBS += -L../libtu -ltu -lm $(X11_LIBS) -lX11
LIBS += -ltu -lm $(X11_LIBS) -lX11
#-lefence
#DIST: INCLUDES += -I../libtu/include $(X11_INCLUDES) -I..
INCLUDES += $(X11_INCLUDES) -I..
#DIST: DEFINES += -DETCDIR=\"$(ETCDIR)\"
DEFINES += -DETCDIR=\"$(ETCDIR)\" -DCF_PLACEMENT_GEOM

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

OBJS=	bindmaps.o conf.o draw.o frame-pointer.o frame.o		\
	funtabs.o main.o placement.o resize.o split.o splitframe.o	\
	winprops.o workspace.o confws.o

EXT_OBJS= ../query/query.a ../wmcore/wmcore.a

TARGETS=ion

######################################

include ../rules.mk

######################################

ion: $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(EXT_OBJS) $(LDFLAGS) -o $@

_install:
	$(INSTALLDIR) $(BINDIR)
	$(INSTALL) -m $(BIN_MODE) ion $(BINDIR)
	$(STRIP) $(BINDIR)/ion
