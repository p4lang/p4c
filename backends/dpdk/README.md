# DPDK backend

The **p4c-dpdk** backend translates the P4-16 programs to DPDK API to configure
the DPDK software switch (SWX) pipeline. DPDK introduced the SWX pipeline in
the DPDK 20.11 LTS release. For more information, please refer to the release
note at https://doc.dpdk.org/guides/rel_notes/release_20_11.html.

The p4c-dpdk compiler accepts P4-16 programs written for the Portable
Switch Architecture (PSA) and Portable NIC Architecture (PNA) (see the
[P4.org specifications page](https://p4.org/specs) for the PSA and PNA
specification document).


The backend for dpdk reuses code from the p4c-bm2 "common" library for
handling the PSA/PNA architecture. Internally, it translates the PSA/PNA
program to a representation that conforms to the DPDK SWX pipeline and
generates the 'spec' file to configure the DPDK pipeline.


## How to use it?

A sample P4 program can be found in the `examples` directory.  To
generate the 'spec' file:
```bash
p4c-dpdk --arch psa vxlan.p4 -o vxlan.spec
```

To load the 'spec' file in dpdk follow the instructions in the
[Pipeline Application User Guide](https://doc.dpdk.org/guides/sample_app_ug/pipeline.html).


## Known issues
### Unsupported Language Features
- Subparsers
- Parser Value Sets

### Unsupported PSA externs and features
- egress parser, control, and deparser are not implemented, only
  ingress parser, control, and deparser.  There is no packet
  replication engine or packet buffer (the combination of which is
  sometimes called a traffic manager).
- Packet Digest
- Random
- Hash
- Timestamp
- Direct Meter
- Direct Counter
- Basic Checksum
- Packet Cloning/Recirculation/Resubmission

### DPDK target limitations
- Currently, programs written for DPDK target should limit the functionality in Ingress blocks, in case non empty  Egress blocks are present it will be ignored by default unless Hidden temporary option `--enableEgress` used.  When egress block support gets added to the DPDK target, and compiler can generate separate spec file for non empty ingress and egress blocks then option `--enableEgress` will be removed.
- DPDK architecture assumes the following signatures for programmable block of PSA. P4C-DPDK converts the input program to this form.

```P4
parser IngressParser (packet_in buffer, out H parsed_hdr, inout M user_meta);

control Ingress (inout H hdr, inout M user_meta);

control IngressDeparser (packet_out buffer, inout H hdr, in M meta);

parser EgressParser (packet_in buffer, out H parsed_hdr, inout M user_meta);

control Egress (inout H hdr, inout M user_meta);

control EgressDeparser (packet_out buffer, inout H hdr, in M meta);
```

- Size of all structure fields to be multiple of 8 bits and should not be greater than 64-bit. (This limitation is planned to be removed by grouping fields into fields which are multiple of 8-bits)
- Combination of header and metadata as table key is not allowed. (This limitation is removed by P4C by copying the differing match key fields to metadata)
- Extract instruction in DPDK target with 2 arguments expects the second argument to be the number
of bytes to be extracted into the varbit field of the header.
In P4 the second argument of the extract method is the number of bits.
Compiler generates instructions which compute the size in bytes from the value in bits.
If the value in bits is not a multiple of 8, the value is rounded down to the lower
multiple of 8 bits.
- Currently dpdk target does not support standard count and execute methods for Counter and Meter externs as defined in PSA and PNA specifications. It requires packet length as parameter in count and execute methods.
```Meter
PNA_MeterColor_t dpdk_execute(in S index, in PNA_MeterColor_t color, in bit<32> pkt_len);
```
```Counter
void count(in S index, in bit<32> pkt_len);
```

## Contacts

Han Wang <han2.wang@intel.com>

Cristian Dumitrescu <cristian.dumitrescu@intel.com>
