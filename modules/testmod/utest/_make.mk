#################################################################
# 
#################################################################

UMODULE := testmod
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk

