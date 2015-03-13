#################################################################
# @FIXME Top matter
#
# Makefile for testmod target
#################################################################
include ../../init.mk

MODULE := simple_switch
include $(BUILDER)/standardinit.mk


LIBRARY := simple_switch_main
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk

DEPENDMODULES := AIM behavioral_utils BMI behavioral_sim

include $(BUILDER)/dependmodules.mk

BINARY := simple_switch

$(BINARY)_LIBRARIES := $(LIBRARY_TARGETS) 
include $(BUILDER)/bin.mk


include $(BUILDER)/targets.mk

# These indicate Linux specific implementations to be used for
# various features
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MODULES_INIT=1
GLOBAL_CFLAGS += -DAIM_CONFIG_INCLUDE_MAIN=1

# @FIXME
# GLOBAL_CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-type-limits

#GLOBAL_CFLAGS += -DOF_OBJECT_TRACKING
GLOBAL_CFLAGS += -g
GLOBAL_CFLAGS += -DELOGGER_NANOMSG

GXX_PEDANTIC_FLAGS += --std=c++11 -g

GLOBAL_LINK_LIBS += -lpthread -lpcap -lm -lgmp -lJudy -lnanomsg
GLOBAL_LINK_LIBS += -lboost_thread -lboost_system
