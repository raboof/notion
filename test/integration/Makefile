TOPDIR=../..
include $(TOPDIR)/system-autodetect.mk

.PHONY: test

libXinerama.so.1.0: Xinerama.c
	gcc -O2 -Wall Xinerama.c -fPIC -o libXinerama.so.1.0 -shared

test: libXinerama.so.1.0
	LD_PRELOAD=`pwd`/libXinerama.so.1.0 $(LUA) ./runtests.lua
