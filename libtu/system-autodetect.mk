##
## System settings
##

# Beware: in releases, the Notion system.mk is used to build both libtu,
# libextl and notion - so structural changes to this file should also be
# carried out on the Notion system.mk

##
## Installation paths
##

PREFIX=/usr/local/

# No need to modify these usually
BINDIR=$(PREFIX)/bin
ETCDIR=$(PREFIX)/etc
MANDIR=$(PREFIX)/man
DOCDIR=$(PREFIX)/doc
LIBDIR=$(PREFIX)/lib
INCDIR=$(PREFIX)/include


##
## libc
##

# You may uncomment this if you know your system has
# asprintf and vasprintf in the c library. (gnu libc has.)
# If HAS_SYSTEM_ASPRINTF is not defined, an implementation
# in sprintf_2.2/ is used.
HAS_SYSTEM_ASPRINTF=1


# The POSIX_SOURCE, XOPEN_SOURCE and WARN options should not be necessary,
# they're mainly for development use. So, if they cause trouble (not
# the ones that should be used on your system or the system is broken),
# just comment them out.

C89_SOURCE=-ansi
POSIX_SOURCE=-D_POSIX_SOURCE

# Same as '-Wall -pedantic' without '-Wunused' as callbacks often
# have unused variables.
WARN=	-W -Wimplicit -Wreturn-type -Wswitch -Wcomment \
	-Wtrigraphs -Wformat -Wchar-subscripts \
	-Wparentheses -pedantic -Wuninitialized


CFLAGS ?= -g -Os
CFLAGS += $(WARN) $(DEFINES) $(INCLUDES) $(EXTRA_INCLUDES) -DHAS_SYSTEM_ASPRINTF=$(HAS_SYSTEM_ASPRINTF)
LDFLAGS ?= -Wl,-O1 -Wl,--as-needed


##
## Install & strip
##

# Should work almost everywhere
INSTALL=sh $(TOPDIR)/install-sh -c
INSTALLDIR=mkdir -p

BIN_MODE=755
DATA_MODE=664

STRIP=strip
