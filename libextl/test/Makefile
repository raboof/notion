# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

# SOURCES=../readconfig.c ../misc.c

#LIBS += $(LUA_LIBS) $(DL_LIBS) -lm
INCLUDES += -I.. $(LIBTU_INCLUDES) $(LUA_INCLUDES)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

SOURCES=../readconfig.c ../misc.c

LIBS += $(LIBTU_LIBS) $(LUA_LIBS) $(DL_LIBS) -lm

HEADERS=readconfig.h extl.h luaextl.h private.h types.h


######################################

include $(TOPDIR)/build/rules.mk

######################################

test: $(SOURCES)
	$(CC) $(CFLAGS) -o extltest $(SOURCES) extltest.c $(INCLUDES) $(LIBS)
	./extltest
	# $(RM) extltest

