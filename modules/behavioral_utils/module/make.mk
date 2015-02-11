###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
behavioral_utils_INCLUDES := -I $(THIS_DIR)inc
behavioral_utils_INTERNAL_INCLUDES := -I $(THIS_DIR)src
behavioral_utils_DEPENDMODULE_ENTRIES := init:behavioral_utils ucli:behavioral_utils

