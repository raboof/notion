##
## System settings
##


##
## Installation paths
##

#DIST: PREFIX=/usr/local
PREFIX=/usr/local/ion-3

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# Main binaries
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/ion3
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/ion3
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/ion3
# Nothing at the moment
INCDIR=$(PREFIX)/include/ion3
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/ion3/mod
# Compiled Lua source code
LCDIR=$(LIBDIR)/ion3/lc
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/ion3/bin
# For ion-completeman system-wide cache
VARDIR=/var/cache/ion3
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

# If you have installed Lua 5.1 from the official tarball without changing
# paths, this should do it.
LUA_DIR=/usr/local
LUA_LIBS = -L$(LUA_DIR)/lib -llua
LUA_INCLUDES = -I$(LUA_DIR)/include
LUA=$(LUA_DIR)/bin/lua
LUAC=$(LUA_DIR)/bin/luac

# If you are using the Debian packages, the following settings should be
# what you want.
#LUA_LIBS=`pkg-config --libs lua5.1`
#LUA_INCLUDES=`pkg-config --cflags lua5.1`
#LUA=`which lua5.1`
#LUAC=`which luac5.1`


##
## X libraries, includes and options
##

X11_PREFIX=/usr/X11R6
# SunOS/Solaris
#X11_PREFIX=/usr/openwin

X11_LIBS=-L$(X11_PREFIX)/lib -lX11 -lXext
X11_INCLUDES=-I$(X11_PREFIX)/include

# Change commenting to disable Xinerama support
XINERAMA_LIBS=-lXinerama
DEFINES += -DCF_XINERAMA
# For Solaris
#XINERAMA_LIBS=
#DEFINES += -DCF_SUN_XINERAMA

# XFree86 libraries up to 4.3.0 have a bug that will cause Ion to segfault
# if Opera is used when i18n support is enabled. The following setting
# should work around that situation.
DEFINES += -DCF_XFREE86_TEXTPROP_BUG_WORKAROUND

# Use the Xutf8 routines (XFree86 extension) instead of Xmb routines in
# an UTF8 locale.
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
