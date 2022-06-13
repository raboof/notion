##
## Notioncore Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(X11_INCLUDES) $(LIBTU_INCLUDES) $(LIBEXTL_INCLUDES) -I..

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=binding.c conf-bindings.c cursor.c event.c exec.c focus.c         \
	strings.c key.c modules.c mwmhints.c pointer.c property.c         \
	screen.c sizehint.c window.c ioncore.c                   	  \
	xic.c selection.c clientwin.c colormap.c region.c eventh.c        \
	attach.c resize.c grab.c manage.c regbind.c    			  \
	rootwin.c tags.c names.c saveload.c frame.c                       \
	frame-pointer.c conf.c reginfo.c extlconv.c fullscreen.c mplex.c  \
	bindmaps.c gr.c infowin.c activity.c netwm.c frame-draw.c         \
	kbresize.c rectangle.c xwindow.c presize.c extlrx.c               \
        pholder.c mplexpholder.c llist.c basicpholder.c sizepolicy.c      \
        stacking.c group.c grouppholder.c group-cw.c navi.c		  \
        group-ws.c float-placement.c groupedpholder.c framedpholder.c	  \
        return.c detach.c screen-notify.c frame-tabs-recalc.c profiling.c \
        log.c tempdir.c

LUA_SOURCES=\
	ioncore_ext.lua ioncore_luaext.lua ioncore_bindings.lua \
	ioncore_winprops.lua ioncore_misc.lua ioncore_efbb.lua \
	ioncore_wd.lua ioncore_menudb.lua

ifeq ($(PRELOAD_MODULES),1)
CFLAGS += -DCF_PRELOAD_MODULES
endif

MAKE_EXPORTS=ioncore

TARGETS=ioncore.a

include $(TOPDIR)/libmainloop/rx.mk

######################################

include $(TOPDIR)/build/rules.mk

######################################

# without -finstrument-functions
profiling.o: profiling.c
	$(CC) $(CPPFLAGS) $(CFLAGS:-finstrument-functions=) -c $< -o $@

ioncore.o: exports.h

ioncore.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+

_install: lc_install
