###############################################################################
#
# 
#
###############################################################################

include ../../../init.mk
MODULE := behavioral_utils_utest
TEST_MODULE := behavioral_utils
DEPENDMODULES := AIM
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
include $(BUILDER)/build-unit-test.mk
