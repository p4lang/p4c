#include <core.p4>

header H {
    bit<32> e;
    bit<32> t;
}

struct Headers {
    H h;
}

control c(in Headers h) {
    action a() {
    }
    action a_params(bit<32> param) {
    }
    table t_exact_ternary {
        key = {
            h.h.e: exact @name("h.h.e");
            h.h.t: ternary @name("h.h.t");
        }
        actions = {
            a();
            a_params();
        }
        default_action = a();
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("duplicate_priorities") entries = {
                        const priority=10: (32w0x1, 32w0x1111 &&& 32w0xf) : a_params(32w1);
                        priority=20: (32w0x2, 32w0x1181) : a_params(32w2);
                        priority=30: (32w0x3, 32w0x1000 &&& 32w0xf000) : a_params(32w3);
                        const priority=40: (32w0x4, 32w0x210 &&& 32w0x2f0) : a_params(32w4);
                        priority=40: (32w0x4, 32w0x10 &&& 32w0x2f0) : a_params(32w5);
                        priority=50: (32w0x6, default) : a_params(32w6);
        }
    }
    apply {
        bit<32> priority = 32w6;
        t_exact_ternary.apply();
    }
}

