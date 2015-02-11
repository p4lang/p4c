###############################################################################
#
# behavioral_utils Unit Test Makefile.
#
###############################################################################
UMODULE := behavioral_utils
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
