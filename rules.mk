##
## Some make rules
##

######################################

ifdef MODULE

TARGETS := $(TARGETS) $(MODULE).so

endif

ifdef SUBDIRS

all: subdirs $(TARGETS)

clean: subdirs-clean _clean

realclean: subdirs-realclean _realclean

depend: subdirs-depend _depend

else

all: $(TARGETS)

clean: _clean

realclean: _realclean

depend: _depend

endif

ifdef INSTALL_SUBDIRS

install: subdirs-install _install

else

install: _install

endif

######################################

ifdef MODULE

$(MODULE).so: $(OBJS) $(EXT_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(MODULE_LDFLAGS) $(OBJS) $(EXT_OBJS) -o $@

$(MODULE).a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

.c.o:
	$(CC) $(CFLAGS) $(MODULE_CFLAGS) -c $< -o $@

module_install:
	$(INSTALLDIR) $(MODULEDIR)
	$(INSTALL) -m $(BIN_MODE) $(MODULE).so $(MODULEDIR)
	$(STRIP) $(MODULEDIR)/$(MODULE).so

else

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

endif

ifdef OBJS

_clean: 
	rm -f core $(DEPEND_FILE) $(OBJS)

_depend:
	$(MAKE_DEPEND) *.c

else

_clean:

_depend:
	
endif

_realclean: _clean
	rm -f $(TARGETS)


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
