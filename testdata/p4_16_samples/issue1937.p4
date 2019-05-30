#include <core.p4>

header h1_t { bit<8> f1; bit<8> f2; }

parser foo (out bit<8> x, in bit<8> y = 5) {
    state start {
        x = (y >> 2);
        transition accept;
    }
}

parser parserImpl(out h1_t hdr) {
    state start {
        foo.apply(hdr.f1, hdr.f1);
        foo.apply(hdr.f2);
        transition accept;
    }
}

parser p<T>(out T h);
package top<T>(p<T> p);

top(parserImpl()) main;
