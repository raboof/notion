# Use system-ac.mk if it exist, system.mk otherwise.

ifndef TOPDIR
  TOPDIR=.
endif

ifeq ($(TOPDIR)/system-ac.mk,$(wildcard $(TOPDIR)/system-ac.mk))
  # Using system-ac.mk
  include $(TOPDIR)/system-ac.mk
else
  # Not using system-ac.mk
  include $(TOPDIR)/system.mk
endif

