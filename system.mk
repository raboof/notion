##
## System settings
##


##
## Installation paths
##

PREFIX=/usr/local/ion-3

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# Main binaries
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/ion
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/ion
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/ion
# Nothing at the moment
INCDIR=$(PREFIX)/include/ion
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/ion
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/ion
# For ion-completeman system-wide cache
VARDIR=/var/cache/ion


##
## Modules
##

# The path to the libtool script. Version 1.4.3 or newer is required.
# Users of many of the *BSD:s will have to manually install a a recent
# libtool because even more-recent-than-libtool-1.4.3 releases of those
# OSes only have an _ancient_ 1.3.x libtool that _will_ _not_ _work even
# though a lot of libltdl-using apps require 1.4.3.
LIBTOOL=libtool

# Settings for compiling and linking to ltdl
LTDL_INCLUDES=
LTDL_LIBS=-lltdl

# The following should do it if you have manually installed libtool 1.5 in
# $(LIBTOOLDIR).
#
# Note to Cygwin users: you must set LIBTOOLDIR point to a real libtool 
# script (e.g. /usr/autotool/stable/bin/libtool) instead of some useless
# autoconf-expecting wrapper. If you also set PRELOAD_MODULES=1 and disable
# Xinerama support below, Ion should compile on Cygwin.
#
#LIBTOOLDIR=/usr/local/stow/libtool-1.5
#LIBTOOL=$(LIBTOOLDIR)/bin/libtool
#LTDL_INCLUDES=-I$(LIBTOOLDIR)/include
#LTDL_LIBS=-L$(LIBTOOLDIR)/lib -lltdl

# Set PRELOAD_MODULES=1 if your system does not support dynamically loaded
# modules. 
#PRELOAD_MODULES=1


##
## Lua
##

# If you have installed Lua 5.0 from the official tarball without changing
# paths, this should do it.
LUA_DIR=/usr/local
LUA_LIBS = -L$(LUA_DIR)/lib -R$(LUA_DIR)/lib -llua -llualib
LUA_INCLUDES = -I$(LUA_DIR)/include
LUA=$(LUA_DIR)/bin/lua
LUAC=$(LUA_DIR)/bin/luac

# If you are using the Debian packages, the following settings should be
# what you want.
#LUA_LIBS=`lua-config50 --libs`
#LUA_INCLUDES=`lua-config50 --include`
#LUA=lua50
#LUAC=luac50


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
#DEFINES += -DCF_NO_XINERAMA

# XFree86 libraries up to 4.3.0 have a bug that will cause Ion to segfault
# if Opera is used when i18n support is enabled. The following setting
# should work around that situation.
DEFINES += -DCF_XFREE86_TEXTPROP_BUG_WORKAROUND

# Use the Xutf8 routines (XFree86 extension) instead of Xmb routines in
# an UTF8 locale given the -i18n command line option?
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
# without even dummy multibyte/widechar support, you may have to uncomment
# the following line:
#DEFINES += -DCF_NO_MB_SUPPORT


##
## C compiler
##

CC=gcc

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized

CFLAGS=-g -Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS=-g -Os $(LIBS) $(EXTRA_LIBS)

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
DO_MAKE_DEPEND=$(CC) -MM $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
MAKE_DEPEND=$(DO_MAKE_DEPEND) $(SOURCES) > $(DEPEND_FILE)
MAKE_DEPEND_MOD=$(DO_MAKE_DEPEND) $(SOURCES) | sed 's/\.o/\.lo/' > $(DEPEND_FILE)

##
## AR
##

AR=ar
ARFLAGS=cr
RANLIB=ranlib


##
## Install & strip
##

# Should work almost everywhere
INSTALL=install
# On a system with pure BSD install, -c might be preferred
#INSTALL=install -c

INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=644

STRIP=strip

