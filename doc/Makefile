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

ioncore-exports.tex:
	$(LUA) $(TOPDIR)/mkexports.lua -module ioncore -mkdoc \
	-o ioncore-exports.tex ../ioncore/*.c

ionws-exports.tex:
	$(LUA) $(TOPDIR)/mkexports.lua -module ionws -mkdoc \
	-o ionws-exports.tex ../ionws/*.c

floatws-exports.tex:
	$(LUA) $(TOPDIR)/mkexports.lua -module floatws -mkdoc \
	-o floatws-exports.tex ../floatws/*.c

query-exports.tex:
	$(LUA) $(TOPDIR)/mkexports.lua -module query -mkdoc \
	-o query-exports.tex ../query/*.c
