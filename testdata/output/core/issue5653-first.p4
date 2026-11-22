#include <core.p4>

header h_t {
    bit<8> f;
}

struct meta_t {
    h_t    h;
    bit<8> f;
}

control C(inout meta_t meta) {
    action a() {
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
    table t {
        actions = {
            a();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;
