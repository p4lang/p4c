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

control c(out bool o) {
    apply {
        Either<bit<32>, bit<16>> e;
        Safe s;
        s.b = 4;
        Safe s1;
        s1.b = 2;
        switch (s) {
            s.b as b: {
                o = b == 0;
            }
            s.c as sc: {
                switch (s1) {
                    s1.b as s1b: {
                        o = s1b == (bit<32>)sc;
                    }
                    default: {
                        o = false;
                    }
                }
            }
            s.f as f: {
                o = f.f == 1;
                f.f = 2;
                s.f = f;
            }
            default: {
                o = true;
                s1.c = 2;
            }
        }
        switch (e) {
            e.t as et: {
                o = o && et == 0;
            }
            e.u as eu: {
                o = o && eu == 0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

