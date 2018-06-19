LLC ?= llc
CLANG ?= clang
# If needed, bpf target files can be hardcoded here
BPFOBJ=
# Interface to attach programs to
IFACE=
INCLUDE=-I./

# P4 Compiler
P4INCLUDE=-I./p4include
P4C=p4c-ebpf
TARGET=kernel
# Arguments for the compiler
ARGS=


all: verify_target_bpf $(BPFOBJ)

# Verify LLVM compiler tools are available and bpf target is supported by llc
.PHONY: verify_cmds verify_target_bpf $(CLANG) $(LLC)

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
%.c: %.p4
	@if ! ($(P4C) --help > /dev/null 2>&1); then \
		echo "*** ERROR: Cannot find p4c-ebpf"; \
		exit 1;\
	fi;
	$(P4C) --Werror $(P4INCLUDE) --target $(TARGET) -o $@ $<;

# Compile the C code with the clang llvm compiler
# do extra CLANG so first pass can return non-zero to shell and stop make
%.bc: %.c
	@$(CLANG) \
		-D__KERNEL__ -D__ASM_SYSREG_H -Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option \
		$(INCLUDE) \
		-O2 -emit-llvm -g -c $< -o - > /dev/null
	$(CLANG) \
		-D__KERNEL__ -D__ASM_SYSREG_H -Wno-unused-value -Wno-pointer-sign \
		-Wno-compare-distinct-pointer-types \
		-Wno-gnu-variable-sized-type-not-at-end \
		-Wno-address-of-packed-member -Wno-tautological-compare \
		-Wno-unknown-warning-option \
		$(INCLUDE) \
		-O2 -emit-llvm -g -c $< -o $@

# Invoke the llvm on the generated .bc code and produce bpf byte code
%.o: %.bc
	$(LLC) -march=bpf -mcpu=probe -filetype=obj $< -o $@

clean: clean_loader
	rm -f *.o *.bc

# For actually attaching BPF programs
attach:
	ip link show $(IFACE); \
	tc qdisc add dev $(IFACE) clsact; \
	for TOOL in $(BPFOBJ); do \
		tc filter add dev $(IFACE) ingress bpf da $(TOOL) sec prog verb; \
	done

# Detach the loaded programs
detach:
	ip link show $(IFACE); \
	tc filter del dev $(IFACE) ingress; \


