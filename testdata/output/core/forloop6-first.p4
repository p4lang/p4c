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
        bit<32> result = 32w0;
        bit<8> mask = m;
        for (bit<8> i = 8w0; i < 8w32; i = i + 8w4, mask = mask >> 1) {
            if (mask == 8w0) {
                break;
            }
            if (mask[0:0] == 1w0) {
                continue;
            }
            result = result + (hdrs.t1.y >> i & 32w0xf);
        }
        hdrs.t1.y = result;
    }
    table test {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        test.apply();
    }
}

top<headers_t>(c()) main;
