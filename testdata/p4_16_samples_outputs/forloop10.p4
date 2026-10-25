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
        bit<32> sum = 1;
        @unroll for (bit<32> i = 5; i != 0; ) {
            sum += i;
            if (sum == 10) {
                i -= 1;
            }
            hdrs.head.cnt += 1;
        }
    }
}

top(c()) main;
