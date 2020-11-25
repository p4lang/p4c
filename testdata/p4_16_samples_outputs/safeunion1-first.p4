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
    apply {
        Either_0 e;
        Safe s;
        s.b = 32w4;
        Safe s1;
        s1.b = 32w2;
        switch (s) {
            Safe.b: {
                o = s.b == 32w0;
            }
            Safe.c: {
                switch (s1) {
                    Safe.b: {
                        o = s1.b == (bit<32>)s.c;
                    }
                    default: {
                        o = false;
                    }
                }
            }
            Safe.f: {
                o = s.f.f == 8w1;
                s.f.f = 8w2;
            }
            default: {
                o = true;
                s1.c = 16w2;
            }
        }
        switch (e) {
            Either_0.t: {
                o = e.t == 32w0;
            }
            Either_0.u: {
                o = e.u == 16w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

