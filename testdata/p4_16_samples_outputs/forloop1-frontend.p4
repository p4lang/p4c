#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

header t2 {
    bit<32> x;
}

struct headers_t {
    t1    head;
    t2[8] stack;
}

control c(inout headers_t hdrs) {
    @name("c.v") t2 v_0;
    @name("c.v") bit<8> v_1;
    @name("c.x") bit<16> x_0;
    apply {
        for (@name("c.v") t2 v_0 in hdrs.stack) {
            hdrs.head.f1 = hdrs.head.f1 + v_0.x;
        }
        for (v_1 = 8w0, x_0 = 16w1; v_1 < hdrs.head.cnt; v_1 = v_1 + 8w1, x_0 = x_0 << 1) {
            hdrs.head.h1 = hdrs.head.h1 + x_0;
        }
    }
}

top<headers_t>(c()) main;
