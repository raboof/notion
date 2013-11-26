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

LUA_VERSION := $(or $(LUA_VERSION), $(shell \
       (pkg-config --exists lua5.2 && echo 5.2) \
    || (pkg-config --exists lua5.1 && echo 5.1) \
    || (pkg-config --exists lua    && echo 5.0)))

ifeq ($(LUA_VERSION),)
    $(error Could not find any lua version. (Did you install the -dev package?))
endif

# prior to 5.1 the lib didn't include version in name.
ifeq ($(LUA_VERSION),5.0)
    LUA_VERSION=
endif

LUA_LIBS     := $(or $(shell pkg-config --libs lua$(LUA_VERSION)),   $(error "pkg-config couldn't find linker flags for lua$(LUA_VERSION)!"))
LUA_INCLUDES := $(shell pkg-config --cflags lua$(LUA_VERSION))
LUA          := $(or $(shell which lua$(LUA_VERSION)),               $(error No lua$(LUA_VERSION) interpreter found!))
LUAC         := $(or $(shell which luac$(LUA_VERSION)),              $(error No lua$(LUA_VERSION) compiler found!))
