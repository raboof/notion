ifeq ($(wildcard $(TOPDIR)/libtu/obj.h),)

#External libtu, feel free to edit
LIBTU_DIR = $(TOPDIR)/../libtu
LIBTU_INCLUDES = -I$(TOPDIR)/..
LIBTU_LIBS = -ltu

else

#In-tree libtu
LIBS_SUBDIRS += libtu

LIBTU_DIR = $(TOPDIR)/libtu
LIBTU_INCLUDES = -I$(TOPDIR)
LIBTU_LIBS = -L$(LIBTU_DIR) -ltu

endif
