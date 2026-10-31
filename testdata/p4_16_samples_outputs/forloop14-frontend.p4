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
    @name("c.y") bit<32> y_0;
    @name("c.j") bit<32> j_0;
    apply {
        x_0 = hdrs.head.init;
        for (@name("c.i") bit<32> i_0 in hdrs.head.limit .. 32w3) {
            x_0 = 32w7;
        }
        hdrs.head.output = x_0;
        y_0 = hdrs.head.init;
        for (@name("c.j") bit<32> j_0 in 32w5 .. 32w3) {
            y_0 = 32w8;
        }
        hdrs.head.field = y_0;
    }
}

top<headers_t>(c()) main;
