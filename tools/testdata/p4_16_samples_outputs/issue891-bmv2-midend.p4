#include <core.p4>
#include <v1model.p4>

header mpls {
    bit<8> label;
}

struct my_packet {
    @name("mpls_data") 
    mpls[8] data;
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
        b.emit<mpls>(p.data[0]);
        b.emit<mpls>(p.data[1]);
        b.emit<mpls>(p.data[2]);
        b.emit<mpls>(p.data[3]);
        b.emit<mpls>(p.data[4]);
        b.emit<mpls>(p.data[5]);
        b.emit<mpls>(p.data[6]);
        b.emit<mpls>(p.data[7]);
    }
}

V1Switch<my_packet, my_metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

