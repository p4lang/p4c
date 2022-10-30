#include <core.p4>

header E {
    bit<16> e;
}

header H {
    bit<16>  a;
}

struct Headers {
    E e;
    H h;
}

control ingress(inout Headers h) {
    H x;

    apply {
        x = h.h;
        x.a = 16w2;
        h.e.e = x.a;
    }
}

control I(inout Headers h);
package top(I i);

top(ingress()) main;
