CC:= gcc
CXX:= g++
CFLAGS:= -O0 -g -Wall -fPIC
CPPFLAGS:= -O0 -g -Wall --std=c++11 -fPIC
LIBS:=
INCS:=

TARGET:=libpd.so

BUILD_DIR := build
BUILD_DIRS := $(BUILD_DIR)

srcs_C:=
srcs_CXX:=

include libpd.mk

$(TARGET)_PUBLIC_INCS := $(INCS)

CFLAGS += $(COMMON_FLAGS)
CPPFLAGS += $(COMMON_FLAGS)

srcs := $(srcs_C) $(srcs_CXX)
BUILD_DIRS += $(patsubst %, $(BUILD_DIR)%, $(sort $(realpath $(dir $(srcs)))))

CFLAGS += $(patsubst %, -I%, $(INCS))
CPPFLAGS += $(patsubst %, -I%, $(INCS))

objs_C := $(patsubst %.c, %.o, $(srcs_C))
objs_CXX := $(patsubst %.cpp, %.o, $(srcs_CXX))

objs := $(objs_C) $(objs_CXX)

deps_C := $(patsubst %.c, %.d, $(srcs_C))
deps_CXX := $(patsubst %.cpp, %.d, $(srcs_CXX))

deps := $(deps_C) $(deps_CXX)

deps_C_ := $(patsubst %, $(BUILD_DIR)%, $(deps_C))
deps_CXX_ := $(patsubst %, $(BUILD_DIR)%, $(deps_CXX))
deps_ := $(patsubst %, $(BUILD_DIR)%, $(deps))
objs_C_ := $(patsubst %, $(BUILD_DIR)%, $(objs_C))
objs_CXX_ := $(patsubst %, $(BUILD_DIR)%, $(objs_CXX))
objs_ := $(patsubst %, $(BUILD_DIR)%, $(objs))

$(TARGET): $(objs_) | $(BUILD_DIRS)
	$(CXX) -o $@ $^ -shared -fPIC

$(BUILD_DIRS):
	mkdir -p $@

$(deps_C_): $(BUILD_DIR)%.d: %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(INC) -MM $< -MT $(BUILD_DIR)$*.o -o $(BUILD_DIR)$*.d

$(deps_CXX_): $(BUILD_DIR)%.d: %.cpp | $(BUILD_DIRS)
	$(CXX) $(CPPFLAGS) $(INC) -MM $< -MT $(BUILD_DIR)$*.o -o $(BUILD_DIR)$*.d

ifeq ($(MAKECMDGOALS),clean)
# doing clean, so dont make deps.
else
# doing build, so make deps.
-include $(deps_)
endif

$(objs_C_): $(BUILD_DIR)%.o: %.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) $(INC) -c -o $(BUILD_DIR)$*.o $<

$(objs_CXX_): $(BUILD_DIR)%.o: %.cpp | $(BUILD_DIRS)
	$(CXX) $(CPPFLAGS) $(INC) -c -o $(BUILD_DIR)$*.o $<

clean:
	rm -rf $(BUILD_DIRS) $(TARGET)

.PHONY: clean
