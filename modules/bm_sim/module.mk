THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))

SRC_DIR := $(realpath $(THIS_DIR)src)

srcs_C += $(wildcard $(SRC_DIR)/*.c)
srcs_CXX += $(wildcard $(SRC_DIR)/*.cpp)

INCS += $(THIS_DIR)include/

LIBS += -lJudy -lm -lnanomsg -lgmp
LIBS += -lboost_thread -lboost_system

