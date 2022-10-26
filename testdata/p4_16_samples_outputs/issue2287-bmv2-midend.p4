#include <core.p4>
#define V1MODEL_VERSION 20180101
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
    H          h;
    B          b;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        pkt.extract<B>(hdr.b);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action act() {
        h.h.a = 8w1;
        h.h.b = 8w1;
        h.h.c = 8w1;
        h.h.d = 8w1;
        h.h.e = 8w1;
        h.h.f = 8w1;
        h.h.g = 8w1;
        h.h.h = 8w1;
        h.h.i = 8w1;
        h.h.j = 8w1;
        h.h.k = 8w1;
        h.h.l = 8w1;
        h.h.m = 8w1;
        h.b.c = 8w1;
        h.b.d = 8w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Headers h) {
    apply {
        b.emit<ethernet_t>(h.eth_hdr);
        b.emit<H>(h.h);
        b.emit<B>(h.b);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
