# Introduction to uBPF Backend

This is a back-end, which generates the code to be consumed by the userspace BPF VM - https://github.com/iovisor/ubpf .

The P4-to-uBPF compiler accepts only the P4_16 programs written for the `ubpf_filter_model.p4` architecture model.

The backend for uBPF is mostly based on [P4-to-eBPF compiler](../ebpf/README.md). In fact, it implements the same concepts, but
generates the C code, which is compatible with the user-space BPF implementation. 

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

The scope of the uBPF backend is similar to the scope of the eBPF backend. It means that initially P4-to-uBPF compiler
supports only simple packet filtering. 

We will continuously work to provide wider range of functionalities for P4-to-uBPF.

The current version of the P4-to-uBPF compiler translaters P4_16 programs to programs written in the C language. This
program is compatible with the uBPF VM and the clang compiler can be used to generate uBPF bytecode.

### Translation between P4 and C

We follow the convention of the P4-to-eBPF compiler so the parser translation is presented in the table 
[Translating parsers](../ebpf#translating-parsers) from P4-to-eBPF. The translation of match-action pipelines is presented
in the [Translating match-action pipelines](../ebpf#translating-match-action-pipelines) table from P4-to-eBPF.

However, we introduced some modifications, which are listed below:

* There are user-space data types used (e.g. uint8_t, etc.). 
* Methods to extract packet fields (e.g. load_dword, etc.) has been re-implemented to use user-space data types.
* The uBPF helpers are imported into the C programs.
* We have added `mark_to_drop()` extern to the `ubpfFilter` model, so that packets to drop are marked in the P4-native way.

### How to use?

The P4 programs for P4-to-uBPF compiler must be written for the `ubpf_filter_model.p4`.

In order to generate the C code use the following command:

`p4c-ubpf PROGRAM.p4 -o out.c`

This command will generate out.c and the corresponding out.h file containing definitions of the packet structures and BPF maps.

Once the C program is generated it can be compiled using:

`clang -O2 -target bpf -c out.c -o /tmp/out.o`

The output file (`out.o`) can be injected to the uBPF VM. 

### Contact

Tomasz Osiński &lt;tomasz.osinski2@orange.com&gt;

Mateusz Kossakowski &lt;mateusz.kossakowski@orange.com&gt;





