#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

enum bit<12> E {
    e1 = 12,
    e2 = 12,
    e3 = 14
}

header H1 {
    bit<1>  b1;
    bool    b2;
    bit<1>  b3;
    bool    b4;
    bit<12> b5;
    bit<12> b6;
    E       en1;
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
    state start {
        b.extract(p.h1);
        p.h1.b5 = (bit<12>)p.h1.en1;
        p.h1.en1 = (E)p.h1.b6;
        p.h1.b1 = (bit<1>)p.h1.b2;
        p.h1.b2 = (bool)p.h1.b3;
        p.h1.b3 = (bit<1>)p.h1.b4;
        transition accept;
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
