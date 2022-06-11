# System-specific configuration is in system.mk
TOPDIR=../..
include $(TOPDIR)/build/system-inc.mk

######################################

SOURCES=../misc.c ../tokenizer.c ../util.c ../output.c

LIBS += $(LUA_LIBS) $(DL_LIBS) $(LIBTU_LIBS) -lm
INCLUDES += $(LIBTU_INCLUDES)
CFLAGS += $(XOPEN_SOURCE) $(C99_SOURCE)

######################################

include $(TOPDIR)/build/rules.mk

######################################

test: $(SOURCES)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(INCLUDES) -o tutest $(SOURCES) tutest.c $(LIBS)
	./tutest
	$(RM) ./tutest