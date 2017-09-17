# Find highest known lua version.
#
# It uses pkg-config to do this, but will fail if you have liblua, but
# not the corresponding interpreter/compiler. Let's say you have liblua5.2
# but want to build with liblua5.1 (for which you have the lib, interpreter
# and compiler), you can override by setting LUA_VERSION=5.0 when invoking
# make.
#
# If successful, sets the following variables:
#  * LUA_VERSION (unless already set)
#  * LUA_LIBS (can be appended to LDFLAGS directly)
#  * LUA_INCLUDES (can be appended to CFLAGS directly)
#  * LUA (full path to lua interpreter)
#  * LUAC (full path to lua compiler)

LUA_VERSIONS_CANDIDATES = $(or $(LUA_VERSION),5.2 5.1 5.0)

LUA_PKG := $(firstword $(foreach ver,$(LUA_VERSIONS_CANDIDATES),$(shell \
       ($(PKG_CONFIG) --exists lua-$(ver)     && echo lua-$(ver)) \
    || ($(PKG_CONFIG) --exists lua$(ver:5.0=) && echo lua$(ver:5.0=)))))

ifeq ($(LUA_PKG),)
    $(error Could not find $(or $(LUA_VERSION),any) lua version. (Did you install the -dev package?))
endif

ifeq ($(LUA_VERSION),)
LUA_VERSION := $(or $(shell $(PKG_CONFIG) --variable=V $(LUA_PKG)),5.0)
endif

# prior to 5.1 the lib didn't include version in name.
LUA_SUFFIX := $(if $(findstring $(LUA_VERSION),5.0),,$(LUA_VERSION))

LUA_LIBS     := $(or $(shell $(PKG_CONFIG) --libs $(LUA_PKG)), $(error "pkg-config couldn't find linker flags for lua$(LUA_SUFFIX)!"))
LUA_INCLUDES := $(shell $(PKG_CONFIG) --cflags $(LUA_PKG))
LUA          := $(or $(shell which lua$(LUA_SUFFIX)),          $(shell which lua),  $(error No lua$(LUA_SUFFIX) interpreter found!))
LUAC         := $(or $(shell which luac$(LUA_SUFFIX)),         $(shell which luac), $(error No lua$(LUA_SUFFIX) compiler found!))
