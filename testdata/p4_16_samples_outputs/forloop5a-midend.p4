#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> x;
    bit<32> y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.a0") action a0() {
        hdrs.t1.x = ((((bit<32>)hdrs.t1.x[7:0] + (bit<32>)hdrs.t1.y[7:0] << 8) + (bit<32>)hdrs.t1.x[15:8] + (bit<32>)hdrs.t1.y[15:8] << 8) + (bit<32>)hdrs.t1.x[23:16] + (bit<32>)hdrs.t1.y[23:16] << 8) + (bit<32>)hdrs.t1.x[31:24] + (bit<32>)hdrs.t1.y[31:24];
    }
    @hidden table tbl_a0 {
        actions = {
            a0();
        }
        const default_action = a0();
    }
    apply {
        tbl_a0.apply();
    }
}

top<headers_t>(c()) main;
