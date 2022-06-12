.PHONY: test

CFLAGS ?= -O2 -Wall

libXinerama.so.1.0: Xinerama.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) Xinerama.c -fPIC -o $@ -shared -Wl,-soname -Wl,$@

test: libXinerama.so.1.0
	LD_PRELOAD=`pwd`/libXinerama.so.1.0 $(LUA) ./runtests.lua
