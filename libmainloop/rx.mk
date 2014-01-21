##
## Rules for re-exporting libmainloop exports.
##

MAINLOOP_DIR = $(TOPDIR)/libmainloop

MAINLOOP_SOURCES_ = select.c defer.c signal.c hooks.c exec.c

MAINLOOP_SOURCES = $(patsubst %,$(MAINLOOP_DIR)/%, $(MAINLOOP_SOURCES_))

MKEXPORTS_EXTRA_DEPS += $(MAINLOOP_SOURCES)
MKEXPORTS_EXTRAS += -reexport mainloop $(MAINLOOP_SOURCES)
