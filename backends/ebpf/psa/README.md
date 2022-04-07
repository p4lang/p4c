# PSA implementation for eBPF backend

This directory implements PSA (Portable Switch Architecture) for the eBPF backend.

# Prerequisites

- Refer to the [Cilium docs](https://docs.cilium.io/en/latest/bpf/) to learn more about eBPF.
- This guide assumes at least basic familiarity with [the PSA specification](https://p4.org/p4-spec/docs/PSA.html).
- The PSA implementation inherits some mechanisms (e.g. generation of Parser and Control block) from `ebpf_model`. Please, get familiar with
[the base eBPF backend](../README.md) first.

# Design

P4 packet processing is translated into a set of eBPF programs attached to the TC hook. The eBPF programs implement packet processing defined
in a P4 program written according to the PSA model. The TC hook is used as a main engine, because it enables a full implementation of the PSA specification.
We also plan to contribute the XDP-based version of the PSA implementation that does not implement the full specification, but provides better performance.

The TC-based design of PSA for eBPF is depicted in Figure below.

![TC-based PSA-eBPF design](psa-ebpf-design.png)

- `xdp-helper` - the fixed, non-programmable "helper" program attached to the XDP hook. The role of the `xdp-helper` program is to prepare
  a packet for further processing in the TC subsystem. Why do we need the XDP helper program? Some eBPF helpers for the TC hook depend on
  the `skb->protocol` type (in particular, IPv4/IPv6 EtherType), which is read by the TC layer before a packet enters the eBPF program.
  This limitation prevents from using TC as a protocol-independent packet processing engine. If a packet arriving at the XDP level isn't
  an IPv4 packet, the XDP helper replaces it's original EtherType with IPv4 EtherType. The original EtherType is passed to TC according to 
  the XDP2TC mode specified by a user (see XDP2TC metadata section). The `tc-ingress` program reads original EtherType and puts it back into the packet. 
  We verified that this workaround enables handling other protocols in the TC layer (e.g., MPLS).
- `tc-ingress` - In the TC Ingress, the PSA Ingress pipeline as well as so-called "Traffic Manager" eBPF program is attached.
  The Ingress pipeline is composed of Parser, Control block and Deparser. The details of Parser, Control block and Deparser implementation
  will be explained further in this document. The same eBPF program in TC contains also the Traffic Manager.
  The role of Traffic Manager is to redirect traffic between the Ingress (TC) and Egress (TC).
  It is also responsible for packet replication via clone sessions or multicast groups and sending packet to CPU.
- `tc-egress` - The PSA Egress pipeline (composed of Parser, Control block and Deparser) is attached to the TC Egress hook. As there is no
  XDP hook in the Egress path, the use of TC is mandatory for the egress processing.
  
## Packet paths

### NFP (Normal Packet From Port)

Packet arriving on an interface is intercepted in the XDP hook by the `xdp-helper` program. It performs pre-processing and
packet is passed for further processing to the TC ingress. Note that there is no P4-related processing done in the `xdp-helper` program.

By default, a packet is further passed to the TC subsystem. It is done by `XDP_PASS` action and packet is further handled by `tc-ingress` program.

### RESUBMIT

The purpose of `RESUBMIT` is to transfer packet processing back to the Ingress Parser from Ingress Deparser.

We implement packet resubmission by calling main `ingress()` function (implementing the PSA Ingress pipeline) in a loop. 
The `MAX_RESUBMIT_DEPTH` variable specifies maximum number of resubmit operations (the `MAX_RESUBMIT_DEPTH` value is currently hardcoded and is set to 4).
The `resubmit` flag defines whether the `tc-ingress` program should enter next iteration (resubmit)
or break the loop. The pseudocode looks as follows:

```c
int i = 0;
int ret = TC_ACT_UNSPEC;
for (i = 0; i < MAX_RESUBMIT_DEPTH; i++) {
    out_md.resubmit = 0;
    ret = ingress(skb, &out_md);
    if (out_md.resubmit == 0) {
        break;
    }
}
```

### NU (Normal Unicast), NM (Normal Multicast), CI2E (Clone Ingress to Egress)

NU, NM and CI2E refer to process of sending packet from the PSA Ingress Pipeline (more specifically from the Traffic Manager)
to the PSA Egress pipeline. The NU path is implemented in the eBPF subsystem by invoking the `bpf_redirect()` helper from
the `tc-ingress` program. This helper sets an output port for a packet and the packet is further intercepted by the TC egress.

Both NM and CI2E require the `bpf_clone_redirect()` helper to be used. It redirects a packet to an output port, but also
clones a packet buffer, so that a packet can be copied and sent to multiple interfaces. From the eBPF program's perspective, 
`bpf_clone_redirect()` must be invoked in the loop to send packets to all ports from a clone session/multicast group.

Clone sessions or multicast groups and theirs members are stored as a BPF array map of maps (`BPF_MAP_TYPE_ARRAY_OF_MAPS`). 
The P4-eBPF compiler generates two outer BPF maps: `multicast_grp_tbl` and `clone_session_tbl`. Both of them store inner maps indexed by 
the clone session or multicast group identifier, respectively. The clone session/multicast group members (defining `egress_port`, `instance` or other parameters used by clone sessions)
are stored in the inner hash map. 

While performing the packet replication, the eBPF program does a lookup to the outer map based on the clone session/multicast group identifier and, then,
does another lookup to the inner map to find all members.

### CE2E (Clone Egress to Egress)

CE2E refers to process of copying a packet that was handled by the Egress pipeline and resubmitting the cloned packet to the Egress Parser.

CE2E is implemented by invoking `bpf_clone_redirect()` helper in the Egress path. Output ports are determined based on the
`clone_session_id` and lookup to "clone_session" BPF map, which is shared among TC ingress and egress (eBPF subsystem allows for map sharing between programs).

### Sending packet to CPU

The PSA implementation for eBPF backend assumes a special interface called `PSA_PORT_CPU` that is used for communication between
a control plane application and data plane. Sending packet to CPU does not differ significantly from normal packet unicast. 
A control plane application should listen for new packets on the interface identified by `PSA_PORT_CPU` in a P4 program. 
By redirecting a packet to `PSA_PORT_CPU` in the Ingress pipeline the packet is forwarded via Traffic Manager to the Egress pipeline and then, sent to the "CPU" interface.

### NTP (Normal packet to port)

Packets from `tc-egress` are sent out to the egress port. The egress port is determined in the Ingress pipeline and is not changed in the Egress pipeline.

Note that before a packet is sent to the output port, it's processed by `TC qdisc` first. The `TC qdisc` is the Linux QoS engine. 
The eBPF programs generated by P4-eBPF compiler sets `skb->priority` value based on the PSA `class_of_service` metadata. 
The `skb->priority` is used to interact between eBPF programs and `TC qdisc`. A user can configure different QoS behaviors via TC CLI and 
send a packet from PSA pipeline to a specific QoS class identified by `skb->priority`. 

### RECIRCULATE

The purpose of `RECIRCULATE` is to transfer packet processing back from the Egress Deparser to the Ingress Parser.

In order to implement `RECIRCULATE` we assume the existence of `PSA_PORT_RECIRCULATE` ports. Therefore, packet recirculation is simply performed by
invoking `bpf_redirect()` to the `PSA_PORT_RECIRCULATE` port with `BPF_F_INGRESS` flag to enforce processing a packet by the Ingress pipeline.

## Metadata

There are some global metadata defined for the PSA architecture. For example, `packet_path` must be shared among different pipelines.
To share a global metadata between pipelines we use `skb->cb` (control buffer), which gives us 20B that are free to use.

## XDP2TC mode

The XDP2TC mode determines how the metadata (containing original EtherType) is passed from XDP up to TC.
By default, PSA-eBPF uses the `bpf_xdp_adjust_meta()` helper to append the original
EtherType to the `skb`’s `data_meta` field, which is further read by the TC Ingress to restore the original format of the packet.
The way of passing metadata is determined by the user-configurable `--xdp2tc` compiler flag. We have noticed that some NIC
drivers does not support the `bpf_xdp_adjust_meta()` BPF helper and the default mode cannot be used. Therefore, we
come up with a more generic mode called `head`, which uses `bpf_xdp_adjust_head()` instead to prepend a packet with
metadata. In this mode, the helper must be invoked twice - in the XDP helper program to append the metadata and in
the TC Ingress to strip the metadata out of a packet. We also introduce the third mode - `cpumap`, which is an experimental features and should be used carefully.
The `cpumap` assumes that the single CPU core handles a packet in the run-to-completion mode from XDP up to the TC layer
(in other words, for a given packet, the CPU core running the TC program is the same as the one for XDP). If the above
condition is met, the `cpumap` mode uses the per-CPU BPF array map to transfer metadata from XDP to TC. Hence, the `cpumap` mode 
should only be used, if there is a guarantee that the same CPU core handles the packet in both XDP and TC hooks.
Note that the XDP helper program introduces a constant but noticeable per-packet overhead. Though, it is necessary to implement P4 processing in the TC layer.

To sum up, the `--xdp2tc` compiler flag can take the following values:
- `meta` (default) - uses the `bpf_xdp_adjust_meta()` BPF helper. It's the most efficient way and should be used wherever possible. 
- `head` - uses the `bpf_xdp_adjust_head()` BPF helper and should be used if `meta` is not supported by a NIC driver.
- `cpumap` - uses the BPF per-CPU array map. It should rather be used for testing purposes only. 

## Control-plane API

The PSA-eBPF compiler assumes that any control plane software managing eBPF programs generated by the 
P4 compiler must be in line with the Control-plane API (a kind of contract or set of instructions that must be followed
to make use of PSA-eBPF programs). The Control-plane API is summarized below, but we suggest using the [psabpf API](https://github.com/P4-Research/psabpf) that
already implements the control-plane API and exposes higher level C API. 

- **Pipeline initialization** - eBPF programs must be first loaded to the eBPF subsystem. The C files generated by the P4 compiler
are compatible with `libbpf` loader and are annotated with BTF. All eBPF objects (programs, maps) must be pinned to the BPF filesystem
under `/sys/fs/bpf/`. Once eBPF objects are loaded and pinned, a control plane application must invoke `map_initialize()` BPF function -
it can be done using `bpf_prog_test_run`. The `map_initialize()` function is auto-generated by the PSA-eBPF compiler
and configures all initial state, i.e. it initializes default actions, const entries, etc.
  
- **Table management** - a control plane software is responsible for inserting BPF map entries that 
are in line with types generated by the P4 compiler. The PSA-eBPF compiler generates C `struct` for BPF map's key and value.
(e.g. `ingress_tbl_fwd_key` and `ingress_tbl_fwd_value`). The `exact` match table is implemented as BPF hash map. 
The `lpm` match table is implemented as BPF `LPM_TRIE`. Both key and value fields must be provided in the host byte order. 

- **Clone sessions or multicast groups management** - Clone sessions or multicast groups are represented as a BPF array map of maps 
(`BPF_MAP_TYPE_ARRAY_OF_MAPS`) in the eBPF subsystem. Each entry of an outer map represents a single clone session or multicast group.
An inner map is a hash map storing clone session/multicast group members, according to the structure defined by
`struct list_key_t` (BPF map key) and `struct element` (value). To add a new clone session/mutlicast group, a control plane must add
a new element to the outer map (indexed by clone session or multicast group identifier referenced by `clone_session_id` 
or `multicast_group` in a PSA program) and initialize an inner map. To add a new clone session/multicast group member,
a con1trol plane must add new element to the inner map.

## PSA externs

### ActionProfile

[ActionProfile](https://p4.org/p4-spec/docs/PSA.html#sec-action-profile) is a table implementation that separates actions
(and its parameters) from a P4 table, introducing a level of indirection. The P4-eBPF compiler generates an additional
BPF hash map, if the Action Profile is specified for a P4 table. The additional BPF map stores the mapping between the
ActionProfile member reference and a P4 action specification. During the lookup to the P4 table with Action Profile, eBPF
program first queries the first BPF map using the match key composed from the packet fields and expects the ActionProfile
member reference to be returned. Next, the eBPF programs uses the obtained member reference as a lookup key to a second
map to retrieve the action specification. Hence, the eBPF program does one additional lookup to the additional BPF map,
if the ActionProfile is specified for a P4 table.

**Note:** As of April 2022, support for ActionProfile extern in `psabpf-ctl` CLI/API is not implemented yet. As a workaround
you can use `psabpf-ctl table` command for this extern.

### Digest

[Digests](https://p4.org/p4-spec/docs/PSA.html#sec-packet-digest) are intended to carry a small piece of user-defined data from the data plane to a control plane.
The PSA-eBPF compiler translates each Digest instance into `BPF_MAP_TYPE_QUEUE` that implements a FIFO queue.
If a deparser triggers the `pack()` method, an eBPF program inserts data defined for a Digest into the BPF queue map using `bpf_map_push_elem`.
A user space application is responsible for performing periodic queries to this map to read a Digest message. It can use either
`psabpf-ctl digest get pipe`, `psabpf_digest_get_next` from psabpf C API or `bpf_map_lookup_and_delete_elem` from `libbpf` API.

# Getting started

## Installation 

Follow standard steps for the P4 compiler to install the eBPF backend with the PSA support.

## Using PSA-eBPF

### Prerequisites

The PSA implemented for eBPF backend is verified to work with the kernel version 5.8+ and `x86-64` CPU architecture.
Moreover, make sure that the BPF filesystem is mounted under `/sys/fs/bpf`.

### Compilation

You can compile a P4-16 PSA program for eBPF in a single step using:

```bash
make -f backends/ebpf/runtime/kernel.mk BPFOBJ={output} ARGS="-DPSA_PORT_RECIRCULATE=<RECIRCULATE_PORT_IDX>" P4FILE=<P4-PROGRAM>.p4 P4C=p4c-ebpf psa
```

You can also perform compilation step by step:

```
$ p4c-ebpf --arch psa --target kernel -o out.c <program>.p4
$ clang -O2 -g -c -emit-llvm -DBTF -DPSA_PORT_RECIRCULATE=<RECIRCULATE_PORT_IDX> -c -o out.bc out.c
$ llc -march=bpf -filetype=obj -o out.o out.bc
```

The above steps generate `out.o` BPF object file that can be loaded to the kernel. 

### psabpf API and psabpf-ctl

We provide the `psabpf` C API and the `psabpf-ctl` CLI tool that can be used to manage eBPF programs generated by P4-eBPF compiler.
To install the CLI tool, follow the guide in [the psabpf repository](https://github.com/P4-Research/psabpf). Use `psabpf-ctl help` to get all possible commands.

**Note!** Although eBPF objects can be loaded and managed by other tools (e.g. `bpftool`), we recommend using `psabpf-ctl`. Some features
(e.g., default actions) will only work when using `psabpf-ctl`.

To load eBPF programs generated by P4-eBPF compiler run:

```bash
psabpf-ctl pipeline load id <PIPELINE-ID> out.o
```

`PIPELINE-ID` is a user-defined value used to uniquely identify PSA-eBPF pipeline (we are going to support for multiple PSA-eBPF pipelines running in parallel).
In the next step, for each interface that should be attached to PSA-eBPF run:

```bash
psabpf-ctl add-port pipe <PIPELINE-ID> dev <INTF>
```

## Running PTF tests

PSA implementation for eBPF backend is covered by a set of PTF tests that verify a correct behavior of various PSA mechanisms. 
The test scripts, PTF test cases and test P4 programs are located under `backends/ebpf/tests`. The tests must be executed from this directory.

To run all PTF tests:

```
sudo ./test.sh
```

You can also specify a single PTF test to run:

```
sudo ./test.sh test.BridgedMetadataPSATest
```

It might be also useful to enable tracing for troubleshooting with `bpftool prog tracelog`:

```
sudo ./test.sh --trace=on
```

## Troubleshooting

The PSA implementation for eBPF backend generates standard BPF objects that can be inspected using `bpftool`.

To troubleshoot PSA-eBPF program you will probably need `bpftool`. Follow the steps below to install it.

You should be able to see `bpftool help`:

```bash
$ bpftool help
  Usage: bpftool [OPTIONS] OBJECT { COMMAND | help }
         bpftool batch file FILE
         bpftool version
  
         OBJECT := { prog | map | link | cgroup | perf | net | feature | btf | gen | struct_ops | iter }
         OPTIONS := { {-j|--json} [{-p|--pretty}] | {-f|--bpffs} |
                      {-m|--mapcompat} | {-n|--nomount} }
```

Refer to [the bpftool guide](https://manpages.ubuntu.com/manpages/focal/man8/bpftool-prog.8.html) for more examples how to use it.

# TODO / Limitations

We list the known bugs/limitations below. Refer to the Roadmap section for features planned in the near future.

- Larger bit fields (e.g. IPv6 addresses) may not work properly.
- We noticed that `bpf_xdp_adjust_meta()` isn't implemented by some NIC drivers, so the PSA impl may not work 
with some NICs. So far, we have verified the correct behavior with Intel 82599ES.
- `lookahead()` with bit fields (e.g., `bit<16>`) doesn't work.
- `@atomic` operation is not supported yet.
- `psa_idle_timeout` is not supported yet.
- DirectCounter and DirectMeter externs are not supported for P4 tables with implementation (ActionProfile).
- The `xdp2tc=head` mode works only for packets larger than 34 bytes (the size of Ethernet and IPv4 header).

# Roadmap

## Planned features

All the below features are already implemented and will be contributed to the P4 compiler in subsequent pull requests.

- **XDP support.** The current version of P4-eBPF compiler leverages the BPF TC hook for P4-programmable packet processing. 
The TC subsystem enables implementation of the full PSA specification, contrary to XDP, but offers lower throughput. We're going to 
contribute the XDP-based version of P4-eBPF that is not fully spec-compliant, but provides higher throughput.
- **XDP2TC mode.** the generated eBPF programs use `bpf_xdp_adjust_meta()` to transfer original EtherType from XDP to TC. This mode 
may not be supported by some NIC drivers. We will add two other modes that can be used alternatively. 
- **ValueSet support.** The parser generated by P4-eBPF does not support ValueSet. We plan to contribute ValueSet implementation for eBPF.
- **All PSA externs.** We plan to contribute implementation of [all PSA externs defined by the PSA specification](https://p4.org/p4-spec/docs/PSA.html#sec-psa-externs). 
- **Ternary matching.** The PSA implementation for eBPF backend currently supports `exact` and `lpm` only. We will add support for `ternary` match kind. 

## Long-term goals

The below features are not implemented yet, but they are considered for the future extensions:

- **Range matching.** P4-eBPF compiler does not support `range` match kind and there is a further investigation needed on how to implement range matching for eBPF programs.
- **Optional matching.** P4-eBPF compiler does not support `optional` match kind yet. However, it can be implemented based on the same algorithm that is used for ternary matching.
- **Investigate support for PNA.** We plan to investigate the PNA implementation for eBPF backend. We believe that the PNA implementation can be significantly based on the PSA implementation. 
- **Meet parity with the latest version of Linux kernel.** The latest Linux kernel brings a few improvements/extensions to eBPF subsystem.
We plan to incorporate them to the P4-eBPF compiler to extend functionalities or improve performance.
- **P4Runtime support.** Currently, PSA-eBPF programs can only be managed by `psabpf API`. We plan to integrate PSA-eBPF with the P4Runtime software stack (e.g., Stratum, TDI or P4-OvS).

## Support

To report any other kind of problem, feel free to open a GitHub Issue or reach out to the project maintainers on the P4 Community Slack or via email.

Project maintainers:

- Tomasz Osiński (tomasz [at] opennetworking.org / osinstom [at] gmail.com)
- Mateusz Kossakowski (mateusz.kossakowski [at] orange.com / mateusz.kossakowski.10 [at] gmail.com)
- Jan Palimąka (jan.palimaka [at] orange.com / jan.palimaka95 [at] gmail.com)
