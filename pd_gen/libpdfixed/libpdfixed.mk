LIBPDFIXED_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

$(TARGET)_TARGET := $(LIBPDFIXED_MK_DIR)$(TARGET)

THRIFT_IDL := $(LIBPDFIXED_MK_DIR)../../thrift_src/standard.thrift
THRIFT_IDL_MC := $(LIBPDFIXED_MK_DIR)../../thrift_src/simple_pre.thrift

.thrift_gen: $(THRIFT_IDL) $(THRIFT_IDL_MC)
	thrift --gen cpp -out $(LIBPDFIXED_MK_DIR)../fixed-cpp/src/ $(THRIFT_IDL)
	thrift --gen cpp -out $(LIBPDFIXED_MK_DIR)../fixed-cpp/src/ $(THRIFT_IDL_MC)
	rm $(LIBPDFIXED_MK_DIR)../fixed-cpp/src/*skeleton.cpp
	@touch .thrift_gen

.thrift_gen_clean:
	rm -f .thrift_gen

include $(LIBPDFIXED_MK_DIR)../fixed-cpp/module.mk
