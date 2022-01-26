#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H1 {
    bit<1> b1;
    bool   b2;
}

header H {
    bit<32> f;
}

struct my_packet {
    H  h;
    H1 h1;
}

struct my_metadata {
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    bool bv = true;
    state start {
        b.extract(p.h1);
        p.h1.b2 = false;
        p.h1.b1 = (bit<1>)bv;
        p.h1.b2 = (bool)p.h1.b1;
        transition select(bv) {
            false: reject;
            true: accept;
        }
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

control C() {
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

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

