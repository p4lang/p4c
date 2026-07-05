#include <core.p4>

header h_t {
    bit<8> f;
}

struct meta_t {
    h_t    h;
    bit<8> f;
}

control C(inout meta_t meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.a") action a() {
        meta.h = (h_t){#};
        if (meta.h == (h_t){#}) {
            meta.f = 8w1;
        }
        if (meta.h != (h_t){#}) {
            meta.f = 8w2;
        }
        meta.h = (h_t){#};
        if (meta.h.isValid()) {
            ;
        } else {
            meta.f = 8w3;
        }
    }
    @name("C.t") table t_0 {
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

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;
