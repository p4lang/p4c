/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// The argument of the extern function below is an instance of a generic extern.
// P4Testgen does not keep values for extern instances. It must skip such an
// argument when it evaluates the arguments of a call.

#include <core.p4>
#include <v1model.p4>

extern void observe(register<bit<32>> r);

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
}

struct metadata_t {}

parser MyParser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
                inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

control MyIngress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    register<bit<32>>(1) reg;

    apply {
        observe(reg);
    }
}

control MyEgress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t sm) {
    apply {}
}

control MyComputeChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

control MyDeparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr.ethernet);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(),
         MyDeparser()) main;
