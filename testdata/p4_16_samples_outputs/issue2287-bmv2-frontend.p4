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
    @name("ingress.dummy_var") bit<8> dummy_var_0;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.tmp_6") bit<8> tmp_6;
    @name("ingress.tmp_9") bit<8> tmp_9;
    @name("ingress.tmp_12") bit<8> tmp_12;
    @name("ingress.tmp_15") bit<8> tmp_15;
    @name("ingress.tmp_18") bit<8> tmp_18;
    @name("ingress.tmp_21") bit<8> tmp_21;
    @name("ingress.tmp_24") bit<8> tmp_23;
    @name("ingress.tmp_27") bit<8> tmp_24;
    @name("ingress.tmp_29") bit<8> tmp_25;
    @name("ingress.tmp_31") bit<8> tmp_26;
    @name("ingress.tmp_34") bit<8> tmp_27;
    @name("ingress.tmp_35") bit<8> tmp_28;
    @name("ingress.tmp_38") bit<8> tmp_29;
    @name("ingress.tmp_39") bit<8> tmp_30;
    @name("ingress.val_0") bit<8> val;
    @name("ingress.hasReturned") bool hasReturned;
    @name("ingress.retval") bit<8> retval;
    @name("ingress.val_1") bit<8> val_17;
    @name("ingress.hasReturned") bool hasReturned_1;
    @name("ingress.retval") bit<8> retval_1;
    @name("ingress.val_2") bit<8> val_18;
    @name("ingress.hasReturned") bool hasReturned_2;
    @name("ingress.retval") bit<8> retval_2;
    @name("ingress.val_3") bit<8> val_19;
    @name("ingress.hasReturned") bool hasReturned_3;
    @name("ingress.retval") bit<8> retval_3;
    @name("ingress.val_4") bit<8> val_20;
    @name("ingress.hasReturned") bool hasReturned_4;
    @name("ingress.retval") bit<8> retval_4;
    @name("ingress.val_5") bit<8> val_21;
    @name("ingress.hasReturned") bool hasReturned_5;
    @name("ingress.retval") bit<8> retval_5;
    @name("ingress.val_6") bit<8> val_22;
    @name("ingress.hasReturned") bool hasReturned_6;
    @name("ingress.retval") bit<8> retval_6;
    @name("ingress.val_7") bit<8> val_23;
    @name("ingress.hasReturned") bool hasReturned_7;
    @name("ingress.retval") bit<8> retval_7;
    @name("ingress.val_8") bit<8> val_24;
    @name("ingress.hasReturned") bool hasReturned_8;
    @name("ingress.retval") bit<8> retval_8;
    @name("ingress.val_9") bit<8> val_25;
    @name("ingress.hasReturned") bool hasReturned_9;
    @name("ingress.retval") bit<8> retval_9;
    @name("ingress.val_10") bit<8> val_26;
    @name("ingress.hasReturned") bool hasReturned_10;
    @name("ingress.retval") bit<8> retval_10;
    @name("ingress.val_11") bit<8> val_27;
    @name("ingress.hasReturned") bool hasReturned_11;
    @name("ingress.retval") bit<8> retval_11;
    @name("ingress.val_12") bit<8> val_28;
    @name("ingress.hasReturned") bool hasReturned_12;
    @name("ingress.retval") bit<8> retval_12;
    @name("ingress.val_13") bit<8> val_29;
    @name("ingress.hasReturned") bool hasReturned_13;
    @name("ingress.retval") bit<8> retval_13;
    @name("ingress.val_14") bit<8> val_30;
    @name("ingress.hasReturned") bool hasReturned_14;
    @name("ingress.retval") bit<8> retval_14;
    @name("ingress.val_15") bit<8> val_31;
    @name("ingress.hasReturned") bool hasReturned_15;
    @name("ingress.retval") bit<8> retval_15;
    @name("ingress.val_16") bit<8> val_32;
    @name("ingress.hasReturned") bool hasReturned_16;
    @name("ingress.retval") bit<8> retval_16;
    apply {
        val = h.h.a;
        hasReturned = false;
        val = 8w1;
        hasReturned = true;
        retval = 8w2;
        h.h.a = val;
        tmp_0 = retval;
        val_17 = h.h.b;
        hasReturned_1 = false;
        val_17 = 8w1;
        hasReturned_1 = true;
        retval_1 = 8w2;
        h.h.b = val_17;
        tmp_3 = retval_1;
        val_18 = h.h.c;
        hasReturned_2 = false;
        val_18 = 8w1;
        hasReturned_2 = true;
        retval_2 = 8w2;
        h.h.c = val_18;
        tmp_6 = retval_2;
        val_19 = h.h.d;
        hasReturned_3 = false;
        val_19 = 8w1;
        hasReturned_3 = true;
        retval_3 = 8w2;
        h.h.d = val_19;
        tmp_9 = retval_3;
        val_20 = h.h.e;
        hasReturned_4 = false;
        val_20 = 8w1;
        hasReturned_4 = true;
        retval_4 = 8w2;
        h.h.e = val_20;
        tmp_12 = retval_4;
        val_21 = h.h.f;
        hasReturned_5 = false;
        val_21 = 8w1;
        hasReturned_5 = true;
        retval_5 = 8w2;
        h.h.f = val_21;
        tmp_15 = retval_5;
        val_22 = h.h.g;
        hasReturned_6 = false;
        val_22 = 8w1;
        hasReturned_6 = true;
        retval_6 = 8w2;
        h.h.g = val_22;
        dummy_var_0 = retval_6;
        val_23 = h.h.h;
        hasReturned_7 = false;
        val_23 = 8w1;
        hasReturned_7 = true;
        retval_7 = 8w2;
        h.h.h = val_23;
        tmp_18 = retval_7;
        val_24 = h.h.i;
        hasReturned_8 = false;
        val_24 = 8w1;
        hasReturned_8 = true;
        retval_8 = 8w2;
        h.h.i = val_24;
        tmp_21 = retval_8;
        val_25 = h.h.j;
        hasReturned_9 = false;
        val_25 = 8w1;
        hasReturned_9 = true;
        retval_9 = 8w2;
        h.h.j = val_25;
        tmp_23 = retval_9;
        val_26 = h.h.k;
        hasReturned_10 = false;
        val_26 = 8w1;
        hasReturned_10 = true;
        retval_10 = 8w2;
        h.h.k = val_26;
        tmp_24 = retval_10;
        val_27 = h.h.l;
        hasReturned_11 = false;
        val_27 = 8w1;
        hasReturned_11 = true;
        retval_11 = 8w2;
        h.h.l = val_27;
        tmp_25 = retval_11;
        val_28 = h.h.m;
        hasReturned_12 = false;
        val_28 = 8w1;
        hasReturned_12 = true;
        retval_12 = 8w2;
        h.h.m = val_28;
        tmp_26 = retval_12;
        val_29 = h.b.c;
        hasReturned_13 = false;
        val_29 = 8w1;
        hasReturned_13 = true;
        retval_13 = 8w2;
        h.b.c = val_29;
        tmp_27 = retval_13;
        val_30 = h.b.c;
        hasReturned_14 = false;
        val_30 = 8w1;
        hasReturned_14 = true;
        retval_14 = 8w2;
        h.b.c = val_30;
        tmp_28 = retval_14;
        val_31 = h.b.d;
        hasReturned_15 = false;
        val_31 = 8w1;
        hasReturned_15 = true;
        retval_15 = 8w2;
        h.b.d = val_31;
        tmp_29 = retval_15;
        val_32 = h.b.d;
        hasReturned_16 = false;
        val_32 = 8w1;
        hasReturned_16 = true;
        retval_16 = 8w2;
        h.b.d = val_32;
        tmp_30 = retval_16;
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

