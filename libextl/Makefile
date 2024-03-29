##
## Extl Makefile
##

ifeq ($(MAKELEVEL),0)
TOPDIR=.
else
TOPDIR=..
endif

# System-specific configuration
include $(TOPDIR)/build/system-inc.mk

######################################

INCLUDES += $(LIBTU_INCLUDES) $(LUA_INCLUDES)

CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=readconfig.c luaextl.c misc.c

LIBS += $(LIBTU_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm

HEADERS=readconfig.h extl.h luaextl.h private.h types.h

TARGETS=libextl.a libextl-mkexports

SUBDIRS = test

.PHONY : libextl-mkexports test

######################################

include $(TOPDIR)/build/rules.mk

######################################

ifdef LUA

libextl.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+

libextl-mkexports: libextl-mkexports.in
	sed "1s:LUA50:$(LUA):" $< > $@

else

libextl.a: libextl-mkexports.in
	echo "Error: LUA interpreter and libraries not found (or inconsistent versions)"
	return -1

libextl-mkexports: libextl-mkexports.in
	echo "Error: LUA interpreter and libraries not found (or inconsistent versions)"
	return -1

endif

install:
	$(INSTALLDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m $(BIN_MODE) libextl-mkexports $(DESTDIR)$(BINDIR)
	$(INSTALLDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m $(DATA_MODE) libextl.a $(DESTDIR)$(LIBDIR)
	$(INSTALLDIR) $(DESTDIR)$(INCDIR)
	for h in $(HEADERS); do \
		$(INSTALL) -m $(DATA_MODE) $$h $(DESTDIR)$(INCDIR); \
	done

test:
	$(MAKE) -C test test
