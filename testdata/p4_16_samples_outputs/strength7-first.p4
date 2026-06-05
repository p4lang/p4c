#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    int<8>  b3;
}

struct headers_t {
    t1 head;
}

extern bit<32> fn(in bit<32> x);
control c(inout headers_t hdrs) {
    action act(bit<32> x) {
        fn(x);
    }
    apply {
        act(32w1);
        if (fn(32w2) == fn(32w2)) {
            act(32w3);
        }
        act(32w5);
        act(32w7);
        act(32w9);
        act(32w11);
        act(32w13);
        act(32w15);
        act(32w17);
        act(32w19);
        act(32w21);
        act(32w23);
    }
}

top<headers_t>(c()) main;
