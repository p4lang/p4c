###############################################################################
#
# behavioral_sim Unit Test Makefile.
#
###############################################################################
UMODULE := behavioral_sim
UMODULE_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/utest.mk
