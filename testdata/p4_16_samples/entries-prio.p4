#include <core.p4>

header H {
    bit<32> e;
    bit<32> t;
}

struct Headers {
    H h;
}

control c(in Headers h) {
    action a() {}
    action a_params(bit<32> param) {}

    table t_exact_ternary {
        key = {
            h.h.e : exact;
            h.h.t : ternary;
        }

        actions = {
            a;
            a_params;
        }

        default_action = a;

        largest_priority_wins = false;
        priority_delta = 10;

 	@noWarn("duplicate_priorities")
        entries = {
            const priority=10: (0x01, 0x1111 &&& 0xF   ) : a_params(1);
                               (0x02, 0x1181           ) : a_params(2); // priority=20
                               (0x03, 0x1000 &&& 0xF000) : a_params(3); // priority=30
            const              (0x04, 0x0210 &&& 0x02F0) : a_params(4); // priority=40
                  priority=40: (0x04, 0x0010 &&& 0x02F0) : a_params(5);
                               (0x06, _                ) : a_params(6); // priority=50
        }
    }

    apply {
        bit<32> priority = 6;
        t_exact_ternary.apply();
    }
}