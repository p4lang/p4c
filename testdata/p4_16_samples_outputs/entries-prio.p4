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
            h.h.e: exact;
            h.h.t: ternary;
        }
        actions = {
            a;
            a_params;
        }
        default_action = a;
        largest_priority_wins = false;
        priority_delta = 10;
        @noWarn("duplicate_priorities") entries = {
                        const priority=10: (0x1, 0x1111 &&& 0xf) : a_params(1);
                        (0x2, 0x1181) : a_params(2);
                        (0x3, 0x1000 &&& 0xf000) : a_params(3);
                        const (0x4, 0x210 &&& 0x2f0) : a_params(4);
                        priority=40: (0x4, 0x10 &&& 0x2f0) : a_params(5);
                        (0x6, default) : a_params(6);
        }
    }
    apply {
        bit<32> priority = 6;
        t_exact_ternary.apply();
    }
}

