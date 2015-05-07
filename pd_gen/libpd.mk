LIBPD_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

$(TARGET)_TARGET := $(LIBPD_MK_DIR)$(TARGET)

include $(LIBPD_MK_DIR)gen-cpp/module.mk
include $(LIBPD_MK_DIR)fixed-cpp/module.mk

$(TARGET)_PUBLIC_INCS := $(INCS)
