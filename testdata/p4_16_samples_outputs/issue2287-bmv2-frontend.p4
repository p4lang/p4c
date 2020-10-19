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
    @name("ingress.tmp") bit<8> tmp;
    @name("ingress.tmp_0") bit<8> tmp_0;
    @name("ingress.tmp_1") bit<8> tmp_1;
    @name("ingress.tmp_2") bit<8> tmp_2;
    @name("ingress.tmp_3") bit<8> tmp_3;
    @name("ingress.tmp_4") bit<8> tmp_4;
    @name("ingress.tmp_5") bit<8> tmp_5;
    @name("ingress.tmp_6") bit<8> tmp_6;
    @name("ingress.tmp_7") bit<8> tmp_7;
    @name("ingress.tmp_8") bit<8> tmp_8;
    @name("ingress.tmp_9") bit<8> tmp_9;
    @name("ingress.tmp_10") bit<8> tmp_10;
    @name("ingress.tmp_11") bit<8> tmp_11;
    @name("ingress.tmp_12") bit<8> tmp_12;
    @name("ingress.tmp_13") bit<8> tmp_13;
    @name("ingress.tmp_14") bit<8> tmp_14;
    @name("ingress.tmp_15") bit<8> tmp_15;
    @name("ingress.tmp_16") bit<8> tmp_16;
    @name("ingress.tmp_17") bit<8> tmp_17;
    @name("ingress.tmp_18") bit<8> tmp_18;
    @name("ingress.tmp_19") bit<8> tmp_19;
    @name("ingress.tmp_20") bit<8> tmp_20;
    @name("ingress.tmp_21") bit<8> tmp_21;
    @name("ingress.tmp_22") bit<8> tmp_22;
    @name("ingress.tmp_23") bit<8> tmp_23;
    @name("ingress.tmp_24") bit<8> tmp_24;
    @name("ingress.tmp_25") bit<8> tmp_25;
    @name("ingress.tmp_26") bit<8> tmp_26;
    @name("ingress.tmp_27") bit<8> tmp_27;
    @name("ingress.tmp_28") bit<8> tmp_28;
    @name("ingress.tmp_29") bit<8> tmp_29;
    @name("ingress.tmp_30") bit<16> tmp_30;
    @name("ingress.tmp_31") bit<8> tmp_31;
    @name("ingress.tmp_32") bit<24> tmp_32;
    @name("ingress.tmp_33") bit<8> tmp_33;
    @name("ingress.tmp_34") bit<8> tmp_34;
    @name("ingress.tmp_35") bit<8> tmp_35;
    @name("ingress.tmp_36") bool tmp_36;
    @name("ingress.tmp_37") bit<8> tmp_37;
    @name("ingress.tmp_38") bit<8> tmp_38;
    @name("ingress.tmp_39") bit<8> tmp_39;
    @name("ingress.tmp_40") bool tmp_40;
    apply {
        tmp = 8w0;
        {
            @name("ingress.val_0") bit<8> val_0 = h.h.a;
            @name("ingress.hasReturned") bool hasReturned = false;
            @name("ingress.retval") bit<8> retval;
            val_0 = 8w1;
            hasReturned = true;
            retval = 8w2;
            h.h.a = val_0;
            tmp_0 = retval;
        }
        tmp_1 = tmp & tmp_0;
        tmp_2 = 8w0;
        {
            @name("ingress.val_1") bit<8> val_1 = h.h.b;
            @name("ingress.hasReturned") bool hasReturned_1 = false;
            @name("ingress.retval") bit<8> retval_1;
            val_1 = 8w1;
            hasReturned_1 = true;
            retval_1 = 8w2;
            h.h.b = val_1;
            tmp_3 = retval_1;
        }
        tmp_4 = tmp_2 * tmp_3;
        tmp_5 = 8w0;
        {
            @name("ingress.val_2") bit<8> val_2 = h.h.c;
            @name("ingress.hasReturned") bool hasReturned_2 = false;
            @name("ingress.retval") bit<8> retval_2;
            val_2 = 8w1;
            hasReturned_2 = true;
            retval_2 = 8w2;
            h.h.c = val_2;
            tmp_6 = retval_2;
        }
        tmp_7 = tmp_5 / tmp_6;
        tmp_8 = 8w0;
        {
            @name("ingress.val_3") bit<8> val_3 = h.h.d;
            @name("ingress.hasReturned") bool hasReturned_3 = false;
            @name("ingress.retval") bit<8> retval_3;
            val_3 = 8w1;
            hasReturned_3 = true;
            retval_3 = 8w2;
            h.h.d = val_3;
            tmp_9 = retval_3;
        }
        tmp_10 = tmp_8 >> tmp_9;
        tmp_11 = 8w0;
        {
            @name("ingress.val_4") bit<8> val_4 = h.h.e;
            @name("ingress.hasReturned") bool hasReturned_4 = false;
            @name("ingress.retval") bit<8> retval_4;
            val_4 = 8w1;
            hasReturned_4 = true;
            retval_4 = 8w2;
            h.h.e = val_4;
            tmp_12 = retval_4;
        }
        tmp_13 = tmp_11 << tmp_12;
        tmp_14 = 8w0;
        {
            @name("ingress.val_5") bit<8> val_5 = h.h.f;
            @name("ingress.hasReturned") bool hasReturned_5 = false;
            @name("ingress.retval") bit<8> retval_5;
            val_5 = 8w1;
            hasReturned_5 = true;
            retval_5 = 8w2;
            h.h.f = val_5;
            tmp_15 = retval_5;
        }
        tmp_16 = tmp_14 % tmp_15;
        {
            @name("ingress.val_6") bit<8> val_6 = h.h.g;
            @name("ingress.hasReturned") bool hasReturned_6 = false;
            @name("ingress.retval") bit<8> retval_6;
            val_6 = 8w1;
            hasReturned_6 = true;
            retval_6 = 8w2;
            h.h.g = val_6;
            dummy_var_0 = retval_6;
        }
        tmp_17 = 8w0;
        {
            @name("ingress.val_7") bit<8> val_7 = h.h.h;
            @name("ingress.hasReturned") bool hasReturned_7 = false;
            @name("ingress.retval") bit<8> retval_7;
            val_7 = 8w1;
            hasReturned_7 = true;
            retval_7 = 8w2;
            h.h.h = val_7;
            tmp_18 = retval_7;
        }
        tmp_19 = tmp_17 |-| tmp_18;
        tmp_20 = 8w255;
        {
            @name("ingress.val_8") bit<8> val_8 = h.h.i;
            @name("ingress.hasReturned") bool hasReturned_8 = false;
            @name("ingress.retval") bit<8> retval_8;
            val_8 = 8w1;
            hasReturned_8 = true;
            retval_8 = 8w2;
            h.h.i = val_8;
            tmp_21 = retval_8;
        }
        tmp_22 = tmp_20 |+| tmp_21;
        tmp_23 = 8w255;
        {
            @name("ingress.val_9") bit<8> val_9 = h.h.j;
            @name("ingress.hasReturned") bool hasReturned_9 = false;
            @name("ingress.retval") bit<8> retval_9;
            val_9 = 8w1;
            hasReturned_9 = true;
            retval_9 = 8w2;
            h.h.j = val_9;
            tmp_24 = retval_9;
        }
        tmp_25 = tmp_23 + tmp_24;
        tmp_26 = 8w255;
        {
            @name("ingress.val_10") bit<8> val_10 = h.h.k;
            @name("ingress.hasReturned") bool hasReturned_10 = false;
            @name("ingress.retval") bit<8> retval_10;
            val_10 = 8w1;
            hasReturned_10 = true;
            retval_10 = 8w2;
            h.h.k = val_10;
            tmp_27 = retval_10;
        }
        tmp_28 = tmp_26 | tmp_27;
        {
            @name("ingress.val_11") bit<8> val_11 = h.h.l;
            @name("ingress.hasReturned") bool hasReturned_11 = false;
            @name("ingress.retval") bit<8> retval_11;
            val_11 = 8w1;
            hasReturned_11 = true;
            retval_11 = 8w2;
            h.h.l = val_11;
            tmp_29 = retval_11;
        }
        tmp_30 = 16w1;
        {
            @name("ingress.val_12") bit<8> val_12 = h.h.m;
            @name("ingress.hasReturned") bool hasReturned_12 = false;
            @name("ingress.retval") bit<8> retval_12;
            val_12 = 8w1;
            hasReturned_12 = true;
            retval_12 = 8w2;
            h.h.m = val_12;
            tmp_31 = retval_12;
        }
        tmp_32 = tmp_30 ++ tmp_31;
        {
            @name("ingress.val_13") bit<8> val_13 = h.b.c;
            @name("ingress.hasReturned") bool hasReturned_13 = false;
            @name("ingress.retval") bit<8> retval_13;
            val_13 = 8w1;
            hasReturned_13 = true;
            retval_13 = 8w2;
            h.b.c = val_13;
            tmp_34 = retval_13;
        }
        tmp_33 = tmp_34;
        {
            @name("ingress.val_14") bit<8> val_14 = h.b.c;
            @name("ingress.hasReturned") bool hasReturned_14 = false;
            @name("ingress.retval") bit<8> retval_14;
            val_14 = 8w1;
            hasReturned_14 = true;
            retval_14 = 8w2;
            h.b.c = val_14;
            tmp_35 = retval_14;
        }
        tmp_36 = tmp_33 != tmp_35;
        {
            @name("ingress.val_15") bit<8> val_15 = h.b.d;
            @name("ingress.hasReturned") bool hasReturned_15 = false;
            @name("ingress.retval") bit<8> retval_15;
            val_15 = 8w1;
            hasReturned_15 = true;
            retval_15 = 8w2;
            h.b.d = val_15;
            tmp_38 = retval_15;
        }
        tmp_37 = tmp_38;
        {
            @name("ingress.val_16") bit<8> val_16 = h.b.d;
            @name("ingress.hasReturned") bool hasReturned_16 = false;
            @name("ingress.retval") bit<8> retval_16;
            val_16 = 8w1;
            hasReturned_16 = true;
            retval_16 = 8w2;
            h.b.d = val_16;
            tmp_39 = retval_16;
        }
        tmp_40 = tmp_37 == tmp_39;
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

