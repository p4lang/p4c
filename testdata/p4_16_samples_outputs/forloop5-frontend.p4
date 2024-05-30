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
    @name("c.i") bit<8> i_0;
    @name("c.a0") action a0() {
        result_0 = 32w0;
        for (i_0 = 8w0; i_0 < 8w4; i_0 = i_0 + 8w1) {
            result_0 = result_0 << 8;
            result_0 = result_0 + ((hdrs.t1.x >> (i_0 << 3)) + (hdrs.t1.y >> (i_0 << 3)) & 32w0xff);
        }
        hdrs.t1.x = result_0;
    }
    apply {
        a0();
    }
}

top<headers_t>(c()) main;
