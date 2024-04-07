#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
extern T foo<T>(in T x);
header t1 {
    bit<32> x;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    action a0(bit<8> x, bit<8> y) {
        bit<8> idx = 8w255;
        for (bit<8> i1 in 8w0 .. x) {
            for (bit<8> j in i1 .. y) {
                idx = foo<bit<8>>(j);
                if (idx == 8w255) {
                    break;
                }
            }
        }
    }
    action a1(bit<8> x, bit<8> y) {
        bit<8> idx = 8w255;
        for (bit<8> i in 8w0 .. x) {
            for (bit<8> j in 8w0 .. y) {
                idx = foo<bit<8>>(j);
                if (idx == 8w255) {
                    continue;
                }
                idx = foo<bit<8>>(i);
            }
        }
    }
    table test {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            a1();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        test.apply();
    }
}

top<headers_t>(c()) main;
