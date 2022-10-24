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
    @name("ingress.val_0") bit<8> val;
    @name("ingress.val_1") bit<8> val_17;
    @name("ingress.val_2") bit<8> val_18;
    @name("ingress.val_3") bit<8> val_19;
    @name("ingress.val_4") bit<8> val_20;
    @name("ingress.val_5") bit<8> val_21;
    @name("ingress.val_6") bit<8> val_22;
    @name("ingress.val_7") bit<8> val_23;
    @name("ingress.val_8") bit<8> val_24;
    @name("ingress.val_9") bit<8> val_25;
    @name("ingress.val_10") bit<8> val_26;
    @name("ingress.val_11") bit<8> val_27;
    @name("ingress.val_12") bit<8> val_28;
    @name("ingress.val_14") bit<8> val_30;
    @name("ingress.val_16") bit<8> val_32;
    apply {
        val = 8w1;
        h.h.a = val;
        val_17 = 8w1;
        h.h.b = val_17;
        val_18 = 8w1;
        h.h.c = val_18;
        val_19 = 8w1;
        h.h.d = val_19;
        val_20 = 8w1;
        h.h.e = val_20;
        val_21 = 8w1;
        h.h.f = val_21;
        val_22 = 8w1;
        h.h.g = val_22;
        val_23 = 8w1;
        h.h.h = val_23;
        val_24 = 8w1;
        h.h.i = val_24;
        val_25 = 8w1;
        h.h.j = val_25;
        val_26 = 8w1;
        h.h.k = val_26;
        val_27 = 8w1;
        h.h.l = val_27;
        val_28 = 8w1;
        h.h.m = val_28;
        val_30 = 8w1;
        h.b.c = val_30;
        val_32 = 8w1;
        h.b.d = val_32;
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
        b.emit<Headers>(h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
