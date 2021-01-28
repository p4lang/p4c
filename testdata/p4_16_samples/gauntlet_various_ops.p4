#include <core.p4>

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
    int<4>  b;
    bit<4>  c;
    int<4>  d;
    bit<4>  e;
    bit<4>  g;
    bit<8>  h;
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
    bool a;
}

struct Headers {
    OVERFLOW overflow;
    UNDERFLOW underflow;
    RSH rshift;
    LSH lshift;
    MOD mod;
    COMPARE comp;
    DIV div;
    BOOL b;
}

parser p(packet_in p, out Headers headers) {
    state start {
        transition accept;
    }
}

control ig(inout Headers h) {
    apply {
        //overflow
        h.overflow.a = 8w255 |+| 8w2;
        h.overflow.b = 8w3 |+| 8w0;
        //underflow
        h.underflow.a = 8w1 |-| 8w2;
        h.underflow.b = 8w3 |-| 8w0;
        const bit<8> uflow_tmp = 1;
        h.underflow.c = uflow_tmp |-| uflow_tmp;
        // unsigned mod
        h.mod.a = 4w1 % 4w8;
        h.mod.b = 4w15 % 4w2;
        // signed mod
        h.mod.c = 1 % 4w8;
        h.mod.d = 3 % 2;
        // // right shift
        bit<4> tmp = 4w0 - 4w1;
        h.rshift.a = tmp / 4w2;
        h.rshift.b = 4s7 >> 1 >> 1;
        h.rshift.c = 4w15 >> 1 >> 1;
        h.rshift.d = -4s7 >> 1 >> 1;
        h.rshift.e = tmp >> 1 >> 1;
        h.rshift.g = 4w1 >> 8w16;
        h.rshift.h = (bit<8>)~(4w1 >> 8w1);
        //left shift
        h.lshift.a = (bit<8>)(4w4 << 8w2);
        h.lshift.b = (bit<8>)(4w4 << 8w16);
        h.lshift.c = 1 << 1;
        h.lshift.d = (bit<8>)1 << 256;
        h.lshift.e = 8w1 << 8w0;

        // comparing various constants
        if (4w15  > 2) { h.comp.a = 1; }
        if (4w3  > 2) { h.comp.b = 1; }
        if (-1  > 4w8) { h.comp.c = 1; }
        if (4w8 > -1) { h.comp.d = 1; }
        // FIXME: This expression should also work
        // if (-1  > 4s8) { h.comp.e = 1; }
        if (-1  > 4s7) { h.comp.e = 1; }
        // Division
        h.div.a = (bit<8>)(4 / 1w1);
        h.div.b = (3 - 8w2 / 2);
        h.div.c = (8w2 / 2 - 3 );
        // nested int operations
        bit<48> tmp2 = (1 | 2) |+| 48w0;
        const int int_def = 1;

        // bool evaluation
        h.b.a = 1 == 1;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ig()) main;

