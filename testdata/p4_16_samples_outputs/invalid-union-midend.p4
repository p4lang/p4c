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
    E ih_0_e;
    H ih_0_h;
    state start {
        transition select((bit<1>)ih_0_e.isValid()) {
            1w1: start_true;
            1w0: start_false;
        }
    }
    state start_true {
        hu.e = ih_0_e;
        transition start_join;
    }
    state start_false {
        transition select((bit<1>)ih_0_h.isValid()) {
            1w1: start_false_true;
            1w0: start_false_false;
        }
    }
    state start_false_true {
        hu.h = ih_0_h;
        transition start_false_join;
    }
    state start_false_false {
        hu.e.setInvalid();
        transition start_false_join;
    }
    state start_false_join {
        transition start_join;
    }
    state start_join {
        transition accept;
    }
}

control c(inout HU hu) {
    @name("c.h") H h_1;
    @name("c.a") action a_0() {
    }
    @hidden action invalidunion27() {
        h_1.setValid();
        h_1.f = 32w0;
        hu.h = h_1;
    }
    @hidden table tbl_a {
        actions = {
            a_0();
        }
        const default_action = a_0();
    }
    @hidden table tbl_invalidunion27 {
        actions = {
            invalidunion27();
        }
        const default_action = invalidunion27();
    }
    bit<32> tmp;
    apply {
        tbl_a.apply();
        tmp = 32w0;
        if (hu.e.isValid()) {
            tmp = tmp + 32w1;
        }
        if (hu.h.isValid()) {
            tmp = tmp + 32w1;
        }
        if (tmp == 32w1) {
            ;
        } else {
            tbl_invalidunion27.apply();
        }
    }
}

parser P(inout HU h);
control C(inout HU h);
package top(P _p, C _c);
top(p(), c()) main;
