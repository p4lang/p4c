#include <core.p4>

header inner_t {
    bit<8> f2;
}

header outer_t {
    bit<8> f1;
}

struct mid_t {
    inner_t[1] inner_arr;
}

struct Headers {
    outer_t[1] outer_arr;
    mid_t[1]   mid_arr;
}

control proto(inout Headers h);
package top(proto _c);
control ingress(inout Headers h) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a") action a() {
        h.mid_arr[3w0].inner_arr[3w0].f2 = 8w1;
    }
    @name("ingress.t") table t_0 {
        actions = {
            a();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

top(ingress()) main;
