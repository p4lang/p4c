#include <core.p4>

control E(out bit<1> b);
control D(out bit<1> b) {
    apply {
        b = 1;
    }
}

control F(out bit<1> b) {
    apply {
        b = 0;
    }
}

control C(out bit<1> b)(E d) {
    apply {
        d.apply(b);
    }
}

control Ingress(out bit<1> b) {
    D() d;
    F() f;
    C(d) c0;
    C(f) c1;
    apply {
        c0.apply(b);
        c1.apply(b);
    }
}

package top(E _e);
top(Ingress()) main;

