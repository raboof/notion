##
## System settings
##


##
## Installation paths
##

PREFIX=/usr/local/ion-devel

# No need to modify these usually
BINDIR=$(PREFIX)/bin
ETCDIR=$(PREFIX)/etc
MANDIR=$(PREFIX)/man
DOCDIR=$(PREFIX)/doc
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include


##
## Modules
##

MODULE_LIST=ionws floatws query

# For dynamically loaded modules
MODULE_SUPPORT_CFLAGS=
MODULE_SUPPORT_LDFLAGS=-export-dynamic -ldl
MODULE_LDFLAGS=-shared
MODULE_CFLAGS=-shared

# Statically loaded modules
#STATIC_MODULES=1
#MODULE_SUPPORT_CFLAGS=
#MODULE_SUPPORT_LDFLAGS=
#MODULE_LDFLAGS=
#MODULE_CFLAGS=

##
## X libraries, includes and options
##

X11_PREFIX=/usr/X11R6
# SunOS/Solaris
#X11_PREFIX=/usr/openwin

X11_LIBS=-L$(X11_PREFIX)/lib
X11_INCLUDES=-I$(X11_PREFIX)/include

# Change commenting to disable Xinerama support
XINERAMA_LIBS=-lXinerama -lXext
#DEFINES += -DCF_NO_XINERAMA

# Uncomment to enable Xft (anti-aliased fonts) support
#DEFINES += -DCF_XFT
#X11_INCLUDES += `xft-config --cflags`
#X11_LIBS += `xft-config --libs`

# Uncomment to enable UTF8 support. You must have XFree86 (4.x?) and C99
# wide char support available (either libc directly or maybe libutf8+libiconv).
# Although iconv (that is needed to convert to wchar_t -- which is not
# necessarily ucs-4 -- to test character properties) is a standardised
# function, encoding names unfortunately aren't and thus these also have to
# specified here.

# GNU/Linux and other glibc-2.2 based systems.
#DEFINES += -DCF_UTF8 -DCF_ICONV_TARGET=\"WCHAR_T\" -DCF_ICONV_SOURCE=\"UTF-8\"

# Systems that depend on libutf8 and libiconv might want these.
#DEFINES += -DCF_UTF8 -DCF_LIBUTF8 -DCF_ICONV_TARGET=\"C99\" -DCF_ICONV_SOURCE=\"UTF-8\"
#EXTRA_LIBS += -liconv -L/usr/local/lib
#EXTRA_INCLUDES -I/usr/local/include

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


CFLAGS=-g -O2 $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES)
LDFLAGS=-g $(LIBS) $(EXTRA_LIBS)


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
DATA_MODE=664

STRIP=strip

