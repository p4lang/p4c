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
    bit<8> e;
    bit<8> f;
    bit<8> g;
    bit<8> h;
    bit<8> i;
    bit<8> j;
    bit<8> k;
    bit<8> l;
    bit<8> m;
}
header B {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
}



struct Headers {
    ethernet_t eth_hdr;
    H h;
    B b;
}

struct Meta {
}

bit<8> function_with_side_effect(inout bit<8> val) {
    val = 1;
    return 8w2;
}

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
        bit<8> dummy_var;
        bool dummy_bool;
        dummy_var = 8w0 & function_with_side_effect(h.h.a);
        dummy_var = 8w0 * function_with_side_effect(h.h.b);
        dummy_var = 8w0 / function_with_side_effect(h.h.c);
        dummy_var = 8w0 >> function_with_side_effect(h.h.d);
        dummy_var = 8w0 << function_with_side_effect(h.h.e);
        dummy_var = 8w0 % function_with_side_effect(h.h.f);
        dummy_var = 8w0 ^ function_with_side_effect(h.h.g);
        dummy_var = 8w0 |-| function_with_side_effect(h.h.h);
        dummy_var = 8w255 |+| function_with_side_effect(h.h.i);
        dummy_var = 8w255 + function_with_side_effect(h.h.j);
        dummy_var = 8w255 | function_with_side_effect(h.h.k);
        dummy_var = 8w0 - function_with_side_effect(h.h.l);
        dummy_var = (16w1 ++ function_with_side_effect(h.h.m))[15:8];

        dummy_bool = true || bool_with_side_effect(h.b.a);
        dummy_bool = false && bool_with_side_effect(h.b.b);
        dummy_bool = function_with_side_effect(h.b.c) != function_with_side_effect(h.b.c);
        dummy_bool = function_with_side_effect(h.b.d) == function_with_side_effect(h.b.d);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
