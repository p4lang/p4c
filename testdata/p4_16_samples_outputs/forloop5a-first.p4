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
        for (bit<8> i = 8w0; i < 8w32; i = i + 8w8) {
            result = result << 8;
            result = result + (bit<32>)hdrs.t1.x[i+:8] + (bit<32>)hdrs.t1.y[i+:8];
        }
        hdrs.t1.x = result;
    }
    apply {
        a0();
    }
}

top<headers_t>(c()) main;
