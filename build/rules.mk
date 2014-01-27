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

ifdef EXTRA_EXECUTABLE
EXECUTABLE := $(EXTRA_EXECUTABLE)
BINDIR_ := $(EXTRABINDIR)
endif

ifdef EXECUTABLE
BINDIR_ ?= $(BINDIR)
EXECUTABLE_ := $(EXECUTABLE)$(BIN_SUFFIX)
TARGETS := $(TARGETS) $(EXECUTABLE_)
endif


# Main targets
######################################

.PHONY: subdirs
.PHONY: subdirs-clean
.PHONY: subdirs-realclean
.PHONY: subdirs-depend
.PHONY: subdirs-install
.PHONY: _install
.PHONY: _depend
.PHONY: _exports

all: subdirs _exports $(TARGETS)

clean: subdirs-clean _clean

realclean: subdirs-realclean _clean _realclean

depend: subdirs-depend _depend

install: subdirs-install _install


ifdef MAKE_EXPORTS

# Exports
######################################

EXPORTS_C = exports.c
EXPORTS_H = exports.h

DEPEND_DEPENDS += $(EXPORTS_H)

TO_CLEAN := $(TO_CLEAN) $(EXPORTS_C) $(EXPORTS_H)

_exports: $(EXPORTS_C)

$(EXPORTS_H): $(EXPORTS_C)

$(EXPORTS_C): $(SOURCES) $(MKEXPORTS_EXTRA_DEPS)
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


ifdef EXECUTABLE

ifdef MODULE_LIST
ifdef MODULE_PATH
ifeq ($(PRELOAD_MODULES),1)
EXT_OBJS += $(foreach mod, $(MODULE_LIST), $(MODULE_PATH)/$(mod)/$(mod).a)
DEPEND_DEPENDS += preload.c
SOURCES += preload.c
TO_CLEAN += preload.c
else # !PRELOAD_MODULES
LDFLAGS += $(EXPORT_DYNAMIC)
WHOLEA = -Wl,-whole-archive
NO_WHOLEA = -Wl,-no-whole-archive
endif # !PRELOAD_MODULES

preload.c:
	$(LUA) $(TOPDIR)/build/mkpreload.lua $(MODULE_LIST) > preload.c

endif # MODULE_PATH
endif # MODULE_LIST

ifeq ($(RELOCATABLE),1)
DEFINES += -DCF_RELOCATABLE_BIN_LOCATION=\"$(BINDIR_)/$(EXECUTABLE)\"
endif

DEFINES += -DCF_EXECUTABLE=\"$(EXECUTABLE)\"

$(EXECUTABLE_): $(OBJS) $(EXT_OBJS)
	$(CC) $(OBJS) $(WHOLEA) $(EXT_OBJS) $(NO_WHOLEA) $(LDFLAGS) -o $@

executable_install:
	$(INSTALLDIR) $(DESTDIR)$(BINDIR_)
	$(INSTALLBIN) $(EXECUTABLE_) $(DESTDIR)$(BINDIR_)

endif # EXECUTABLE


ifdef MODULE

ifneq ($(PRELOAD_MODULES),1)

CC_PICFLAGS=-fPIC -DPIC
LD_SHAREDFLAGS=-shared

%.o: %.c
	$(CC) $(CC_PICFLAGS) $(CFLAGS) -c $< -o $@

# notion might not link to Xext, so modules will have to link to it themselves
# if they need it: 
LIBS += $(X11_LIBS)

$(MODULE).so: $(OBJS) $(EXT_OBJS)
	$(CC) $(LD_SHAREDFLAGS) $(LDFLAGS) $(OBJS) $(EXT_OBJS) $(LIBS) -o $@


module_install: module_stub_install
	$(INSTALLDIR) $(DESTDIR)$(MODULEDIR)
	$(INSTALLBIN) $(MODULE).so $(DESTDIR)$(MODULEDIR)

else # PRELOAD_MODULES

PICOPT=-fPIC -DPIC
LINKOPT=-shared

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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

else # !MODULE


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


endif# !MODULE


# Clean rules
######################################

_clean:
	$(RM) -f $(TO_CLEAN) core $(DEPEND_FILE) $(OBJS)

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

ifdef SOURCES

_depend: $(DEPEND_DEPENDS)
	$(MAKE_DEPEND)

ifeq ($(DEPEND_FILE),$(wildcard $(DEPEND_FILE)))
include $(DEPEND_FILE)
endif

endif

# Subdirectories
######################################

ifdef SUBDIRS

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

endif

# Localisation
######################################

TO_CLEAN += potfiles_c potfiles_lua

_potfiles:
	echo "$(SOURCES)"|tr ' ' '\n' > potfiles_c
	echo "$(LUA_SOURCES) $(ETC)"|tr ' ' '\n' > potfiles_lua

# Defaults
######################################

INSTALL_STRIP ?= -s
INSTALLBIN ?= $(INSTALL) $(INSTALL_STRIP) -m $(BIN_MODE)
