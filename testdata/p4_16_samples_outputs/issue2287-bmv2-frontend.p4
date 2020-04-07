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
    bit<8> dummy_var_0;
    bit<8> tmp;
    bit<8> tmp_0;
    bit<8> tmp_1;
    bit<8> tmp_2;
    bit<8> tmp_3;
    bit<8> tmp_4;
    bit<8> tmp_5;
    bit<8> tmp_6;
    bit<8> tmp_7;
    bit<8> tmp_8;
    bit<8> tmp_9;
    bit<8> tmp_10;
    bit<8> tmp_11;
    bit<8> tmp_12;
    bit<8> tmp_13;
    bit<8> tmp_14;
    bit<8> tmp_15;
    bit<8> tmp_16;
    bit<8> tmp_17;
    bit<8> tmp_18;
    bit<8> tmp_19;
    bit<8> tmp_20;
    bit<24> tmp_21;
    bit<8> tmp_22;
    bit<8> tmp_23;
    bool tmp_24;
    bit<8> tmp_25;
    bit<8> tmp_26;
    bool tmp_27;
    apply {
        {
            bit<8> val_0 = h.h.a;
            bool hasReturned = false;
            bit<8> retval;
            val_0 = 8w1;
            hasReturned = true;
            retval = 8w2;
            h.h.a = val_0;
            tmp = retval;
        }
        tmp_0 = 8w0 & tmp;
        {
            bit<8> val_1 = h.h.b;
            bool hasReturned_1 = false;
            bit<8> retval_1;
            val_1 = 8w1;
            hasReturned_1 = true;
            retval_1 = 8w2;
            h.h.b = val_1;
            tmp_1 = retval_1;
        }
        tmp_2 = 8w0 * tmp_1;
        {
            bit<8> val_2 = h.h.c;
            bool hasReturned_2 = false;
            bit<8> retval_2;
            val_2 = 8w1;
            hasReturned_2 = true;
            retval_2 = 8w2;
            h.h.c = val_2;
            tmp_3 = retval_2;
        }
        tmp_4 = 8w0 / tmp_3;
        {
            bit<8> val_3 = h.h.d;
            bool hasReturned_3 = false;
            bit<8> retval_3;
            val_3 = 8w1;
            hasReturned_3 = true;
            retval_3 = 8w2;
            h.h.d = val_3;
            tmp_5 = retval_3;
        }
        tmp_6 = 8w0 >> tmp_5;
        {
            bit<8> val_4 = h.h.e;
            bool hasReturned_4 = false;
            bit<8> retval_4;
            val_4 = 8w1;
            hasReturned_4 = true;
            retval_4 = 8w2;
            h.h.e = val_4;
            tmp_7 = retval_4;
        }
        tmp_8 = 8w0 << tmp_7;
        {
            bit<8> val_5 = h.h.f;
            bool hasReturned_5 = false;
            bit<8> retval_5;
            val_5 = 8w1;
            hasReturned_5 = true;
            retval_5 = 8w2;
            h.h.f = val_5;
            tmp_9 = retval_5;
        }
        tmp_10 = 8w0 % tmp_9;
        {
            bit<8> val_6 = h.h.g;
            bool hasReturned_6 = false;
            bit<8> retval_6;
            val_6 = 8w1;
            hasReturned_6 = true;
            retval_6 = 8w2;
            h.h.g = val_6;
            dummy_var_0 = retval_6;
        }
        {
            bit<8> val_7 = h.h.h;
            bool hasReturned_7 = false;
            bit<8> retval_7;
            val_7 = 8w1;
            hasReturned_7 = true;
            retval_7 = 8w2;
            h.h.h = val_7;
            tmp_11 = retval_7;
        }
        tmp_12 = 8w0 |-| tmp_11;
        {
            bit<8> val_8 = h.h.i;
            bool hasReturned_8 = false;
            bit<8> retval_8;
            val_8 = 8w1;
            hasReturned_8 = true;
            retval_8 = 8w2;
            h.h.i = val_8;
            tmp_13 = retval_8;
        }
        tmp_14 = 8w255 |+| tmp_13;
        {
            bit<8> val_9 = h.h.j;
            bool hasReturned_9 = false;
            bit<8> retval_9;
            val_9 = 8w1;
            hasReturned_9 = true;
            retval_9 = 8w2;
            h.h.j = val_9;
            tmp_15 = retval_9;
        }
        tmp_16 = 8w255 + tmp_15;
        {
            bit<8> val_10 = h.h.k;
            bool hasReturned_10 = false;
            bit<8> retval_10;
            val_10 = 8w1;
            hasReturned_10 = true;
            retval_10 = 8w2;
            h.h.k = val_10;
            tmp_17 = retval_10;
        }
        tmp_18 = 8w255 | tmp_17;
        {
            bit<8> val_11 = h.h.l;
            bool hasReturned_11 = false;
            bit<8> retval_11;
            val_11 = 8w1;
            hasReturned_11 = true;
            retval_11 = 8w2;
            h.h.l = val_11;
            tmp_19 = retval_11;
        }
        {
            bit<8> val_12 = h.h.m;
            bool hasReturned_12 = false;
            bit<8> retval_12;
            val_12 = 8w1;
            hasReturned_12 = true;
            retval_12 = 8w2;
            h.h.m = val_12;
            tmp_20 = retval_12;
        }
        tmp_21 = 16w1 ++ tmp_20;
        {
            bit<8> val_13 = h.b.c;
            bool hasReturned_13 = false;
            bit<8> retval_13;
            val_13 = 8w1;
            hasReturned_13 = true;
            retval_13 = 8w2;
            h.b.c = val_13;
            tmp_22 = retval_13;
        }
        {
            bit<8> val_14 = h.b.c;
            bool hasReturned_14 = false;
            bit<8> retval_14;
            val_14 = 8w1;
            hasReturned_14 = true;
            retval_14 = 8w2;
            h.b.c = val_14;
            tmp_23 = retval_14;
        }
        tmp_24 = tmp_22 != tmp_23;
        {
            bit<8> val_15 = h.b.d;
            bool hasReturned_15 = false;
            bit<8> retval_15;
            val_15 = 8w1;
            hasReturned_15 = true;
            retval_15 = 8w2;
            h.b.d = val_15;
            tmp_25 = retval_15;
        }
        {
            bit<8> val_16 = h.b.d;
            bool hasReturned_16 = false;
            bit<8> retval_16;
            val_16 = 8w1;
            hasReturned_16 = true;
            retval_16 = 8w2;
            h.b.d = val_16;
            tmp_26 = retval_16;
        }
        tmp_27 = tmp_25 == tmp_26;
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

