#include <core.p4>

match_kind {
    range
}

struct meta_t {
    error e1;
    error e2;
    error e3;
}

control C(inout meta_t m);
package top(C c);
control MyControl(inout meta_t m) {
    action noop() {
    }
    table t_ternary {
        key = {
            m.e1: ternary;
        }
        actions = {
            noop;
        }
    }
    table t_lpm {
        key = {
            m.e2: lpm;
        }
        actions = {
            noop;
        }
    }
    table t_range {
        key = {
            m.e3: range;
        }
        actions = {
            noop;
        }
    }
    apply {
        t_ternary.apply();
        t_lpm.apply();
        t_range.apply();
    }
}

top(MyControl()) main;
