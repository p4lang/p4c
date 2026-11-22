#include <core.p4>

control generic<M>(inout M m);
package top<M>(generic<M> c);
enum bit<2> foo_t {
    A = 2w0,
    B = 2w1,
    C = 2w2,
    D = 2w3
}

struct meta_t {
    bit<2> x;
    bit<6> y;
}

control c(inout meta_t m) {
    action set_x(bit<2> v) {
        m.x = v;
    }
    table t {
        key = {
            m.y: exact @name("m.y");
        }
        actions = {
            set_x();
        }
        default_action = set_x(foo_t.A);
    }
    apply {
        t.apply();
    }
}

top<meta_t>(c()) main;
