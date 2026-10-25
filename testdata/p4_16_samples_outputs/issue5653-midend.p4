#include <core.p4>

header h_t {
    bit<8> f;
}

struct meta_t {
    h_t    h;
    bit<8> f;
}

control C(inout meta_t meta) {
    h_t ih;
    h_t ih_0;
    h_t ih_1;
    h_t ih_2;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.a") action a() {
        ih.setInvalid();
        ih_0.setInvalid();
        ih_1.setInvalid();
        ih_2.setInvalid();
        meta.h = ih;
        if (!meta.h.isValid() && !ih_0.isValid() || meta.h.isValid() && ih_0.isValid() && meta.h.f == ih_0.f) {
            meta.f = 8w1;
        }
        if (!meta.h.isValid() && !ih_1.isValid() || meta.h.isValid() && ih_1.isValid() && meta.h.f == ih_1.f) {
            ;
        } else {
            meta.f = 8w2;
        }
        meta.h = ih_2;
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
