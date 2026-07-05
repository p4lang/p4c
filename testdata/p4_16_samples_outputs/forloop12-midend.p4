#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<32> x;
    bit<32> y;
}

struct headers_t {
    t1 t1;
    t1 t2;
}

control c(inout headers_t hdrs) {
    @name("c.result") bit<32> result_0;
    @name("c.a0") action a0() {
        result_0 = 32w0;
        result_0 = 32w0;
        result_0 = hdrs.t1.x + hdrs.t1.y & 32w0xff;
        if (hdrs.t2.isValid()) {
            result_0 = (hdrs.t1.x + hdrs.t1.y & 32w0xff) + (hdrs.t2.x + hdrs.t2.y & 32w0xff);
        }
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> 8w8) + (hdrs.t1.y >> 8w8) & 32w0xff);
        if (hdrs.t2.isValid()) {
            result_0 = result_0 + ((hdrs.t2.x >> 8w8) + (hdrs.t2.y >> 8w8) & 32w0xff);
        }
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> 8w16) + (hdrs.t1.y >> 8w16) & 32w0xff);
        if (hdrs.t2.isValid()) {
            result_0 = result_0 + ((hdrs.t2.x >> 8w16) + (hdrs.t2.y >> 8w16) & 32w0xff);
        }
        result_0 = result_0 << 8;
        result_0 = result_0 + ((hdrs.t1.x >> 8w24) + (hdrs.t1.y >> 8w24) & 32w0xff);
        if (hdrs.t2.isValid()) {
            result_0 = result_0 + ((hdrs.t2.x >> 8w24) + (hdrs.t2.y >> 8w24) & 32w0xff);
        }
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
