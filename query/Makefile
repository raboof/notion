##
## query Makefile
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
DEFINES += -DETCDIR=\"$(ETCDIR)\"

CFLAGS += $(XOPEN_SOURCE) $(MODULE_SUPPORT)
LDFLAGS += $(MODULE_SUPPORT_LDFLAGS)

OBJS=	complete.o complete_file.o edln.o input.o listing.o main.o	\
	wedln.o wmessage.o query.o fwarn.o

TARGETS=query.a

######################################

include ../rules.mk

######################################

query.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

_install:
