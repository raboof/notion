LIBS_SUBDIRS =

ifeq ($(shell ls $(TOPDIR)/libtu/obj.h 2>/dev/null),)

#External libtu, feel free to edit
LIBTU_DIR = $(TOPDIR)/../libtu
LIBTU_INCLUDES = 
LIBTU_LIBS = -ltu

else

#libtu as subproject
LIBTU_DIR = $(TOPDIR)/libtu
LIBTU_INCLUDES = -I$(TOPDIR)
LIBTU_LIBS = -L$(LIBTU_DIR) -ltu
LIBS_SUBDIRS += libtu

endif

ifeq ($(shell ls $(TOPDIR)/libextl/luaextl.h 2>/dev/null),)

#External libextl, feel free to edit
LIBEXTL_DIR = $(TOPDIR)/../libextl
LIBEXTL_INCLUDES =
LIBEXTL_LIBS = -lextl

MKEXPORTS = libextl-mkexports

else

LIBEXTL_DIR = $(TOPDIR)/libextl
LIBEXTL_INCLUDES = -I$(TOPDIR)
LIBEXTL_LIBS = -L$(LIBEXTL_DIR) -lextl

MKEXPORTS = $(LUA) $(LIBEXTL_DIR)/libextl-mkexports

LIBS_SUBDIRS += libextl

endif

LIBMAINLOOP_DIR = $(TOPDIR)/libmainloop
LIBMAINLOOP_INCLUDES = -I$(TOPDIR)
LIBMAINLOOP_LIBS = -L$(LIBMAINLOOP_DIR) -lmainloop

LIBS_SUBDIRS += libmainloop
