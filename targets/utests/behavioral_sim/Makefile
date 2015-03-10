###############################################################################
#
# 
#
###############################################################################

include ../../../init.mk
MODULE := behavioral_sim_utest
TEST_MODULE := behavioral_sim
DEPENDMODULES := AIM behavioral_utils
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1
GLOBAL_LINK_LIBS += -lpthread -lJudy -lgmpxx -lgmp -lnanomsg
GXX_PEDANTIC_FLAGS += --std=c++11

include $(BUILDER)/build-unit-test.mk
