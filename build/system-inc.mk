# Use system-ac.mk if it exist, system.mk otherwise.

ifndef TOPDIR
  TOPDIR=.
endif

SYSTEM_MK = $(TOPDIR)/system.mk
AC_SYSTEM_MK = $(TOPDIR)/build/ac/system-ac.mk

ifeq ($(AC_SYSTEM_MK),$(wildcard $(AC_SYSTEM_MK)))
  # Using system-ac.mk
  include $(AC_SYSTEM_MK)
else
  # Not using system-ac.mk
  include $(SYSTEM_MK)
endif

