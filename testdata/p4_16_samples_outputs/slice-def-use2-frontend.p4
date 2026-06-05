#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<256> x;
    bit<256> y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    @name("c.a0") action a0() {
        hdrs.t1.x[255:128] = hdrs.t1.y[127:0];
        hdrs.t1.x[127:0] = hdrs.t1.y[255:128];
    }
    apply {
        a0();
    }
}

top<headers_t>(c()) main;
