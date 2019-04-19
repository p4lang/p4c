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
SRCDIR=.
BUILDDIR:= $(BPFDIR)build
override INCLUDES+= -I./$(SRCDIR) -include ebpf_runtime_$(TARGET).h
# Optimization flags to save space
override CFLAGS+=-O2 -g # -Wall -Werror
LIBS+=-lpcap
SOURCES=$(SRCDIR)/ebpf_registry.c  $(SRCDIR)/ebpf_map.c $(BPFNAME).c
SRC_BASE+=$(SRCDIR)/ebpf_runtime.c $(SRCDIR)/pcap_util.c $(SOURCES)
SRC_BASE+=$(SRCDIR)/ebpf_runtime_$(TARGET).c
OBJECTS = $(SRC_BASE:%.c=$(BUILDDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)

# checks the executable and symlinks to the output
all: $(BPFNAME).c $(BPFNAME)
	# @$(RM) -rf $(BUILDDIR)

# Creation of the executable
$(BPFNAME): $(OBJECTS)
	@echo "Linking: $@"
	$(GCC) $(CFLAGS) $(INCLUDES) $(OBJECTS) -o $@ $(LIBS)

# Add dependency files, if they exist
-include $(DEPS)

# Source file rules
# After the first compilation they will be joined with the rules from the
# dependency files to provide header dependencies
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Creating folder: $(dir $(OBJECTS))"
	@mkdir -p $(dir $(OBJECTS))
	@echo "Compiling: $< -> $@"
	$(GCC) $(CFLAGS) $(INCLUDES) $(LIBS) -MP -MMD -c $< -o $@

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

