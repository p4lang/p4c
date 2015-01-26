###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
behavioral_sim_INCLUDES := -I $(THIS_DIR)inc
behavioral_sim_INTERNAL_INCLUDES := -I $(THIS_DIR)src
behavioral_sim_DEPENDMODULE_ENTRIES := init:behavioral_sim ucli:behavioral_sim

