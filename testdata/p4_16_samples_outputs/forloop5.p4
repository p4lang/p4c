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
        bit<32> result = 0;
        for (bit<8> i = 0; i < 4; i = i + 1) {
            result = result << 8;
            result = result + ((hdrs.t1.x >> 8 * i) + (hdrs.t1.y >> 8 * i) & 0xff);
        }
        hdrs.t1.x = result;
    }
    apply {
        a0();
    }
}

top(c()) main;
