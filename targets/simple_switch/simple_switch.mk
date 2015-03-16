THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRC_DIR := $(realpath $(THIS_DIR))

srcs_C += $(wildcard $(SRC_DIR)/*.c)
srcs_CXX += $(wildcard $(SRC_DIR)/*.cpp)

INCS +=

LIBS +=

COMMON_FLAGS += -DELOGGER_NANOMSG
