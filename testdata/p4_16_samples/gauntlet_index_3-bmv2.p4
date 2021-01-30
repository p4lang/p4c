#include <core.p4>
#include <v1model.p4>
bit<3> id(in bit<3> val) {
    return val;
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}

header I {
    bit<3> idx;
    bit<5> padding;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]  h;
    I i;
}

bit<8> simple_function(out bit<8> dummy) {
    return 1;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.i);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        simple_function(h.h[id(h.i.idx)].a);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.h[0]);
        pkt.emit(h.h[1]);
        pkt.emit(h.i);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

