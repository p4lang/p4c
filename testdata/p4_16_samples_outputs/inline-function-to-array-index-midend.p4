#include <core.p4>

header h_t {
}

struct Headers {
    h_t[1] hdr_arr;
}

control proto(inout Headers h);
package top(proto _c);
control ingress(inout Headers h) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.a") action a() {
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
