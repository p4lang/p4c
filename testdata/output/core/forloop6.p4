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
    action a0(bit<8> m) {
        bit<32> result = 0;
        bit<8> mask = m;
        for (bit<8> i = 0; i < 32; i = i + 4, mask = mask >> 1) {
            if (mask == 0) {
                break;
            }
            if (mask[0:0] == 0) {
                continue;
            }
            result = result + (hdrs.t1.y >> i & 0xf);
        }
        hdrs.t1.y = result;
    }
    table test {
        key = {
            hdrs.t1.x: exact;
        }
        actions = {
            a0;
        }
    }
    apply {
        test.apply();
    }
}

top(c()) main;
