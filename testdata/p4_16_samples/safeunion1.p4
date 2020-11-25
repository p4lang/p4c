#include <core.p4>

struct S {
    bit<8> f;
}

union Safe {
    bit<32> b;
    bit<16> c;
    S f;
}

union Either<T, U> {
    T t;
    U u;
}

control c(out bool o) {
    apply {
        Either<bit<32>, bit<16>> e;

        Safe s;
        s.b = 4;
        Safe s1;
        s1.b = 2;
        switch (s) {
            Safe.b: { o = (s.b == 0); }
            Safe.c: {
                switch (s1) {
                    Safe.b: { o = (s1.b == (bit<32>)s.c); }
                    default: { o = false; }
                }
            }
            Safe.f: { o = (s.f.f == 1); s.f.f = 2; }
            default: { o = true; s1.c = 2; }
        }

        switch (e) {
            Either.t: { o = e.t == 0; }
            Either.u: { o = e.u == 0; }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);

top(c()) main;
