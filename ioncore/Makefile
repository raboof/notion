##
## Ioncore Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES)
INCLUDES += $(LTDL_INCLUDES)
#DIST: INCLUDES += -I../libtu/include -I..
INCLUDES += -I..

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=binding.c conf-bindings.c cursor.c event.c exec.c focus.c         \
	strings.c key.c modules.c mwmhints.c pointer.c property.c         \
	readconfig.c screen.c signal.c sizehint.c window.c ioncore.c      \
	xic.c selection.c clientwin.c colormap.c region.c eventh.c        \
	attach.c resize.c defer.c grab.c manage.c mainloop.c regbind.c    \
	rootwin.c tags.c names.c saveload.c genws.c frame.c               \
	frame-pointer.c conf.c reginfo.c extlconv.c fullscreen.c mplex.c  \
	bindmaps.c gr.c infowin.c activity.c netwm.c frame-draw.c         \
	kbresize.c rectangle.c xwindow.c hooks.c

ifeq ($(PRELOAD_MODULES),1)
CFLAGS += -DCF_PRELOAD_MODULES
endif

MAKE_EXPORTS=ioncore

TARGETS=ioncore.a

######################################

include $(TOPDIR)/rules.mk

######################################

ioncore.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

_install:

