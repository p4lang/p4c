LIBPD_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

$(TARGET)_TARGET := $(LIBPD_MK_DIR)$(TARGET)

.thrift_gen: $(LIBPD_MK_DIR)../thrift_src/runtime.thrift
	thrift --gen cpp -out $(LIBPD_MK_DIR)fixed-cpp/src/ $(LIBPD_MK_DIR)../thrift_src/runtime.thrift
	rm $(LIBPD_MK_DIR)fixed-cpp/src/*skeleton.cpp
	@touch .thrift_gen

.thrift_gen_clean:
	rm -f .thrift_gen

include $(LIBPD_MK_DIR)gen-cpp/module.mk
include $(LIBPD_MK_DIR)fixed-cpp/module.mk

$(TARGET)_PUBLIC_INCS := $(INCS)
