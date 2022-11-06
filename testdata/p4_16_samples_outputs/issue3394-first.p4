#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header header_t {
}

struct metadata {
}

parser MyParser(packet_in packet, out header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control C1(int a, inout bit<8> b) {
    apply {
        if (b == (bit<8>)a) {
            b = (bit<8>)a + 8w1;
        } else {
            b = (bit<8>)(1 + a);
        }
    }
}

control MyIngress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("C1") C1() C1_inst;
    apply {
        bit<8> r = 8w1;
        C1_inst.apply(3, r);
    }
}

control MyVerifyChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

control MyEgress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in header_t hdr) {
    apply {
    }
}

control MyComputeChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<header_t, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
