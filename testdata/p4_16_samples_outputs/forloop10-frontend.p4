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
    @name("c.sum") bit<32> sum_0;
    @name("c.i") bit<32> i_0;
    apply {
        sum_0 = 32w1;
        @unroll for (i_0 = 32w5; i_0 != 32w0; ) {
            sum_0 = sum_0 + i_0;
            if (sum_0 == 32w10) {
                i_0 = i_0 - 32w1;
            }
            hdrs.head.cnt = hdrs.head.cnt + 8w1;
        }
    }
}

top<headers_t>(c()) main;
