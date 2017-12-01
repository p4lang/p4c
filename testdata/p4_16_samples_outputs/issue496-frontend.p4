#include <core.p4>
#include <v1model.p4>

header h_t {
    bit<8> f;
}

struct my_packet {
    h_t h;
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

control D() {
    apply {
    }
}

control C()(D d) {
    apply {
        d.apply();
    }
}

control MyIngress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    @name("d") D() d_0;
    @name("c") C(d_0) c_0;
    apply {
        c_0.apply();
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

