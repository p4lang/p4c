###############################################################################
#
# 
#
###############################################################################

include ../../../init.mk
MODULE := BMI_utest
TEST_MODULE := BMI
DEPENDMODULES := AIM
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1

GLOBAL_LINK_LIBS += -lpthread -lpcap -lJudy

include $(BUILDER)/build-unit-test.mk
