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
    apply {
        bit<32> i = 32w0;
        @no_unroll for (bit<32> j = 32w0; j < 32w10; j += 32w1) {
            i += 32w1;
        }
        @unroll for (; i < 32w20; ) {
            hdrs.head.h1 += 16w1;
        }
    }
}

top<headers_t>(c()) main;
