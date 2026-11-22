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
    @name("c.set_x") action set_x(@name("v") bit<2> v) {
        m.x = v;
    }
    @name("c.t") table t_0 {
        key = {
            m.y: exact @name("m.y");
        }
        actions = {
            set_x();
        }
        default_action = set_x(foo_t.A);
    }
    apply {
        t_0.apply();
    }
}

top<meta_t>(c()) main;
