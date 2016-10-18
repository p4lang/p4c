# The BMv2 Simple Switch target

The bmv2 framework lets developpers implement their own P4-programmable
architecture as a software switch. The simple_switch architecture is the
de-facto architecture for most users, as it is roughly equivalent to the
"abstract switch model" described in the P4_14 spec.

This document aims at providing P4 programmers with important information
regarding the simple_switch architecture.

## Intrinsic metadata

Each architecture usually defines its own intrinsic metadata fields, which are
used in addition to the standard metadata fields to offer more advanced
features. In the case of simple_switch, we have two separate intrinsic metadata
headers. These headers are not strictly required by the architecture as it is
possible to write a P4 program and run it through simple_switch without them
being defined. However, their presence is required to enable some features of
simple_switch. For most of these fields, there is no strict requirement as to
the bitwidth, but we recommend that you follow our suggestions below. Some of
these intrinsic metadata fields can be accessed (read and / or write) directly,
others should only be accessed through primitive actions.

### `intrinsic_metadata` header

We recommend that you define and instantiate this metadata header for every P4
program that you write for the simple_switch architecture, with the following
P4-14 code:
```
header_type intrinsic_metadata_t {
    fields {
        ingress_global_timestamp : 48;
        lf_field_list : 8;
        mcast_grp : 16;
        egress_rid : 16;
        resubmit_flag : 8;
        recirculate_flag : 8;
    }
}
metadata intrinsic_metadata_t intrinsic_metadata;
```
- `ingress_global_timestamp`: a timestamp, in microseconds, set when the packet
shows up on ingress. The clock is set to 0 every time the switch starts. This
field can be read directly from either pipeline (ingress and egress) but should
not be written to.
- `lf_field_list`: used to store the learn id when calling `generate_digest`; do
not access directly.
- `mcast_grp`: needed for the multicast feature. This field needs to be written
in the ingress pipeline when you wish the packet to be multicast. A value of 0
means no multicast. This value must be one of a valid multicast group configured
through bmv2 runtime interfaces.
- `egress_rid`: needed for the multicast feature. This field is only valid in
the egress pipeline and can only be read from. It is used to uniquely identify
multicast copies of the same ingress packet.
- `resubmit_flag`: should not be accessed directly. It is set by the `resubmit`
action primitive and is required for the resubmit feature. As a remainder,
`resubmit` needs to be called in the ingress pipeline.
- `recirculate_flag`: should not be accessed directly. It is set by the
`recirculate` action primitive and is required for the recirculate feature. As a
remainder, `recirculate` needs to be called from the egress pipeline.

### `queueing_metadata` header

You only need to define this P4 header if you want to access queueing
information - as a remainder, the packet is queued between the ingress and
egress pipelines. Note that this header is "all or nothing". Either you do not
define it or you define it with all its fields (there are 4 of them). We
recommend that you use the following P4_14 code:
```
header_type queueing_metadata_t {
    fields {
        enq_timestamp : 48;
        enq_qdepth : 16;
        deq_timedelta : 32;
        deq_qdepth : 16;
    }
}
metadata queueing_metadata_t queueing_metadata;
```
Of course, all of these fields can only be accessed from the egress pipeline and
they are read-only.
- `enq_timestamp`: a timestamp, in microseconds, set when the packet is first
enqueued.
- `enq_qdepth`: the depth of the queue when the packet was first enqueued.
- `deq_timedelta`: the time, in microseconds, that the packet spent in the
queue.
- `deq_qdepth`: the depth of queue when the packet was dequeued.

## Supported primitive actions

We mostly support the standard P4_14 primitive actions. One difference is that
optional parameters are not supported in bmv2, so all parameters are always
required (see `resubmit` for example).
The full list of primitives can be seen in this [C++ source file]
(../targets/simple_switch/primitives.cpp).
