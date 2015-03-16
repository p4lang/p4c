CC:= gcc
CXX:= g++
COMMON_FLAGS:=
CFLAGS:= -O0 -g -Wall
CPPFLAGS:= -O0 -g -Wall --std=c++11
LIBS:= -lJudy
INCS:=

TARGET:=simple_switch

BUILD_DIR := build
BUILD_DIRS := $(BUILD_DIR)

srcs_C:=
srcs_CXX:=

MODULES_DIR :=
MODULES_NAMES :=

include ../../modules.mk

include simple_switch.mk

CFLAGS += $(COMMON_FLAGS)
CPPFLAGS += $(COMMON_FLAGS)

srcs := $(srcs_C) $(srcs_CXX)
BUILD_DIRS += $(patsubst %, $(BUILD_DIR)/%, $(sort $(realpath $(dir $(srcs)))))

CFLAGS += $(patsubst %, -I%, $(INCS))
CPPFLAGS += $(patsubst %, -I%, $(INCS))

objs_C := $(patsubst %.c, %.o, $(srcs_C))
objs_CXX := $(patsubst %.cpp, %.o, $(srcs_CXX))

objs := $(objs_C) $(objs_CXX)

deps_C := $(patsubst %.c, %.d, $(srcs_C))
deps_CXX := $(patsubst %.cpp, %.d, $(srcs_CXX))

deps := $(deps_C) $(deps_CXX)

deps_ := $(patsubst %, $(BUILD_DIR)/%, $(deps))
objs_ := $(patsubst %, $(BUILD_DIR)/%, $(objs))

$(TARGET): $(objs_) | $(BUILD_DIRS)
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LIBS)

$(BUILD_DIRS):
	mkdir -p $@

$(BUILD_DIR)/%.d: %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(INC) -MM $< -MT $(BUILD_DIR)$*.o -o $(BUILD_DIR)$*.d

$(BUILD_DIR)/%.d: %.cpp | $(BUILD_DIRS)
	$(CXX) $(CPPFLAGS) $(INC) -MM $< -MT $(BUILD_DIR)$*.o -o $(BUILD_DIR)$*.d

ifeq ($(MAKECMDGOALS),clean)
# doing clean, so dont make deps.
else
# doing build, so make deps.
-include $(deps_)
endif

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(INC) -c -o $(BUILD_DIR)$*.o $<

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIRS)
	$(CXX) $(CPPFLAGS) $(INC) -c -o $(BUILD_DIR)$*.o $<

clean:
	rm -rf $(BUILD_DIRS) $(TARGET)

.PHONY: clean
