##
## Some make rules
##

######################################

ifdef MODULE
ifneq ($(STATIC_MODULES),1)
TARGETS := $(TARGETS) $(MODULE).so
else
TARGETS := $(TARGETS) $(MODULE).a
endif
endif


ifdef SUBDIRS

all: subdirs $(TARGETS)

clean: subdirs-clean _clean

realclean: subdirs-realclean _clean _realclean

depend: subdirs-depend _depend

else

all: $(TARGETS)

clean: _clean

realclean: _clean _realclean

depend: _depend

endif

ifdef INSTALL_SUBDIRS

install: subdirs-install _install

else

install: _install

endif

######################################

OBJS=$(subst .c,.o,$(SOURCES))

ifdef MAKE_EXPORTS

TO_CLEAN := $(TO_CLEAN) exports.c

OBJS := $(OBJS) exports.o

exports.c: $(SOURCES)
	$(PERL) $(TOPDIR)/mkexports.pl $(MAKE_EXPORTS) exports.c $(SOURCES)

endif

ifdef MODULE

ifneq ($(STATIC_MODULES),1)

$(MODULE).so: $(OBJS) $(EXT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(MODULE_LDFLAGS) $(OBJS) $(EXT_OBJS) -o $@

module_install:
	$(INSTALLDIR) $(MODULEDIR)
	$(INSTALL) -m $(BIN_MODE) $(MODULE).so $(MODULEDIR)
	# $(STRIP) $(MODULEDIR)/$(MODULE).so

else

$(MODULE).a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

module_install:

endif

.c.o:
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) -c $< -o $@

else

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

endif

_clean: 
	rm -f core $(DEPEND_FILE) $(OBJS) $(TO_CLEAN)

_realclean:
	rm -f $(TARGETS) $(TO_REALCLEAN)


ifdef SOURCES
_depend:
	$(MAKE_DEPEND) $(SOURCES)
else
_depend:
	
endif

######################################

subdirs:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

subdirs-depend:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i depend; done

subdirs-clean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done

subdirs-realclean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done

subdirs-install:
	set -e; for i in $(INSTALL_SUBDIRS); do $(MAKE) -C $$i install; done

######################################

ifeq ($(DEPEND_FILE),$(wildcard $(DEPEND_FILE)))
include $(DEPEND_FILE)
endif
