##
## System settings
##


##
## Installation paths
##

PREFIX=/usr/local/ion-devel

# Unless you are creating a package conforming to some OS's standards, you
# probably do not want to modify the following directories:

# 'ioncore' binary and 'ion' script
BINDIR=$(PREFIX)/bin
# Configuration .lua files
ETCDIR=$(PREFIX)/etc/ion-devel
# Some .lua files and ion-* shell scripts
SHAREDIR=$(PREFIX)/share/ion-devel
# Manual pages
MANDIR=$(PREFIX)/man
# Some documents
DOCDIR=$(PREFIX)/doc/ion-devel
# Nothing at the moment
INCDIR=$(PREFIX)/include/ion-devel
# Nothing at the moment
LIBDIR=$(PREFIX)/lib
# Modules
MODULEDIR=$(LIBDIR)/ion-devel
# ion-completefile (does not belong in SHAREDIR being a binary file)
EXTRABINDIR=$(LIBDIR)/ion-devel

##
## Modules
##

LIBTOOL=libtool

# Set PRELOAD_MODULES=1 if your system does not support dynamically loaded
# modules. 
#PRELOAD_MODULES=1

# Note to Cygwin users: you must set this option and also LIBTOOL point to 
# a real libtool script (e.g. /usr/autotool/stable/bin/libtool) instead of
# some useless autoconf-expecting wrapper. In order to worksapce save files
# to work, you should also add the setting
#DEFINES+=-DCF_SECOND_RATE_OS_FS
# to replace colons in save file names with underscores. With these settings
# Ion should compile on at least the version of Cygwin I installed on
# 2003-06-17 on WinXP.

# List of modules to build (and possibly preload)
MODULE_LIST=ionws floatws query

# Settings for compiling and linking to ltdl
LTDL_INCLUDES=
LTDL_LIBS=-lltdl
# Libtool 1.4.3 or newer is highly recommended, but some older versions
# might work with the following flag.
#LTDL_CFLAGS=-DCF_LTDL_ANCIENT


##
## Lua
##

# If you have installed Lua 5.0 from the official tarball without changing
# paths, this so do it.
LUA_PATH=/usr/local
LUA_LIBS = -L$(LUA_PATH)/lib -R$(LUA_PATH)/lib -llua -llualib
LUA_INCLUDES = -I$(LUA_PATH)/include
LUA=$(LUA_PATH)/bin/lua

# If you are using the Debian packages, the following settings should be
# what you want.
#LUA_PATH=/usr
#LUA_LIBS = -llua50 -llualib50
#LUA_INCLUDES = -I$(LUA_PATH)/include/lua50
#LUA=$(LUA_PATH)/bin/lua50


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

# Uncomment to enable UTF8 support. You must have XFree86 (4.x?) and C99
# wide char support available (either libc directly or maybe libutf8+libiconv).
# Although iconv (that is needed to convert to wchar_t -- which is not
# necessarily ucs-4 -- to test character properties) is a standardised
# function, encoding names unfortunately aren't and thus these also have to
# specified here.
#
# Note that XFree86 libraries up to 4.3.0 have a bug that will cause Ion to
# segfault if Opera is used when UTF8 support is enabled.

# GNU/Linux and other glibc-2.2 based systems.
#DEFINES += -DCF_UTF8 -DCF_ICONV_TARGET=\"WCHAR_T\" -DCF_ICONV_SOURCE=\"UTF-8\"

# Systems that depend on libutf8 and libiconv might want these.
#DEFINES += -DCF_UTF8 -DCF_LIBUTF8 -DCF_ICONV_TARGET=\"C99\" -DCF_ICONV_SOURCE=\"UTF-8\"
#EXTRA_LIBS += -liconv -lutf8 -L/usr/local/lib
#EXTRA_INCLUDES += -I/usr/local/include

# Uncomment to enable Xft (anti-aliased fonts) support. 
# NOTE: This feature is a bonus that may or may not work. I have better
# things to do than test something I strongly dislike after every change to
# Ion.
#DEFINES += -DCF_XFT
#X11_INCLUDES += `xft-config --cflags`
#X11_LIBS += `xft-config --libs`


##
## libc
##

# You may uncomment this if you know your system has
# asprintf and vasprintf in the c library. (gnu libc has.)
# If HAS_SYSTEM_ASPRINTF is not defined, an implementation
# in sprintf_2.2/ is used.
#HAS_SYSTEM_ASPRINTF=1


##
## C compiler
##

CC=gcc

# The POSIX_SOURCE, XOPEN_SOURCE and WARN options should not be necessary,
# they're mainly for development use. So, if they cause trouble (not
# the ones that should be used on your system or the system is broken),
# just comment them out.

# libtu/ uses POSIX_SOURCE

POSIX_SOURCE=-ansi -D_POSIX_SOURCE

# and . (ion) XOPEN_SOURCE.
# There is variation among systems what should be used and how they interpret
# it so it is perhaps better not using anything at all.

# Most systems
#XOPEN_SOURCE=-ansi -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED
# sunos, (irix)
#XOPEN_SOURCE=-ansi -D__EXTENSIONS__

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized

# If you have a recent C compiler (e.g. gcc 3.x) then uncommenting the
# following should optimize function calls to Lua a little.
#C99_SOURCE=-std=c99 -DCF_HAS_VA_COPY

CFLAGS=-g -Os $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS=-g -Os $(LIBS) $(EXTRA_LIBS)


##
## make depend
##

DEPEND_FILE=.depend
MAKE_DEPEND=$(CC) -M $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES) > $(DEPEND_FILE)


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

