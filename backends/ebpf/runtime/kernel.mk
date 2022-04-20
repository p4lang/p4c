# Actual location of the makefile
ROOT_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
# Argument for the CLANG compiler
LLC ?= llc
CLANG ?= clang
override INCLUDES+= -I$(ROOT_DIR) -I$(ROOT_DIR)usr/include/ -I$(ROOT_DIR)contrib/libbpf/include/uapi/
override LIBS+=
# Optimization flags to save space
override CFLAGS+= -O2 -g -c -D__KERNEL__ -D__ASM_SYSREG_H \
		-Wno-unused-value  -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option -Wnoparentheses-equality

# Arguments for the P4 Compiler
P4INCLUDE=-I$(ROOT_DIR)/p4include
# P4 source file argument for the compiler
P4FILE=
P4C=p4c-ebpf
TARGET=kernel
# Extra arguments for the compiler
ARGS=
# Extra arguments for the P4 compiler
P4ARGS=--Werror

# If needed, bpf target files can be hardcoded here
# This can be any file of type ".c", ".bc" or, ".o"
BPFOBJ=
# Get the source name of the object to match targets
BPFNAME=$(basename $(BPFOBJ))

# Possible values: "-mcpu=generic|probe|v1|v2|v3" or "-mcpu=v1|v2 -mattr=+alu32"
LLC_FLAGS="-mcpu=generic"

all: verify_target_bpf $(BPFOBJ)

# Verify LLVM compiler tools are available and bpf target is supported by llc
.PHONY: verify_target_bpf $(CLANG) $(LLC)

verify_cmds: $(CLANG) $(LLC)
	@for TOOL in $^ ; do \
		if ! (which -- "$${TOOL}" > /dev/null 2>&1); then \
			echo "*** ERROR: Cannot find LLVM tool $${TOOL}" ;\
			exit 1; \
		else \
			echo "pass verify_cmds:" \
			true; fi; \
	done

verify_target_bpf: verify_cmds
	@if ! (${LLC} -march=bpf -mattr=help > /dev/null 2>&1); then \
		echo "*** ERROR: LLVM (${LLC}) does not support 'bpf' target" ;\
		echo "   NOTICE: LLVM version >= 3.7.1 required" ;\
		exit 2; \
	else \
		echo "pass verify_target_bpf:" \
		true; fi

# Generate .c files with the P4 compiler
# Keep the intermediate .c file
.PRECIOUS: %.c
$(BPFNAME).c: $(P4FILE)
	@if ! ($(P4C) --help > /dev/null 2>&1); then \
		echo "*** ERROR: Cannot find p4c-ebpf"; \
		exit 1;\
	fi;
	$(P4C) $(P4INCLUDE) --target $(TARGET) -o $@ $< $(P4ARGS) $(P4ARGS_TARGET);

# Compile the C code with the clang llvm compiler
$(BPFNAME).bc: %.bc : %.c
	$(CLANG) $(ARGS) $(CFLAGS) $(INCLUDES) -emit-llvm -c $< -o $@

# Invoke the llvm on the generated .bc code and produce bpf byte code
$(BPFNAME).o: %.o : %.bc
	$(LLC) -march=bpf -mcpu=probe -filetype=obj $< -o $@

.PHONY: psa
psa: P4ARGS_TARGET= --arch psa
psa: $(BPFNAME).c
	$(CLANG) $(ARGS) $(CFLAGS) $(INCLUDES) -emit-llvm -DBTF -c -o  $(BPFNAME).bc $(BPFNAME).c
	$(LLC) -march=bpf $(LLC_FLAGS) -filetype=obj -o $(BPFNAME).o $(BPFNAME).bc

clean:
	rm -f *.o *.bc $(BPFNAME).c $(BPFNAME).h
