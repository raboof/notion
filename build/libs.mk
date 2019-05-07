LIBS_SUBDIRS = libmainloop

LIBMAINLOOP_DIR = $(TOPDIR)/libmainloop
LIBMAINLOOP_INCLUDES = -I$(TOPDIR)
LIBMAINLOOP_LIBS = -L$(LIBMAINLOOP_DIR) -lmainloop

ifeq ($(wildcard $(TOPDIR)/libtu/obj.h),)

#External libtu, feel free to edit
LIBTU_DIR = $(TOPDIR)/../libtu
LIBTU_INCLUDES =
LIBTU_LIBS = -ltu

else

#In-tree libtu
LIBS_SUBDIRS += libtu

LIBTU_DIR = $(TOPDIR)/libtu
LIBTU_INCLUDES = -I$(TOPDIR)
LIBTU_LIBS = -L$(LIBTU_DIR) -ltu

endif

ifeq ($(wildcard $(TOPDIR)/libextl/luaextl.h),)

#External libextl, feel free to edit
LIBEXTL_DIR = $(TOPDIR)/../libextl
LIBEXTL_INCLUDES =
LIBEXTL_LIBS = -lextl

MKEXPORTS = libextl-mkexports

else

#In-tree libextl
LIBS_SUBDIRS += libextl

LIBEXTL_DIR = $(TOPDIR)/libextl
LIBEXTL_INCLUDES = -I$(TOPDIR)
LIBEXTL_LIBS = -L$(LIBEXTL_DIR) -lextl

MKEXPORTS = $(LUA) $(LIBEXTL_DIR)/libextl-mkexports

endif
