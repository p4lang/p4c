#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> f1;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  cnt;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    @name("c.i") bit<32> i_0;
    @name("c.j") bit<32> j_0;
    apply {
        i_0 = 32w0;
        @no_unroll for (j_0 = 32w0; j_0 < 32w10; j_0 = j_0 + 32w1) {
            i_0 = i_0 + 32w1;
        }
        @unroll for (; i_0 < 32w20; ) {
            hdrs.head.h1 = hdrs.head.h1 + 16w1;
        }
    }
}

top<headers_t>(c()) main;
