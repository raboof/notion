ifndef TOPDIR
  TOPDIR=.
endif

include $(TOPDIR)/system-autodetect.mk
include $(TOPDIR)/build/libs.mk

-include $(TOPDIR)/system-local.mk
