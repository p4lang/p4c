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
    h_t ih;
    h_t ih_0;
    h_t ih_1;
    h_t ih_2_a;
    h2_t ih_2_b;
    h_t ih_3;
    @name("C.a") action a_0() {
        ih.setInvalid();
        ih_0.setInvalid();
        ih_1.setInvalid();
        ih_3.setInvalid();
        if (!meta.h.isValid() && !ih.isValid() || meta.h.isValid() && ih.isValid() && meta.h.f == ih.f) {
            meta.f = 8w1;
        }
        if (!meta.h.isValid() && !ih_0.isValid() || meta.h.isValid() && ih_0.isValid() && meta.h.f == ih_0.f) {
            ;
        } else {
            meta.f = 8w2;
        }
        meta.h = ih_1;
        if (meta.h.isValid()) {
            ;
        } else {
            meta.f = 8w3;
        }
        if (ih_2_a.isValid()) {
            hu.a.setValid();
            hu.a = ih_2_a;
            hu.b.setInvalid();
        } else {
            hu.a.setInvalid();
        }
        if (ih_2_b.isValid()) {
            hu.b.setValid();
            hu.b = ih_2_b;
            hu.a.setInvalid();
        } else {
            hu.b.setInvalid();
        }
        if (!nested.m.h.isValid() && !ih_3.isValid() || nested.m.h.isValid() && ih_3.isValid() && nested.m.h.f == ih_3.f) {
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
    h_t ih_4;
    h_t ih_5;
    state start {
        ih_4.setInvalid();
        ih_5.setInvalid();
        meta.h = ih_4;
        transition select((bit<1>)(!meta.h.isValid() && !ih_5.isValid() || meta.h.isValid() && ih_5.isValid() && meta.h.f == ih_5.f)) {
            1w1: start_true;
            1w0: start_join;
            default: noMatch;
        }
    }
    state start_true {
        meta.h.setInvalid();
        transition start_join;
    }
    state start_join {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control proto(inout meta_t meta, inout nested_t nested, inout hu_t hu);
parser par(inout meta_t meta, inout nested_t nested, inout hu_t hu);
package top(par p, proto c);
top(P(), C()) main;
