##
## Ion scripts Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

TARGETS=ioncore-exports.tex ionws-exports.tex \
	floatws-exports.tex query-exports.tex \
	querylib-fns.tex ioncorelib-fns.tex

######################################

include $(TOPDIR)/rules.mk

######################################

ioncore-exports.tex: ../ioncore/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module ioncore -mkdoc -o $@ $+

ionws-exports.tex: ../ionws/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module ionws -mkdoc -o $@ $+

floatws-exports.tex: ../floatws/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module floatws -mkdoc -o $@ $+

query-exports.tex: ../query/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module query -mkdoc -o $@ $+

querylib-fns.tex: ../etc/querylib.lua
	$(LUA) $(TOPDIR)/mkexports.lua -module query -luadoc -o $@ $+

ioncorelib-fns.tex: ../etc/ioncorelib.lua
	$(LUA) $(TOPDIR)/mkexports.lua -module query -luadoc -o $@ $+
