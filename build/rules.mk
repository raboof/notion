##
## Some make rules
##

ifdef MODULE
ifeq ($(PRELOAD_MODULES),1)
MODULE_TARGETS := $(MODULE).a $(MODULE).lc
else
MODULE_TARGETS := $(MODULE).so $(MODULE).lc
endif
TARGETS := $(TARGETS) $(MODULE_TARGETS)
endif

ifdef LUA_SOURCES
LUA_COMPILED := $(subst .lua,.lc, $(LUA_SOURCES))
TARGETS := $(TARGETS) $(LUA_COMPILED)
endif


# Main targets
######################################

.PHONY: subdirs
.PHONY: subdirs-clean
.PHONY: subdirs-realclean
.PHONY: subdirs-install
.PHONY: _install
.PHONY: _exports

all: subdirs _exports $(TARGETS)

clean: subdirs-clean _clean

realclean: subdirs-realclean _clean _realclean

install: subdirs-install _install


ifdef MAKE_EXPORTS

# Exports
######################################

EXPORTS_C = exports.c
EXPORTS_H = exports.h

TO_CLEAN := $(TO_CLEAN) $(EXPORTS_C) $(EXPORTS_H)

_exports: $(EXPORTS_C)

# this funny syntax (more than one pattern-based target) is meant to tell Make
# that this rule makes BOTH of these targets. Look at the last paragraph of
# http://www.gnu.org/software/make/manual/html_node/Pattern-Intro.html
%xports.c %xports.h: $(SOURCES) $(MKEXPORTS_EXTRA_DEPS)
	$(MKEXPORTS) -module $(MAKE_EXPORTS) -o $(EXPORTS_C) -h $(EXPORTS_H) \
	$(SOURCES) $(MKEXPORTS_EXTRAS)

# Exports documentation
######################################

EXPORTS_DOC = exports.tex

TO_CLEAN := $(TO_CLEAN) $(EXPORTS_DOC)

_exports_doc: $(EXPORTS_DOC)

$(EXPORTS_DOC): $(SOURCES) $(LUA_SOURCES) $(MKEXPORTS_EXTRA_DEPS)
	$(MKEXPORTS) -mkdoc -module $(MAKE_EXPORTS) -o $(EXPORTS_DOC) \
	$(SOURCES) $(LUA_SOURCES) $(MKEXPORTS_EXTRAS)

else # !MAKE_EXPORTS

EXPORTS_C =
EXPORTS_H =
EXPORTS_DOC =

endif # !MAKE_EXPORTS


# Compilation and linking
######################################

OBJS=$(subst .c,.o,$(SOURCES) $(EXPORTS_C))

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ifdef MODULE

ifneq ($(PRELOAD_MODULES),1)

CFLAGS += -fPIC -DPIC
LD_SHAREDFLAGS=-shared

# notion might not link to Xext, so modules will have to link to it themselves
# if they need it:
LIBS += $(X11_LIBS)

$(MODULE).so: $(OBJS) $(EXT_OBJS)
	$(CC) $(LD_SHAREDFLAGS) $(LDFLAGS) $(OBJS) $(EXT_OBJS) $(LIBS) -o $@


module_install: module_stub_install
	$(INSTALLDIR) $(MODULEDIR)
	$(INSTALL) -m $(BIN_MODE) $(MODULE).so $(MODULEDIR)

else # PRELOAD_MODULES

PICOPT=-fPIC -DPIC
LINKOPT=-shared

$(MODULE).a: $(OBJS) $(EXT_OBJS)
	$(AR) $(ARFLAGS) $@ $+
	$(RANLIB) $@

module_install: module_stub_install

endif # PRELOAD_MODULES

module_stub_install:
	$(INSTALLDIR) $(DESTDIR)$(LCDIR)
	$(INSTALL) -m $(DATA_MODE) $(MODULE).lc $(DESTDIR)$(LCDIR)

ifndef MODULE_STUB

$(MODULE).lc:
	echo "ioncore.load_module('$(MODULE)') package.loaded['$(MODULE)']=true" | $(LUAC) -o $@ -
else

LUA_SOURCES += $(MODULE_STUB)

endif #MODULE_STUB

endif# !MODULE


# Clean rules
######################################

_clean:
	$(RM) -f $(TO_CLEAN) core *.d *.lc $(OBJS)

_realclean:
	$(RM) -f $(TO_REALCLEAN) $(TARGETS)

# Lua rules
######################################

%.lc: %.lua
	$(LUAC) -o $@ $<

lc_install:
	$(INSTALLDIR) $(DESTDIR)$(LCDIR)
	for i in $(LUA_COMPILED); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(LCDIR); \
	done

etc_install:
	$(INSTALLDIR) $(DESTDIR)$(ETCDIR)
	for i in $(ETC); do \
		$(INSTALL) -m $(DATA_MODE) $$i $(DESTDIR)$(ETCDIR); \
	done

# Dependencies
######################################

CFLAGS += -MMD
-include *.d

# Subdirectories
######################################

ifdef SUBDIRS

subdirs:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i; done

subdirs-clean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i clean; done

subdirs-realclean:
	set -e; for i in $(SUBDIRS); do $(MAKE) -C $$i realclean; done

subdirs-install:
	set -e; for i in $(INSTALL_SUBDIRS); do $(MAKE) -C $$i install; done

endif

# Localisation
######################################

TO_CLEAN += potfiles_c potfiles_lua

_potfiles:
	echo "$(SOURCES)"|tr ' ' '\n' > potfiles_c
	echo "$(LUA_SOURCES) $(ETC)"|tr ' ' '\n' > potfiles_lua
