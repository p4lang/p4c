###############################################################################
#
# BMI Unit Test Makefile.
#
###############################################################################
UMODULE := BMI
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
