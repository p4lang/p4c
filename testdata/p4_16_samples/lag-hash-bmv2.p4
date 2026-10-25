/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium & Devansh Singh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Test that standard_metadata.lag_hash selects the BMv2 LAG member.
 */

#include <core.p4>
#include <v1model.p4>

header lag_test_h {
    bit<16> hash;
    bit<16> data;
}

struct metadata {
}

struct headers {
    @name("lag_test")
    lag_test_h h;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta,
                  inout standard_metadata_t standard_metadata) {
    @name("parse_lag_test") state parse_lag_test {
        packet.extract(hdr.h);
        transition accept;
    }
    @name("start") state start {
        transition parse_lag_test;
    }
}

control ingress(inout headers hdr, inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    apply {
        // Multicast into the LAG; lag_hash selects the member.
        standard_metadata.mcast_grp = 1;
        standard_metadata.lag_hash = hdr.h.hash;
    }
}

control egress(inout headers hdr, inout metadata meta,
               inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.h);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(),
         computeChecksum(), DeparserImpl()) main;
