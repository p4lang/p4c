#include <core.p4>
control generic<M>(inout M m);
package top<M>(generic<M> c);

enum bit<2> foo_t { A = 0, B = 1, C = 2, D = 3 }

struct meta_t {
    bit<2>      x;
    bit<6>      y;
}

control c(inout meta_t m) {
    action set_x(bit<2> v) { m.x = v; }

    table t {
        key = { m.y : exact; }
        actions = { set_x; }
        default_action = set_x(foo_t.A);
    }

    apply {
        t.apply();
    }
}

top(c()) main;
