/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header H {
    bit<8> value;
    bit<8> offset;
    bit<8> result;
}

struct Headers { H h; }
struct Metadata {}

parser SliceParser(packet_in pkt, out Headers hdr, inout Metadata meta,
                   inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.h);
        transition accept;
    }
}

control Verify(inout Headers hdr, inout Metadata meta) { apply {} }

control SliceIngress(inout Headers hdr, inout Metadata meta,
                     inout standard_metadata_t sm) {
    apply {
        // A slice below the shift amount can include both data and zero-filled bits.
        hdr.h.result = (bit<8>) ((hdr.h.value << 8w4)[hdr.h.offset +: 4]);
        sm.egress_spec = 0;
    }
}

control SliceEgress(inout Headers hdr, inout Metadata meta,
                    inout standard_metadata_t sm) { apply {} }

control Compute(inout Headers hdr, inout Metadata meta) { apply {} }

control SliceDeparser(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit(hdr);
    }
}

V1Switch(SliceParser(), Verify(), SliceIngress(), SliceEgress(), Compute(), SliceDeparser()) main;
