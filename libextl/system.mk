##
## System settings
##


##
## Installation paths
##

PREFIX=/usr/local

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# Main binaries
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/libextl
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/libextl
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/libextl
# Nothing at the moment
INCDIR=$(PREFIX)/include/libextl
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/libextl/mod
# Compiled Lua source code
LCDIR=$(LIBDIR)/libextl/lc
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/libextl/bin
# For ion-completeman system-wide cache
VARDIR=/var/cache/libextl
# Message catalogs
LOCALEDIR=$(PREFIX)/share/locale


##
## Libtu
##

LIBTU_LIBS = -ltu
LIBTU_INCLUDES =

##
## Lua
##

# To skip auto-detection of lua uncomment this and edit the variables below to
# suit your installation of lua.
#LUA_MANUAL=1

# Default to paths and names that should work for a build installed from the
# official Lua 5.1 source tarball.
LUA_DIR=$(shell dirname `which lua` | xargs dirname)
LUA_LIBS=-L$(LUA_DIR)/lib -llua
LUA_INCLUDES = -I$(LUA_DIR)/include
LUA=$(LUA_DIR)/bin/lua
LUAC=$(LUA_DIR)/bin/luac

# Attempt to autodect lua using pkg-config.

ifndef LUA_MANUAL

ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua5.1 && pkg-config --modversion lua5.1)))

LUA_LIBS=`pkg-config --libs lua5.1`
LUA_INCLUDES=`pkg-config --cflags lua5.1`
LUA=`which lua5.1`
LUAC=`which luac5.1`

else ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua && pkg-config --modversion lua)))

LUA_LIBS=`pkg-config --libs lua`
LUA_INCLUDES=`pkg-config --cflags lua`
LUA=`which lua`
LUAC=`which luac`

endif # lua

endif # LUA_MANUAL


##
## C compiler
##

CC=gcc

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized

CFLAGS=-g -Os $(WARN) $(DEFINES) $(EXTRA_INCLUDES) $(INCLUDES)
LDFLAGS=-g -Os $(EXTRA_LIBS) $(LIBS)

# The following options are mainly for development use and can be used
# to check that the code seems to conform to some standards. Depending
# on the version and vendor of you libc, the options may or may not have
# expected results. If you define one of C99_SOURCE or XOPEN_SOURCE, you
# may also have to define the other. 

#C89_SOURCE=-ansi

#POSIX_SOURCE=-D_POSIX_SOURCE

# Most systems
#XOPEN_SOURCE=-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
# SunOS, (Irix)
#XOPEN_SOURCE=-D__EXTENSIONS__

#C99_SOURCE=-std=c99 -DCF_HAS_VA_COPY

# The -DCF_HAS_VA_COPY option should allow for some optimisations, and 
# in some cases simply defining
#C99_SOURCE=-DCF_HAS_VA_COPY
# might allow for those optimisations to be taken without any  special
# libc or compiler options.


##
## make depend
##

DEPEND_FILE=.depend
DO_MAKE_DEPEND=$(CC) -MM $(DEFINES) $(EXTRA_INCLUDES) $(INCLUDES)
MAKE_DEPEND=$(DO_MAKE_DEPEND) $(SOURCES) > $(DEPEND_FILE)

##
## AR
##

AR=ar
ARFLAGS=cr
RANLIB=ranlib


##
## Install & strip
##

INSTALL=sh $(TOPDIR)/install-sh -c
INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=644

STRIP=strip

RM=rm
