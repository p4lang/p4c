#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
}

header B {
    bit<8> a;
    bit<8> b;
}

bit<8> do_function(out bit<8> val) {
    return val;
}

struct Headers {
    ethernet_t eth_hdr;
    H h;
    B b;
}

struct Meta {}

bool bool_with_side_effect(inout bit<8> val) {
    val = 1;
    return true;
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h);
        pkt.extract(hdr.b);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        bool dummy_bool;
        dummy_bool = true || bool_with_side_effect(h.h.a);
        dummy_bool = (false || true) || bool_with_side_effect(h.h.b);
        dummy_bool = false && bool_with_side_effect(h.h.c);
        dummy_bool = (true && false) && bool_with_side_effect(h.h.d);
        h.b.a = (8w1 != do_function(h.b.b) || false) ? 8w1 : 8w2;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

