#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
extern bool fn(in bit<16> a, in bit<16> b);
header t1 {
    bit<32> x;
    bit<32> y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    action a0(bit<8> m) {
        for (bit<16> a in 16w1 .. 16w2) {
            fn(a, 16w0);
            for (bit<16> b in 16w1 .. 16w2) {
                if (fn(a, b)) {
                    continue;
                }
                return;
            }
            fn(a, 16w3);
            fn(a, 16w4);
        }
        fn(16w3, 16w3);
    }
    table test {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        test.apply();
    }
}

top<headers_t>(c()) main;
