##
## Ion scripts Makefile
##

# System-specific configuration is in system.mk
TOPDIR=..
include $(TOPDIR)/system-inc.mk

######################################

EXPORT_TEXES=ioncore-exports.tex ionws-exports.tex \
             floatws-exports.tex query-exports.tex

######################################

include $(TOPDIR)/rules.mk

######################################

all: $(EXPORT_TEXES)

ioncore-exports.tex: ../ioncore/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module ioncore -mkdoc -o $@ $+

ionws-exports.tex: ../ionws/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module ionws -mkdoc -o $@ $+

floatws-exports.tex: ../floatws/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module floatws -mkdoc -o $@ $+

query-exports.tex: ../query/*.c
	$(LUA) $(TOPDIR)/mkexports.lua -module query -mkdoc -o $@ $+
