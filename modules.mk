THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

MODULES_DIR := $(THIS_DIR)modules
MODULES_NAMES := jsoncpp BMI bf_lpm_trie bm_sim
MODULES := $(patsubst %, $(MODULES_DIR)/%, $(MODULES_NAMES))

include $(patsubst %, %/module.mk, $(MODULES))

