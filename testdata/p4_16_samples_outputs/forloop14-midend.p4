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
    @hidden action forloop14l28() {
        x_0 = 32w7;
    }
    @hidden action forloop14l26() {
        x_0 = hdrs.head.init;
    }
    @hidden action forloop14l30() {
        hdrs.head.output = x_0;
        hdrs.head.field = hdrs.head.init;
    }
    @hidden table tbl_forloop14l26 {
        actions = {
            forloop14l26();
        }
        const default_action = forloop14l26();
    }
    @hidden table tbl_forloop14l28 {
        actions = {
            forloop14l28();
        }
        const default_action = forloop14l28();
    }
    @hidden table tbl_forloop14l30 {
        actions = {
            forloop14l30();
        }
        const default_action = forloop14l30();
    }
    apply {
        tbl_forloop14l26.apply();
        for (@name("c.i") bit<32> i_0 in hdrs.head.limit .. 32w3) {
            tbl_forloop14l28.apply();
        }
        tbl_forloop14l30.apply();
    }
}

top<headers_t>(c()) main;
