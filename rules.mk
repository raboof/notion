##
## Some make rules
##


# Main targets
######################################

ifdef MODULE
ifeq ($(PRELOAD_MODULES),1)
MODULE_TARGETS := $(ODULE).a $(MODULE).lc
else
MODULE_TARGETS := $(MODULE).so $(MODULE).lc
endif
TARGETS := $(TARGETS) $(MODULE_TARGETS)
endif


ifdef LUA_SOURCES
LUA_COMPILED := $(subst .lua,.lc, $(LUA_SOURCES))
TARGETS := $(TARGETS) $(LUA_COMPILED)
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


# Exports
######################################

ifdef MAKE_EXPORTS

TO_CLEAN := $(TO_CLEAN) exports.c

EXPORTS_C = exports.c

exports.c: $(SOURCES)
	$(LUA) $(TOPDIR)/mkexports.lua -module $(MAKE_EXPORTS) -o exports.c $(SOURCES)

else # !MAKE_EXPORTS

EXPORTS_C = 

endif # !MAKE_EXPORTS


# Compilation and linking
######################################

OBJS=$(subst .c,.o,$(SOURCES) $(EXPORTS_C))


ifdef MODULE

ifneq ($(PRELOAD_MODULES),1)

CC_PICFLAGS=-fPIC -DPIC
LD_SHAREDFLAGS=-shared

%.o: %.c
	$(CC) $(CC_PICFLAGS) $(CFLAGS) -c $< -o $@

$(MODULE).so: $(OBJS) $(EXT_OBJS)
	$(CC) $(LD_SHAREDFLAGS) $(LDFLAGS) $(OBJS) $(EXT_OBJS) -o $@


module_install: module_stub_install
	$(INSTALLDIR) $(MODULEDIR)
	$(INSTALL) -m $(BIN_MODE) $(MODULE).so $(MODULEDIR)

else # PRELOAD_MODULES

PICOPT=-fPIC -DPIC
LINKOPT=-shared

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(MODULE).a: $(OBJS) $(EXT_OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

$(MODULE).lc: $(MODULE).a
	echo "ioncore.load_module('$(MODULE)')" | $(LUAC) -o $@ -

module_install: module_stub_install

endif # PRELOAD_MODULES

module_stub_install:
	$(INSTALLDIR) $(LCDIR)
	$(INSTALL) -m $(DATA_MODE) $(MODULE).lc $(LCDIR)

ifndef MODULE_STUB

$(MODULE).lc: $(MODULE).so
	echo "ioncore.load_module('$(MODULE)')" \
	| $(LUAC) -o $@ -
else
	
$(MODULE).lc: $(MODULE_STUB)
	$(LUAC) -o $@ $(MODULE_STUB)

endif #MODULE_STUB

else # !MODULE


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


endif# !MODULE


# Clean rules
######################################

ifdef SOURCES

clean_objs:
	$(RM) -f $(OBJS)

else #!SOURCES

clean_objs:

endif #!SOURCES

clean_target:
	$(RM) -f $(TARGETS)

_clean: clean_objs
	$(RM) -f core $(DEPEND_FILE) $(TO_CLEAN)

_realclean: clean_target
	$(RM) -f $(TO_REALCLEAN)

# Lua rules
######################################

%.lc: %.lua
	$(LUAC) -o $@ $<


# Dependencies
######################################

ifdef SOURCES

_depend:
	$(MAKE_DEPEND)

else #!SOURCES

_depend:
	
endif #!SOURCES


# Subdirectories
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


# Dependencies
######################################

ifeq ($(DEPEND_FILE),$(wildcard $(DEPEND_FILE)))
include $(DEPEND_FILE)
endif
