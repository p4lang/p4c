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
    action a0() {
        bit<32> result = 32w0;
        for (bit<8> i = 8w0; i < 8w4; i = i + 8w1) {
            result = result << 8;
            result = result + ((hdrs.t1.x >> (i << 3)) + (hdrs.t1.y >> (i << 3)) & 32w0xff);
        }
        hdrs.t1.x = result;
    }
    apply {
        a0();
    }
}

top<headers_t>(c()) main;
