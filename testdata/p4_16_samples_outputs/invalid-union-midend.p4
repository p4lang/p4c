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
    bit<32> tmp;
    @name("c.a") action a() {
    }
    @hidden action invalidunion24() {
        tmp = 32w1;
    }
    @hidden action invalidunion24_0() {
        tmp = 32w0;
    }
    @hidden action invalidunion24_1() {
        tmp = tmp + 32w1;
    }
    @hidden action invalidunion27() {
        hu.h.setValid();
        hu.h = h_0;
        hu.e.setInvalid();
    }
    @hidden action invalidunion27_0() {
        hu.h.setInvalid();
    }
    @hidden action invalidunion25() {
        h_0.setValid();
        h_0.f = 32w0;
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    @hidden table tbl_invalidunion24 {
        actions = {
            invalidunion24_0();
        }
        const default_action = invalidunion24_0();
    }
    @hidden table tbl_invalidunion24_0 {
        actions = {
            invalidunion24();
        }
        const default_action = invalidunion24();
    }
    @hidden table tbl_invalidunion24_1 {
        actions = {
            invalidunion24_1();
        }
        const default_action = invalidunion24_1();
    }
    @hidden table tbl_invalidunion25 {
        actions = {
            invalidunion25();
        }
        const default_action = invalidunion25();
    }
    @hidden table tbl_invalidunion27 {
        actions = {
            invalidunion27();
        }
        const default_action = invalidunion27();
    }
    @hidden table tbl_invalidunion27_0 {
        actions = {
            invalidunion27_0();
        }
        const default_action = invalidunion27_0();
    }
    apply {
        tbl_a.apply();
        tbl_invalidunion24.apply();
        if (hu.e.isValid()) {
            tbl_invalidunion24_0.apply();
        }
        if (hu.h.isValid()) {
            tbl_invalidunion24_1.apply();
        }
        if (tmp == 32w1) {
            ;
        } else {
            tbl_invalidunion25.apply();
            if (h_0.isValid()) {
                tbl_invalidunion27.apply();
            } else {
                tbl_invalidunion27_0.apply();
            }
        }
    }
}

parser P(inout HU h);
control C(inout HU h);
package top(P _p, C _c);
top(p(), c()) main;
