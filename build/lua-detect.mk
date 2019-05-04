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

LUA_VERSION ?= $(shell pkg-config --exists lua5.2 && echo 5.2)
LUA_VERSION ?= $(shell pkg-config --exists lua5.1 && echo 5.1)
LUA_VERSION ?= $(shell pkg-config --exists lua    && echo 5.0)

ifeq ($(LUA_VERSION),)
    $(error Could not find any lua version. (Did you install the -dev package?))
endif

# prior to 5.1 the lib didn't include version in name.
ifeq ($(LUA_VERSION),5.0)
    LUA_VERSION=
endif

LUA_LIBS = $(shell pkg-config --libs lua$(LUA_VERSION))
LUA_INCLUDES = $(shell pkg-config --cflags lua$(LUA_VERSION))
LUA = $(shell which lua$(LUA_VERSION))
LUAC = $(shell which luac$(LUA_VERSION))

ifeq ($(LUA_LIBS),)
    $(error "pkg-config couldn't find linker flags for lua$(LUA_VERSION)!")
endif

ifeq ($(LUA_INCLUDES),)
    $(error "pkg-config couldn't find compiler flags for lua$(LUA_VERSION)!")
endif

ifeq ($(LUA),)
    $(error No lua$(LUA_VERSION) interpreter found!)
endif

ifeq ($(LUAC),)
    $(error No lua$(LUA_VERSION) compiler found!)
endif
