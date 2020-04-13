#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<32> f;
}

struct my_packet {
    H h;
}

struct my_metadata {
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    state start {
        transition accept;
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

struct tuple_0 {
    bit<32> field;
}

control MyIngress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    bit<32> x_0;
    @hidden action issue430bmv2l40() {
        hash<bit<32>, bit<32>, tuple_0, bit<32>>(x_0, HashAlgorithm.crc32, 32w0, { p.h.f ^ 32w0xffff }, 32w65536);
    }
    @hidden table tbl_issue430bmv2l40 {
        actions = {
            issue430bmv2l40();
        }
        const default_action = issue430bmv2l40();
    }
    apply {
        tbl_issue430bmv2l40.apply();
    }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
    }
}

control MyComputeChecksum(inout my_packet p, inout my_metadata m) {
    apply {
    }
}

control MyDeparser(packet_out b, in my_packet p) {
    apply {
    }
}

V1Switch<my_packet, my_metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

