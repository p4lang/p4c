#################################################################
# @FIXME Top matter
#
# Makefile for testmod target
#################################################################
include ../../init.mk

MODULE := tm_target
include $(BUILDER)/standardinit.mk


LIBRARY := tm_target_main
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk

DEPENDMODULES := OS IOF murmur BigList AIM testmod VPI cjson

include $(BUILDER)/dependmodules.mk

BINARY := tm_target

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

GLOBAL_LINK_LIBS += -lpthread -lpcap -ledit -lm
