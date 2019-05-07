##
## System settings
##

# Some system may have custom pkg-config executable name
PKG_CONFIG ?= pkg-config

##
## Installation paths
##

#DIST: PREFIX=/usr/local
PREFIX ?= /usr/local

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# Main binaries
BINDIR=$(PREFIX)/bin
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/notion
# Manual pages
MANDIR=$(PREFIX)/share/man
# Some documents
DOCDIR=$(PREFIX)/share/doc/notion
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

# Configuration .lua files. Overrideable, as config files are usually
# not placed under $(PREFIX).
ETCDIR ?= $(PREFIX)/etc/notion

# Force all include files to be installed to /usr even if the
# PREFIX is unset. No header files are installed at the moment
# though.
ifeq ($(PREFIX),)
    INCDIR = $(PREFIX)/include/notion
else
    INCDIR = /usr/include/notion
endif

# Executable suffix (for Cygwin).
#BIN_SUFFIX = .exe

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

include $(TOPDIR)/build/lua-detect.mk

##
## X libraries, includes and options
##

# Paths
X11_PREFIX ?= /usr/X11R6
# SunOS/Solaris
#X11_PREFIX ?= /usr/openwin

X11_LIBS=$(shell $(PKG_CONFIG) --libs x11 xext)
X11_INCLUDES=$(shell $(PKG_CONFIG) --cflags-only-I x11 xext)

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
HAS_SYSTEM_ASPRINTF=1

# If you're on an archaic system (such as relatively recent *BSD releases)
# without even dummy multibyte/widechar and localisation support, you may
# have to uncomment the following line:
#DEFINES += -DCF_NO_LOCALE

# On some other systems you may something like this:
#EXTRA_LIBS += -lintl
#EXTRA_INCLUDES +=


##
## libc
##

# You may uncomment this if you know that your system C libary provides
# asprintf and  vasprintf. (GNU libc does.) If HAS_SYSTEM_ASPRINTF is not
# defined, an implementation provided in libtu/sprintf_2.2/ is used.
HAS_SYSTEM_ASPRINTF ?= 1

# The following setting is needed with GNU libc for clock_gettime and the
# monotonic clock. Other systems may not need it, or may not provide a
# monotonic clock at all (which Ion can live with, and usually detect).
EXTRA_LIBS += -lrt

# Cygwin needs this. Also when you disable _BSD_SOURCE you may need it.
#DEFINES += -DCF_NO_GETLOADAVG


#
# If you're using/have gcc, it is unlikely that you need to modify
# any of the settings below this line.
#
#####################################################################


##
## C compiler.
##

CC ?= gcc

WARN=-W -Wall -pedantic

CFLAGS += -Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES) \
          -DHAS_SYSTEM_ASPRINTF=$(HAS_SYSTEM_ASPRINTF)

LDFLAGS += -Wl,--as-needed $(LIBS) $(EXTRA_LIBS)
EXPORT_DYNAMIC=-Xlinker --export-dynamic

# The following options are mainly for development use and can be used
# to check that the code seems to conform to some standards. Depending
# on the version and vendor of you libc, the options may or may not have
# expected results. If you define one of C99_SOURCE or XOPEN_SOURCE, you
# may also have to define the other.

#C89_SOURCE=-ansi

POSIX_SOURCE?=-D_POSIX_C_SOURCE=200112L
BSD_SOURCE?=-D_BSD_SOURCE

# Most systems
XOPEN_SOURCE=-D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
# SunOS, (Irix)
#XOPEN_SOURCE=-D__EXTENSIONS__

C99_SOURCE?=-std=c99 -DCF_HAS_VA_COPY

# The -DCF_HAS_VA_COPY option should allow for some optimisations, and
# in some cases simply defining
#C99_SOURCE=-DCF_HAS_VA_COPY
# might allow for those optimisations to be taken without any  special
# libc or compiler options.

##
## AR
##

AR ?= ar
ARFLAGS ?= cr
RANLIB ?= ranlib


##
## Install & strip
##

INSTALL ?= sh $(TOPDIR)/install-sh -c
INSTALL_STRIP = -s
INSTALLDIR ?= mkdir -p

BIN_MODE ?= 755
DATA_MODE ?= 644

RM ?= rm


##
## Debugging
##

RM=rm

ifeq ($(PRELOAD_MODULES),1)
X11_LIBS += -lXinerama -lXrandr
endif
