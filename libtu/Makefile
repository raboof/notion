##
## Tu Makefile
##

# System-specific configuration is in system.mk
ifeq ($(MAKELEVEL),0)
TOPDIR=.
else
TOPDIR=..
endif
include $(TOPDIR)/build/system-inc.mk

######################################

#INCLUDES += $(LIBTU_INCLUDES) $(LUA_INCLUDES)

CFLAGS += $(C98_SOURCE) $(POSIX_SOURCE) $(WARN)

SOURCES=iterable.c  map.c  misc.c  obj.c  objlist.c  optparser.c  output.c  parser.c  prefix.c  ptrlist.c  rb.c  setparam.c  stringstore.c  tokenizer.c  util.c errorlog.c

HEADERS=debug.h  errorlog.h  locale.h  minmax.h  obj.h      objp.h       output.h  pointer.h  private.h  rb.h        stringstore.h  types.h dlist.h  iterable.h  map.h     misc.h    objlist.h  optparser.h  parser.h  prefix.h   ptrlist.h  setparam.h  tokenizer.h    util.h

TARGETS=libtu.a 

######################################

include $(TOPDIR)/build/rules.mk

######################################

libtu.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

install:
	$(INSTALLDIR) $(LIBDIR)
	$(INSTALL) -m $(DATA_MODE) libtu.a $(LIBDIR)
	$(INSTALLDIR) $(INCDIR)/libtu
	for h in $(HEADERS); do \
		$(INSTALL) -m $(DATA_MODE) $$h $(INCDIR)/libtu; \
	done
