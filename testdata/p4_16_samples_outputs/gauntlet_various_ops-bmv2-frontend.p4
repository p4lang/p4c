#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header OVERFLOW {
    bit<8> a;
    bit<8> b;
}

header UNDERFLOW {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

header MOD {
    bit<4> a;
    bit<4> b;
    bit<4> c;
    bit<4> d;
}

header RSH {
    bit<4> a;
    int<4> b;
    bit<4> c;
    int<4> d;
    bit<4> e;
    bit<4> g;
    bit<8> h;
}

header LSH {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
    bit<8> e;
}

header COMPARE {
    bit<8> a;
    bit<8> b;
    bit<8> c;
    bit<8> d;
    bit<8> e;
}

header DIV {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

header BOOL {
    bool   a;
    bit<7> padding;
}

struct Headers {
    ethernet_t eth_hdr;
    OVERFLOW   overflow;
    UNDERFLOW  underflow;
    RSH        rshift;
    LSH        lshift;
    MOD        mod;
    COMPARE    comp;
    DIV        div;
    BOOL       b;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<OVERFLOW>(hdr.overflow);
        pkt.extract<UNDERFLOW>(hdr.underflow);
        pkt.extract<RSH>(hdr.rshift);
        pkt.extract<LSH>(hdr.lshift);
        pkt.extract<MOD>(hdr.mod);
        pkt.extract<COMPARE>(hdr.comp);
        pkt.extract<DIV>(hdr.div);
        pkt.extract<BOOL>(hdr.b);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("ingress.tmp") bit<4> tmp_0;
    apply {
        h.overflow.a = 8w255;
        h.overflow.b = 8w3;
        h.underflow.a = 8w0;
        h.underflow.b = 8w3;
        h.underflow.c = 8w0;
        h.mod.a = 4w1;
        h.mod.b = 4w1;
        h.mod.c = 4w1;
        h.mod.d = 4w1;
        tmp_0 = 4w15;
        h.rshift.a = tmp_0 >> 1;
        h.rshift.b = 4s1;
        h.rshift.c = 4w3;
        h.rshift.d = -4s2;
        h.rshift.e = tmp_0 >> 2;
        h.rshift.g = 4w0;
        h.rshift.h = 8w15;
        h.lshift.a = 8w0;
        h.lshift.b = 8w0;
        h.lshift.c = 8w2;
        h.lshift.d = 8w0;
        h.lshift.e = 8w1;
        h.comp.a = 8w1;
        h.comp.b = 8w1;
        h.comp.c = 8w1;
        h.div.a = 8w0;
        h.div.b = 8w2;
        h.div.c = 8w254;
        h.b.a = true;
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

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
