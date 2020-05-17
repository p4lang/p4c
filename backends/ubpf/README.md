# Introduction to uBPF Backend

The **p4c-ubpf** compiler allows to translate P4 programs into the uBPF programs. We use the uBPF implementation provided 
by [the P4rt-OVS switch](https://github.com/Orange-OpenSource/p4rt-ovs). The uBPF VM is based on the
open-source implementation provided by [IOVisor](https://github.com/iovisor/ubpf).

The P4-to-uBPF compiler accepts only the P4_16 programs written for the [ubpf_model.p4](../ubpf/p4include/ubpf_model.p4) architecture model.

The backend for uBPF is mostly based on [P4-to-eBPF compiler](../ebpf/README.md). In fact, it implements the same concepts, but
generates C code, which is compatible with the user space BPF implementation. 

## Background

### P4

Please, refer to [the overview of P4 written in eBPF Backend](../ebpf#p4).

### uBPF

**Why uBPF?** The uBPF Virtual Machine can be used in any solution implementing the kernel bypass (e.g. DPDK apps).

The uBPF project re-implements the [eBPF](../ebpf#ebpf) kernel-based Virtual Machine. While the BPF programs are 
intented to run in the kernel, the uBPF project enables running the BPF programs in user-space applications. It contains 
eBPF assembler, disassembler, interpreter, and JIT compiler for x86-64.

Moreover, contrary to the eBPF implementation, uBPF is not licensed under GPL. The uBPF implementation is licensed under
Apache License, version 2.0. 

## Compiling P4 to uBPF

The scope of the uBPF backend is wider than the scope of the eBPF backend. Except for simple packet filtering the 
P4-to-uBPF compiler supports also P4 registers and programmable actions including packet's modifications and tunneling. For further details
refer to [uBPF architecture model](p4include/ubpf_model.p4).

**Note!** Due to the reason that the `standard_metadata` has been introduced to the uBPF model at 15th of May 2020 the old P4 programs
will not work anymore. You should update your P4 program to the latest architecture model. Alternatively, you can also specify the old version of uBPF model:
`#define UBPF_MODEL_VERSION 20200304` before `#include <ubpf_model.p4>`. 

The current version of the P4-to-uBPF compiler translates P4_16 programs to programs written in the C language. This
program is compatible with the uBPF VM and the `clang` compiler can be used to generate uBPF bytecode.

### Translation between P4 and C

We follow the convention of the P4-to-eBPF compiler so the parser translation is presented in the table 
[Translating parsers](../ebpf#translating-parsers) from P4-to-eBPF. The translation of match-action pipelines is presented
in the [Translating match-action pipelines](../ebpf#translating-match-action-pipelines) table from P4-to-eBPF.

However, we introduced some modifications, which are listed below:

* The generated code uses user-level data types (e.g. uint8_t, etc.). 
* Methods to extract packet fields (e.g. load_dword, etc.) have been re-implemented to use user-space data types.
* The uBPF helpers are imported into the C programs.
* We have added `mark_to_drop()` extern to the `ubpf` model, so that packets to drop are marked in the P4-native way.
* We have added support for P4 registers implemented as BPF maps

### How to use?

The sample P4 programs are located in `examples/` directory. We have tested them with the [P4rt-OVS](https://github.com/Orange-OpenSource/p4rt-ovs) switch - 
the Open vSwitch that can be extended with BPF programs at runtime. See [the detailed tutorial](./docs/EXAMPLES.md) on how to run and test those examples.

In order to generate the C code use the following command:

`p4c-ubpf PROGRAM.p4 -o out.c`

This command will generate out.c and the corresponding out.h file containing definitions of the packet structures and BPF maps.

Once the C program is generated it can be compiled using:

`clang -O2 -target bpf -c out.c -o /tmp/out.o`

The output file (`out.o`) can be injected to the uBPF VM. 

#### Custom C extern functions

The P4 to uBPF compiler allows to define custom C extern functions and call them from P4 program as P4 action.

The design of this feature is identical to `p4c-ebpf`. See [the P4 to eBPF documentation](../ebpf/README.md#how-to-inject-custom-extern-function-to-the-generated-ebpf-program) 
to learn how to use this feature. Note that the C extern function written for `p4c-ubpf` must be compatible with userspace BPF VM.

### Known limitations

* No support for some P4 constructs (meters, counters, etc.)

### Contact

Tomasz Osiński &lt;tomasz.osinski2@orange.com&gt;

Mateusz Kossakowski &lt;mateusz.kossakowski@orange.com&gt;





