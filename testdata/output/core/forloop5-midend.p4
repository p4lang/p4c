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
        hdrs.t1.x = ((((hdrs.t1.x + hdrs.t1.y & 32w0xff) << 8) + ((hdrs.t1.x >> 8w8) + (hdrs.t1.y >> 8w8) & 32w0xff) << 8) + ((hdrs.t1.x >> 8w16) + (hdrs.t1.y >> 8w16) & 32w0xff) << 8) + ((hdrs.t1.x >> 8w24) + (hdrs.t1.y >> 8w24) & 32w0xff);
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
