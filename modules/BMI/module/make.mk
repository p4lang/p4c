###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
BMI_INCLUDES := -I $(THIS_DIR)inc
BMI_INTERNAL_INCLUDES := -I $(THIS_DIR)src
BMI_DEPENDMODULE_ENTRIES := init:bmi ucli:bmi

