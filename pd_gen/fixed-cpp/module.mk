THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRC_DIR := $(realpath $(THIS_DIR)src)

srcs_C += $(wildcard $(SRC_DIR)/*.c)
srcs_CXX += $(wildcard $(SRC_DIR)/*.cpp)

INCS += $(THIS_DIR)include/
INCS += $(THIS_DIR)src/

LIBS += -lthrift


