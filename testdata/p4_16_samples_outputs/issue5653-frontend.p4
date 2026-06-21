#include <core.p4>

header h_t {
    bit<8> f;
}

header h2_t {
    bit<16> g;
}

header_union hu_t {
    h_t  a;
    h2_t b;
}

struct meta_t {
    h_t    h;
    bit<8> f;
}

struct nested_t {
    meta_t m;
}

control C(inout meta_t meta, inout nested_t nested, inout hu_t hu) {
    @name("C.a") action a_0() {
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
        hu = (hu_t){#};
        if (nested.m.h == (h_t){#}) {
            nested.m.f = 8w4;
        }
    }
    @name("C.t") table t {
        actions = {
            a_0();
        }
        default_action = a_0();
    }
    apply {
        t.apply();
    }
}

parser P(inout meta_t meta, inout nested_t nested, inout hu_t hu) {
    state start {
        meta.h = (h_t){#};
        transition select(meta.h == (h_t){#}) {
            true: start_true;
            false: start_join;
        }
    }
    state start_true {
        meta.h.setInvalid();
        transition start_join;
    }
    state start_join {
        transition accept;
    }
}

control proto(inout meta_t meta, inout nested_t nested, inout hu_t hu);
parser par(inout meta_t meta, inout nested_t nested, inout hu_t hu);
package top(par p, proto c);
top(P(), C()) main;
