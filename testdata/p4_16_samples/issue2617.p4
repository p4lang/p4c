#include <core.p4>

enum bit<32> X {
    A = 0,
    B = 1
}

enum E {
    A, B, C
}

parser p(out bit<32> v) {
    state start {
        transition select (32w1) {
            0: zero;
            1: one;
            2: two;
        }
    }

    state zero {
        v = 0;
        transition accept;
    }

    state one {
        v = 1;
        transition accept;
    }

    state two {
        v = 2;
        transition reject;
    }
}

parser p1(out bit<32> v) {
    state start {
        transition select (X.A, E.A, 32w0) {
            (X.A, E.B, _): zero;
            (X.A, E.A, 1): one;
            (X.A, _,   _): two;
        }
    }

    state zero {
        v = 0;
        transition accept;
    }

    state one {
        v = 1;
        transition accept;
    }

    state two {
        v = 2;
        transition reject;
    }
}

control c(out bit<32> v) {
    apply {
        switch (32w1) {
            0: { v = 0; }
            1: { v = 1; }
            2:
        }

        switch (E.C) {
            E.A: { v = v + 1; }
            default: { v = v + 2; }
        }

        switch (E.C) {
            E.A: { v = v + 1; }
            default: { v = v + 3; }
        }

        switch (X.B) {
            X.A: { v = v + 10; }
            X.B: { v = v + 20; }
        }
    }
}

parser _p(out bit<32> v);
control _c(out bit<32> v);
package top(_p _p0, _p p1, _c _c0);

top(p(), p1(), c()) main;
