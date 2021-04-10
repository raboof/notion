# Find highest known lua version.
#
# It uses pkg-config to do this, but will fail if you have liblua, but
# not the corresponding interpreter/compiler. Let's say you have liblua5.2
# but want to build with liblua5.1 (for which you have the lib, interpreter
# and compiler), you can override by setting LUA_VERSION=5.1 when invoking
# make.
#
# If successful, sets the following variables:
#  * LUA_VERSION (unless already set)
#  * LUA_LIBS (can be appended to LDFLAGS directly)
#  * LUA_INCLUDES (can be appended to CFLAGS directly)
#  * LUA (full path to lua interpreter)
#  * LUAC (full path to lua compiler)
#
# If unsuccessful, you can set these manually.

ifndef LUA

# first off, some helper functions to $(call) - this one executes lua/luac to match it's version
lua_ver_match_bin = $(shell $2 -v 2>&1 | grep -qFo $1. && echo 1)
# this one is like which(1), which isn't portable - from the gmake manual
pathsearch = $(firstword $(wildcard $(addsuffix /$(1),$(subst :, ,$(PATH)))))
# Extract "5.x" from lua -v
lua_ver_extract = $(shell $1 -v 2>&1 | cut -f2 -d' ' | cut -f-2 -d.)

# Lua does not provide an official .pc file, so finding one at all is tricky,
# and there won't be any guarantees it looks the same as elsewhere.
# We try the package names lua$(ver), lua-$(ver) for all candidate versions in
# descending order and last but not least "lua" here.
PKG_CONFIG_ALL_PACKAGES:= $(shell pkg-config --list-all | cut -f1 -d' ')

# these are in order of preference
LUA_CANDIDATES:=		$(or $(LUA_VERSION),5.4 54 5.3 53 5.2 52 5.1 51)
LUA_BIN_CANDIDATES:=		$(foreach ver,$(LUA_CANDIDATES),lua$(ver) lua-$(ver))
LUAC_BIN_CANDIDATES:=		$(foreach ver,$(LUA_CANDIDATES),luac$(ver) luac-$(ver))

# the binaries might of course also just be called lua and luac
ifndef LUA_VERSION
LUA_BIN_CANDIDATES+=		lua
LUAC_BIN_CANDIDATES+=		luac
endif

# find pkg-config packages in the same order of preference
PKG_CONFIG_LUA_PACKAGES:= 	$(foreach lua,$(LUA_BIN_CANDIDATES),$(filter $(lua),$(PKG_CONFIG_ALL_PACKAGES)))
PKG_CONFIG_LUA:=		$(firstword $(PKG_CONFIG_LUA_PACKAGES))

ifeq ($(PKG_CONFIG_LUA),)
$(error It seems that pkg-config is not aware of your lua package. Make sure \
	the -dev package is installed along with a .pc file within the       \
	pkg-config search-path. Alternatively, you can set the following     \
	variables manually: LUA LUAC LUA_VERSION LUA_LIBS LUA_INCLUDES)
endif

PKG_CONFIG_LUA_VERSION:=	$(shell pkg-config --modversion $(PKG_CONFIG_LUA) | cut -d. -f-2)

$(info >> pkg-config found Lua $(PKG_CONFIG_LUA_VERSION) (available: $(PKG_CONFIG_LUA_PACKAGES:=)).)

LUA_LIBS=	$(shell pkg-config --libs $(PKG_CONFIG_LUA))
LUA_INCLUDES=	$(shell pkg-config --cflags $(PKG_CONFIG_LUA))
LUA_VERSION?=	$(PKG_CONFIG_LUA_VERSION)

LUA=		$(firstword $(foreach bin,$(LUA_BIN_CANDIDATES),$(call pathsearch,$(bin))))
LUAC=		$(firstword $(foreach bin,$(LUAC_BIN_CANDIDATES),$(call pathsearch,$(bin))))

$(info >> Lua $(LUA_VERSION) binary is $(LUA) and luac is $(LUAC))

ifneq ($(call lua_ver_match_bin,$(LUA_VERSION),$(LUA)),1)
$(error $(LUA) should be $(LUA_VERSION) but is $(call lua_ver_extract,$(LUA)))
endif

ifneq ($(call lua_ver_match_bin,$(LUA_VERSION),$(LUAC)),1)
$(error $(LUAC) should be $(LUA_VERSION) but is $(call lua_ver_extract,$(LUAC)))
endif

endif

# this is necessary, otherwise the rest of the build process keeps calling
# pkg-config and all these other programs up there
export LUA_VERSION LUA_INCLUDES LUA_LIBS LUA LUAC
