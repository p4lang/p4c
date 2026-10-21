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
    @hidden action forloop13l26() {
        x_0 = 32w7;
        hdrs.head.field = 32w9;
    }
    @hidden action forloop13l24() {
        x_0 = hdrs.head.init;
    }
    @hidden action forloop13l29() {
        hdrs.head.output = x_0;
        hdrs.head.field = hdrs.head.field + 32w1;
    }
    @hidden table tbl_forloop13l24 {
        actions = {
            forloop13l24();
        }
        const default_action = forloop13l24();
    }
    @hidden table tbl_forloop13l26 {
        actions = {
            forloop13l26();
        }
        const default_action = forloop13l26();
    }
    @hidden table tbl_forloop13l29 {
        actions = {
            forloop13l29();
        }
        const default_action = forloop13l29();
    }
    apply {
        tbl_forloop13l24.apply();
        for (i_0 = 32w0; i_0 < hdrs.head.limit; i_0 = i_0 + 32w1) {
            tbl_forloop13l26.apply();
        }
        tbl_forloop13l29.apply();
    }
}

top<headers_t>(c()) main;
