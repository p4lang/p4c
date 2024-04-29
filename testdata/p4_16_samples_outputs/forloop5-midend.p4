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
    @name("c.result") bit<32> result_0;
    @name("c.a0") action a0() {
        result_0 = 32w0;
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> (8w0 << 3)) + (hdrs.t1.y >> (8w0 << 3)) & 32w0xff);
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> (8w1 << 3)) + (hdrs.t1.y >> (8w1 << 3)) & 32w0xff);
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> (8w2 << 3)) + (hdrs.t1.y >> (8w2 << 3)) & 32w0xff);
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> (8w3 << 3)) + (hdrs.t1.y >> (8w3 << 3)) & 32w0xff);
        hdrs.t1.x = result_0;
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
