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
    @name("c.mask") bit<8> mask_0;
    @name("c.i") bit<8> i_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.a0") action a0(@name("m") bit<8> m_1) {
        result_0 = 32w0;
        mask_0 = m_1;
        for (i_0 = 8w0; i_0 < 8w32; i_0 = i_0 + 8w4, mask_0 = mask_0 >> 1) {
            if (mask_0 == 8w0) {
                break;
            } else if (mask_0[0:0] == 1w0) {
                continue;
            } else {
                result_0 = result_0 + (hdrs.t1.y >> i_0 & 32w0xf);
            }
        }
        hdrs.t1.y = result_0;
    }
    @name("c.test") table test_0 {
        key = {
            hdrs.t1.x: exact @name("hdrs.t1.x");
        }
        actions = {
            a0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        test_0.apply();
    }
}

top<headers_t>(c()) main;
