#include <core.p4>

struct S {
    bit<8> f;
}

union Safe {
    bit<32> b;
    bit<16> c;
    S       f;
}

union Either<T, U> {
    T t;
    U u;
}

union Either_0 {
    bit<32> t;
    bit<16> u;
}

control c(out bool o) {
    @name("c.e") Either_0 e_0;
    @name("c.s") Safe s_0;
    @name("c.s1") Safe s1_0;
    apply {
        s_0.b = 32w4;
        s1_0.b = 32w2;
        switch (s_0) {
            Safe.b: {
            }
            Safe.c: {
                switch (s1_0) {
                    Safe.b: {
                    }
                    default: {
                    }
                }
            }
            Safe.f: {
            }
            default: {
            }
        }
        switch (e_0) {
            Either_0.t: {
                o = e_0.t == 32w0;
            }
            Either_0.u: {
                o = e_0.u == 16w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

