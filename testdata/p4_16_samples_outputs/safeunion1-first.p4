#include <core.p4>

struct S {
    bit<8> f;
}

union Safe {
    bit<32> b;
    bit<16> c;
    S       f;
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
            s.b as b: {
                o = b == 32w0;
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
                o = f.f == 8w1;
                f.f = 8w2;
                s.f = f;
            }
            default: {
                o = true;
                s1.c = 16w2;
            }
        }
        switch (e) {
            e.t as et: {
                o = o && et == 32w0;
            }
            e.u as eu: {
                o = o && eu == 16w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

