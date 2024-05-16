#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

extern bool fn(in bit<16> a, in bit<16> b);

header t1 {
    bit<32>     x;
    bit<32>     y;
}

struct headers_t {
    t1          t1;
}

control c(inout headers_t hdrs) {
    action a0(bit<8> m) {
        for (bit<16> a in 1..2) {
            fn(a, 0);
            for (bit<16> b in 1..2) {
                if (fn(a, b)) continue;
                return;
            }
            fn(a, 3);
            fn(a, 4);
        }
        fn(3, 3);
    }

    table test {
        key = { hdrs.t1.x: exact; }
        actions = { a0; }
    }


    apply {
        test.apply();
    }
}

top(c()) main;
