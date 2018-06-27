# If needed, bpf target files can be hardcoded here
# This can be any file with the extension ".c"
BPFOBJ=
# Get the source name of the object to match targets
BPFNAME=$(basename $(BPFOBJ))
INCLUDES+= -I$(dir $(BPFOBJ))

# Argument for the GCC compiler
GCC ?= gcc
SRCDIR=.
INCLUDES+= -I./$(SRCDIR)
# Optimization flags to save space
CFLAGS+= -DCONTROL_PLANE -O2 -g # -Wall -Werror
LIBS+=-lpcap
SRC+= $(SRCDIR)/ebpf_runtime.c $(SRCDIR)/ebpf_map.c $(SRCDIR)/ebpf_registry.c
HDRS := $(BPFNAME).h $(SRCDIR)/ebpf_user.h $(SRCDIR)/ebpf_map.h $(SRCDIR)/ebpf_registry.h

# Arguments for the P4 Compiler
P4INCLUDE=-I./p4include
# P4 source file argument for the compiler (can be hardcoded)
P4FILE=
P4C=p4c-ebpf
TARGET=test
# Extra arguments for the compiler
P4ARGS=

all: $(BPFOBJ)

# Generate .c files with the P4 compiler
$(BPFNAME).c: $(P4FILE)
	@if ! ($(P4C) --help > /dev/null 2>&1); then \
		echo "*** ERROR: Cannot find p4c-ebpf"; \
		exit 1;\
	fi;
	$(P4C) --Werror $(P4INCLUDE) --target $(TARGET) -o $@ $< $(P4ARGS)

# Compile the runtime with the generated file as dependency
$(BPFNAME): $(BPFNAME).c $(SRC) $(HDRS)
	$(GCC) $(CFLAGS) -o $@ $^ $(INCLUDES) $(LIBS)

clean: clean_loader
	rm -f *.o $(BPFNAME).c $(BPFNAME).h
