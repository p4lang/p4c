#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> x;
}

struct headers_t {
    t1 t1;
}

bit<8> foo(in bit<8> aa, in bit<8> bb) {
    return aa - bb;
}
control c(inout headers_t hdrs) {
    action a0(bit<8> x, bit<8> y) {
        for (bit<8> i in 8w1 .. x + y) {
            foo(x, y);
        }
        for (bit<8> i in 8w1 .. x + y) {
            foo(x, y + i);
        }
        for (bit<8> i in 8w1 .. 8w42) {
            foo(x, y + i);
        }
    }
    action a1(bit<8> x, bit<8> y) {
        for (bit<8> i in 8w1 .. x) {
            for (bit<8> j in i .. y) {
                foo(x, y + i);
            }
        }
    }
    action a2(bit<8> x, bit<8> y) {
        bit<8> i = 8w10;
        for (bit<8> i in 8w1 .. x) {
            foo(x, y + i);
        }
        for (bit<8> k in i .. x + y) {
            foo(i, (i << 1) + x);
        }
    }
    table test {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            a1();
            a2();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        test.apply();
    }
}

top<headers_t>(c()) main;
