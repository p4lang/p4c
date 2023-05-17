#include <core.p4>

header E {
    bit<16> e;
}

header H {
    bit<16> a;
}

struct Headers {
    E e;
    H h;
}

control ingress(inout Headers h) {
    @name("ingress.x") H x_0;
    apply {
        x_0.setInvalid();
        x_0 = h.h;
        x_0.a = 16w2;
        h.e.e = x_0.a;
    }
}

control I(inout Headers h);
package top(I i);
top(ingress()) main;
