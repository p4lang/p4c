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
    @hidden action issue2890l17() {
        x_0.setInvalid();
        x_0 = h.h;
        x_0.a = 16w2;
        h.e.e = 16w2;
    }
    @hidden table tbl_issue2890l17 {
        actions = {
            issue2890l17();
        }
        const default_action = issue2890l17();
    }
    apply {
        tbl_issue2890l17.apply();
    }
}

control I(inout Headers h);
package top(I i);
top(ingress()) main;
