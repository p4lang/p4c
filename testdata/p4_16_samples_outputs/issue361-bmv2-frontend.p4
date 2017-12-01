#include <core.p4>
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
    bool bv_0;
    state start {
        bv_0 = true;
        transition select(bv_0) {
            false: next;
            true: accept;
        }
    }
    state next {
        b.extract<H>(p.h);
        transition accept;
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

control MyIngress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
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

