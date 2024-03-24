#include <core.p4>

enum bit<2> foo_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3
}

struct meta_t {
    bit<2> x;
    bit<6> y;
}

control c(inout meta_t m) {
    action set_x(foo_t v) {
        m.x = v;
    }
    table t {
        key = {
            m.y: exact;
        }
        actions = {
            set_x;
        }
        default_action = set_x(2w0);
    }
    apply {
        t.apply();
    }
}

