# Actual location of the makefile
ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
# If needed, bpf target files can be hardcoded here
# This can be any file with the extension ".c"
BPFOBJ=
# Get the source name of the object to match targets
BPFNAME=$(basename $(BPFOBJ))
BPFDIR=$(dir $(BPFOBJ))
override INCLUDES+= -I$(dir $(BPFOBJ))

# Arguments for the P4 Compiler
P4INCLUDE=-I./p4include
# P4 source file argument for the compiler (can be hardcoded)
P4FILE=
P4C=p4c-ebpf
# the default target is test but it can be overridden
TARGET=test
# Extra arguments for the compiler
P4ARGS=

# Argument for the GCC compiler
GCC ?= gcc
BUILDDIR:= $(BPFDIR)build
override INCLUDES+= -I$(ROOT_DIR) -include $(ROOT_DIR)ebpf_runtime_$(TARGET).h
# Optimization flags to save space
override CFLAGS+= -O2 -g # -Wall -Werror
override LIBS+= -lpcap

# The base files required to build the runtime
SOURCE_BASE= $(ROOT_DIR)ebpf_runtime.c $(ROOT_DIR)pcap_util.c
SOURCE_BASE+= $(ROOT_DIR)ebpf_runtime_$(TARGET).c
# Add the generated file and externs to the base sources
override SOURCES+= $(SOURCE_BASE)
SRC_PROCESSED= $(notdir $(SOURCES))
OBJECTS = $(SRC_PROCESSED:%.c=$(BUILDDIR)/%.o)
DEPS = $(OBJECTS:%.o=%.d)

# checks the executable and symlinks to the output
all: $(BPFNAME).c $(BPFNAME)
	# @$(RM) -rf $(BUILDDIR)

# Creation of the executable
$(BPFNAME): $(OBJECTS)
	@echo "Linking: $@"
	$(GCC) $(CFLAGS) $(INCLUDES) $(OBJECTS) -o $@ $(LIBS)

# Add dependency files, if they exist
-include $(DEPS)


# We build the main target separately from the auxiliary objects
# TODO: Find a way to make this more elegant
$(BUILDDIR)/$(notdir $(BPFNAME)).o: $(BPFNAME).c
	@echo "Creating folder: $(dir $@)"
	@mkdir -p $(dir $@)
	@echo "Compiling: $< -> $@"
	$(GCC) $(CFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

# Compile the base files
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILDDIR)/%.o: ./%.c
	@echo "Creating folder: $(dir $@)"
	@mkdir -p $(dir $@)
	@echo "Compiling: $< -> $@"
	$(GCC) $(CFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

# If the target file is missing, generate .c files with the P4 compiler
$(BPFNAME).c: $(P4FILE)
	@if ! ($(P4C) --help > /dev/null 2>&1); then \
		echo "*** ERROR: Cannot find $(P4C)"; \
		exit 1;\
	fi;
	$(P4C) --Werror $(P4INCLUDE) --target $(TARGET) -o $@ $< $(P4ARGS)

.PHONY: clean
clean:
	@echo "Deleting build folder"
	@$(RM) -rf $(BUILDDIR)
	@$(RM) $(BPFNAME)

