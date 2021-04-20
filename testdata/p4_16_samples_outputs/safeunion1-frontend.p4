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
    @name("c.e") Either_0 e_0;
    @name("c.s") Safe s_0;
    @name("c.s1") Safe s1_0;
    apply {
        s_0.b = 32w4;
        s1_0.b = 32w2;
        switch (s_0) {
            s_0.b as b: {
                o = b == 32w0;
            }
            s_0.c as sc: {
                switch (s1_0) {
                    s1_0.b as s1b: {
                        o = s1b == (bit<32>)sc;
                    }
                    default: {
                        o = false;
                    }
                }
            }
            s_0.f as f: {
                o = f.f == 8w1;
            }
            default: {
                o = true;
            }
        }
        switch (e_0) {
            e_0.t as et: {
                o = o && et == 32w0;
            }
            e_0.u as eu: {
                o = o && eu == 16w0;
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

