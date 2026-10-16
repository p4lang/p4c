// SPDX-FileCopyrightText: 2026 The P4 Language Consortium
// SPDX-License-Identifier: Apache-2.0

#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> etherType;
}

struct headers_t { ethernet_t ethernet; }
struct metadata_t { }

parser ParserImpl(packet_in packet, out headers_t hdr,
                  inout metadata_t meta, inout standard_metadata_t sm) {
    state start {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control VerifyChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control IngressImpl(inout headers_t hdr, inout metadata_t meta,
                    inout standard_metadata_t sm) {
    apply {
        sm.egress_spec = 1;
        // Permit only IPv4 packets to this destination. Every other packet
        // must reach mark_to_drop, including an unmatched switch expression.
        if (hdr.ethernet.dst == 48w0x001122334455) {
            switch (hdr.ethernet.etherType) {
                16w0x0800: { return; }
#ifdef EXPLICIT_DEFAULT
                default: { }
#endif
            }
        }
        mark_to_drop(sm);
    }
}

control EgressImpl(inout headers_t hdr, inout metadata_t meta,
                   inout standard_metadata_t sm) {
    apply { }
}

control ComputeChecksumImpl(inout headers_t hdr, inout metadata_t meta) {
    apply { }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply { packet.emit(hdr.ethernet); }
}

V1Switch(ParserImpl(), VerifyChecksumImpl(), IngressImpl(), EgressImpl(),
         ComputeChecksumImpl(), DeparserImpl()) main;
