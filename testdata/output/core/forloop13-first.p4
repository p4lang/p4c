#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> init;
    bit<32> limit;
    bit<32> output;
    bit<32> field;
}

struct headers_t {
    t1 head;
}

control c(inout headers_t hdrs) {
    apply {
        bit<32> x = hdrs.head.init;
        for (bit<32> i = 32w0; i < hdrs.head.limit; i = i + 32w1) {
            x = 32w7;
            hdrs.head.field = 32w9;
        }
        hdrs.head.output = x;
        hdrs.head.field = hdrs.head.field + 32w1;
    }
}

top<headers_t>(c()) main;
