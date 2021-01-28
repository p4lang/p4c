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
    bool a;
}

struct Headers {
    OVERFLOW  overflow;
    UNDERFLOW underflow;
    RSH       rshift;
    LSH       lshift;
    MOD       mod;
    COMPARE   comp;
    DIV       div;
    BOOL      b;
}

parser p(packet_in p, out Headers headers) {
    state start {
        transition accept;
    }
}

control ig(inout Headers h) {
    apply {
        h.overflow.a = 8w255;
        h.overflow.b = 8w3;
        h.underflow.a = 8w0;
        h.underflow.b = 8w3;
        const bit<8> uflow_tmp = 8w1;
        h.underflow.c = 8w0;
        h.mod.a = 4w1;
        h.mod.b = 4w1;
        h.mod.c = 4w1;
        h.mod.d = 4w1;
        bit<4> tmp = 4w15;
        h.rshift.a = tmp >> 1;
        h.rshift.b = 4s1;
        h.rshift.c = 4w3;
        h.rshift.d = -4s2;
        h.rshift.e = tmp >> 2;
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
        bit<48> tmp2 = 48w3;
        const int int_def = 1;
        h.b.a = true;
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
package top(Parser p, Ingress ig);
top(p(), ig()) main;

