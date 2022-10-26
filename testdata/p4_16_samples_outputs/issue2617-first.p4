#include <core.p4>

enum bit<32> X {
    A = 32w0,
    B = 32w1
}

enum E {
    A,
    B,
    C
}

parser p(out bit<32> v) {
    state start {
        transition one;
    }
    state zero {
        v = 32w0;
        transition accept;
    }
    state one {
        v = 32w1;
        transition accept;
    }
    state two {
        v = 32w2;
        transition reject;
    }
}

parser p1(out bit<32> v) {
    state start {
        transition two;
    }
    state zero {
        v = 32w0;
        transition accept;
    }
    state one {
        v = 32w1;
        transition accept;
    }
    state two {
        v = 32w2;
        transition reject;
    }
}

control c(out bit<32> v) {
    apply {
        switch (32w1) {
            32w0: {
                v = 32w0;
            }
            32w1: {
                v = 32w1;
            }
        }
        switch (E.C) {
            E.A: {
                v = v + 32w1;
            }
            default: {
                v = v + 32w2;
            }
        }
        switch (E.C) {
            E.A: {
                v = v + 32w1;
            }
            default: {
                v = v + 32w3;
            }
        }
        switch (X.B) {
            X.A: {
                v = v + 32w10;
            }
            X.B: {
                v = v + 32w20;
            }
        }
    }
}

parser _p(out bit<32> v);
control _c(out bit<32> v);
package top(_p _p0, _p p1, _c _c0);
top(p(), p1(), c()) main;
