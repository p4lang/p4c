#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
header t1 {
    bit<256> data;
    bit<32>  x;
    bit<32>  y;
}

struct headers_t {
    t1 t1;
}

control c(inout headers_t hdrs) {
    action a0() {
        hdrs.t1.y = hdrs.t1.data[hdrs.t1.x & 32w0xff+:32];
    }
    apply {
        a0();
    }
}

top<headers_t>(c()) main;
