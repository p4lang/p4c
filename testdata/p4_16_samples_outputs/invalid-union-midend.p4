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
    HU ih;
    state start {
        hu = ih;
        transition accept;
    }
}

control c(inout HU hu) {
    @name("c.h") H h_0;
    @name("c.a") action a() {
    }
    @hidden action invalidunion27() {
        h_0.setValid();
        h_0.f = 32w0;
        if (h_0.isValid()) {
            hu.h.setValid();
            hu.h = h_0;
            hu.e.setInvalid();
        } else {
            hu.h.setInvalid();
        }
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
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
