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
    @name("c.x") bit<32> x_0;
    @name("c.i") bit<32> i_0;
    apply {
        x_0 = hdrs.head.init;
        for (i_0 = 32w0; i_0 < hdrs.head.limit; i_0 = i_0 + 32w1) {
            x_0 = 32w7;
            hdrs.head.field = 32w9;
        }
        hdrs.head.output = x_0;
        hdrs.head.field = hdrs.head.field + 32w1;
    }
}

top<headers_t>(c()) main;
