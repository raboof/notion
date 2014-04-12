##
## System settings
##


##
## Installation paths
##

#DIST: PREFIX=/usr/local
PREFIX=/usr/local

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# Main binaries
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/notion
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/notion
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/notion
# Nothing at the moment
INCDIR=$(PREFIX)/include/notion
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/notion/mod
# Compiled Lua source code
LCDIR=$(LIBDIR)/notion/lc
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/notion/bin
# For notion-completeman system-wide cache
VARDIR=/var/cache/notion
# Message catalogs
LOCALEDIR=$(PREFIX)/share/locale


##
## Modules
##

# Set PRELOAD_MODULES=1 if your system does not support dynamically loaded
# modules through 'libdl' or has non-standard naming conventions.
#PRELOAD_MODULES=1

# Flags to link with libdl.
DL_LIBS=-ldl


##
## Lua
##

# To skip auto-detection of lua uncomment this and edit the variables below to
# suit your installation of lua.
#LUA_MANUAL=1

# Default to paths and names that should work for a build installed from the
# official Lua 5.1 source tarball.
LUA_DIR=/usr/local
LUA_LIBS=-L$(LUA_DIR)/lib -llua
LUA_INCLUDES = -I$(LUA_DIR)/include

ifneq ($(shell which lua),)
LUA=$(LUA_DIR)/bin/lua
LUAC=$(LUA_DIR)/bin/luac
endif

# Attempt to autodect lua using pkg-config.

ifndef LUA_MANUAL

# lua libraries and includes:

ifeq (5.2,$(findstring 5.2,$(shell pkg-config --exists lua5.2 && pkg-config --modversion lua5.2)))
ifneq ($(shell which lua5.2),)
HAS_LUA52=true
HAS_LUA=true
endif
endif

ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua5.1 && pkg-config --modversion lua5.1)))
ifneq ($(shell which lua5.1),)
HAS_LUA51=true
HAS_LUA=true
endif
endif

ifeq (5.1,$(findstring 5.1,$(shell pkg-config --exists lua && pkg-config --modversion lua)))
ifneq ($(shell which lua),)
HAS_LUA=true
endif
endif

ifdef HAS_LUA52

LUA_LIBS=`pkg-config --libs lua5.2`
LUA_INCLUDES=`pkg-config --cflags lua5.2`
LUA=`which lua5.2`
LUAC=`which luac5.2`

else ifdef HAS_LUA51

LUA_LIBS=`pkg-config --libs lua5.1`
LUA_INCLUDES=`pkg-config --cflags lua5.1`
LUA=`which lua5.1`
LUAC=`which luac5.1`

else ifdef HAS_LUA

LUA_LIBS=`pkg-config --libs lua`
LUA_INCLUDES=`pkg-config --cflags lua`
LUA=`which lua`
LUAC=`which luac`

endif # lua

endif # LUA_MANUAL

##
## X libraries, includes and options
##

X11_PREFIX=/usr/X11R6
# SunOS/Solaris
#X11_PREFIX=/usr/openwin

X11_LIBS=-L$(X11_PREFIX)/lib -lX11 -lXext
X11_INCLUDES=-I$(X11_PREFIX)/include

# XFree86 libraries up to 4.3.0 have a bug that can cause a segfault.
# The following setting  should  work around that situation.
DEFINES += -DCF_XFREE86_TEXTPROP_BUG_WORKAROUND

# Use the Xutf8 routines (XFree86 extension) instead of the Xmb routines
# in an UTF-8 locale. (No, you don't need this in UTF-8 locales, and 
# most likely don't even want. It's only there because both Xmb and 
# Xutf8 routines are broken, in different ways.)
#DEFINES += -DCF_DE_USE_XUTF8

# Remap F11 key to SunF36 and F12 to SunF37? You may want to set this
# on SunOS.
#DEFINES += -DCF_SUN_F1X_REMAP


##
## libc
##

# You may uncomment this if you know your system has
# asprintf and vasprintf in the c library. (gnu libc has.)
# If HAS_SYSTEM_ASPRINTF is not defined, an implementation
# in sprintf_2.2/ is used.
#HAS_SYSTEM_ASPRINTF=1

# If you're on an archaic system (such as relatively recent *BSD releases)
# without even dummy multibyte/widechar and localisation support, you may 
# have to uncomment the following line:
#DEFINES += -DCF_NO_LOCALE

# On some other systems you may something like this:
#EXTRA_LIBS += -lintl
#EXTRA_INCLUDES +=


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
CFLAGS=-Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS=-Wl,--as-needed $(LIBS) $(EXTRA_LIBS) 
EXPORT_DYNAMIC=-Xlinker --export-dynamic

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

ifeq ($(PRELOAD_MODULES),1)
X11_LIBS += -lXinerama -lXrandr
endif
