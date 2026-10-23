#include <core.p4>

match_kind {
    optional
}

struct meta_t {
    error e1;
    error e2;
}

control C(inout meta_t m);
package top(C c);
control MyControl(inout meta_t m) {
    action noop() {
    }
    table t_exact {
        key = {
            m.e1: exact;
        }
        actions = {
            noop;
        }
    }
    table t_optional {
        key = {
            m.e2: optional;
        }
        actions = {
            noop;
        }
    }
    apply {
        t_exact.apply();
        t_optional.apply();
    }
}

top(MyControl()) main;
