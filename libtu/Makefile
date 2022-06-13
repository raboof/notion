##
## Tu Makefile
##

# System-specific configuration is in system.mk
ifeq ($(MAKELEVEL),0)
TOPDIR=.
else
TOPDIR=..
endif

# System-specific configuration
include $(TOPDIR)/build/system-inc.mk

######################################

#INCLUDES += $(LIBTU_INCLUDES) $(LUA_INCLUDES)

CFLAGS += $(C89_SOURCE) $(POSIX_SOURCE) $(WARN)

SOURCES=iterable.c  map.c  misc.c  obj.c  objlist.c  optparser.c  output.c  parser.c  prefix.c  ptrlist.c  rb.c  setparam.c  stringstore.c  tokenizer.c  util.c errorlog.c

HEADERS=debug.h  errorlog.h  locale.h  minmax.h  obj.h      objp.h       output.h  pointer.h  private.h  rb.h        stringstore.h  types.h dlist.h  iterable.h  map.h     misc.h    objlist.h  optparser.h  parser.h  prefix.h   ptrlist.h  setparam.h  tokenizer.h    util.h

TARGETS=libtu.a

SUBDIRS = test

######################################

include $(TOPDIR)/build/rules.mk

######################################

.PHONY: test

libtu.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+

install:
	$(INSTALLDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m $(DATA_MODE) libtu.a $(DESTDIR)$(LIBDIR)
	$(INSTALLDIR) $(DESTDIR)$(INCDIR)/libtu
	for h in $(HEADERS); do \
		$(INSTALL) -m $(DATA_MODE) $$h $(DESTDIR)$(INCDIR)/libtu; \
	done

test:
	$(MAKE) -C test test
