#################################################################
# 
#  TESTMOD unit test makefile
#
#################################################################
include ../../../init.mk

MODULE := testmod_utest
TEST_MODULE :=  testmod
DEPENDMODULES := AIM uCli BigList IOF murmur

GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_VALGRIND=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_CFLAGS += -DUCLI_CONFIG_INCLUDE_FGETS_LOOP=1

# @FIXME
# GLOBAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-type-limits

OS_MAKE_CONFIG_AUTOSELECT := 1
PEDANTIC := 1

include $(BUILDER)/build-unit-test.mk

