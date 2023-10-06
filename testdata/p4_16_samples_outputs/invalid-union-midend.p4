#include <core.p4>

header E {
}

header H {
    bit<32> f;
}

header_union HU {
    E e;
    H h;
}

parser p(inout HU hu) {
    @name("ih_e") E ih_e_0;
    @name("ih_h") H ih_h_0;
    state start {
        transition select(ih_e_0.isValid()) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        hu.e.setValid();
        hu.e = ih_e_0;
        hu.h.setInvalid();
        transition start_join;
    }
    state start_false {
        hu.e.setInvalid();
        transition start_join;
    }
    state start_join {
        transition select(ih_h_0.isValid()) {
            true: start_true_0;
            false: start_false_0;
        }
    }
    state start_true_0 {
        hu.h.setValid();
        hu.h = ih_h_0;
        hu.e.setInvalid();
        transition start_join_0;
    }
    state start_false_0 {
        hu.h.setInvalid();
        transition start_join_0;
    }
    state start_join_0 {
        transition accept;
    }
}

control c(inout HU hu) {
    @name("c.h") H h_1;
    @name("c.a") action a_0() {
    }
    @hidden @name("invalidunion27") action invalidunion27_0() {
        h_1.setValid();
        h_1.f = 32w0;
        if (h_1.isValid()) {
            hu.h.setValid();
            hu.h = h_1;
            hu.e.setInvalid();
        } else {
            hu.h.setInvalid();
        }
    }
    @hidden @name("tbl_a") table tbl_a_0 {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    @hidden @name("tbl_invalidunion27") table tbl_invalidunion27_0 {
        actions = {
            invalidunion27_0();
        }
        const default_action = invalidunion27_0();
    }
    @name("tmp") bit<32> tmp_0;
    apply {
        tbl_a_0.apply();
        tmp_0 = 32w0;
        if (hu.e.isValid()) {
            tmp_0 = tmp_0 + 32w1;
        }
        if (hu.h.isValid()) {
            tmp_0 = tmp_0 + 32w1;
        }
        if (tmp_0 == 32w1) {
            ;
        } else {
            tbl_invalidunion27_0.apply();
        }
    }
}

parser P(inout HU h);
control C(inout HU h);
package top(P _p, C _c);
top(p(), c()) main;
