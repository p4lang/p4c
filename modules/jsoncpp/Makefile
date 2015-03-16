CC:= g++
CFLAGS:= -O2 -Wall
CPPFLAGS:=
LIBS:= -lJudy
INC:= -I include/

BUILD_DIR:= build

SRC_DIR = src
SRC_EXT = cpp

# vpath %.$(SRC_EXT) $(SRC_DIR)

srcs=$(wildcard $(SRC_DIR)/*.$(SRC_EXT))
objs=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(BUILD_DIR)/%.o,$(srcs))
deps=$(patsubst $(SRC_DIR)/%.$(SRC_EXT),$(BUILD_DIR)/%.d,$(srcs))

target=libJson.a

all: $(target)

ifeq ($(MAKECMDGOALS),clean)
# doing clean, so dont make deps.
deps_=
else
# doing build, so make deps.
deps_=$(deps)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.$(SRC_EXT) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC) -MM $< >> $(BUILD_DIR)/$*.d

-include $(deps_)
endif # ($(MAKECMDGOALS),clean)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.$(SRC_EXT) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(target): $(objs)
	ar -rcs $@ $^

clean:
	rm -f $(objs) $(deps) $(target)
