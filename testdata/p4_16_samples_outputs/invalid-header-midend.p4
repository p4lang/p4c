header E {
}

header H {
    bit<32> f;
}

parser p(inout H h) {
    H ih;
    state start {
        ih.setInvalid();
        h = ih;
        transition accept;
    }
}

control c(inout H h, inout E e) {
    H ih_1;
    E ih_2;
    @name("c.x") bit<32> x_0;
    H ih_0;
    @name("c.a") action a() {
        ih_0.setInvalid();
        h = ih_0;
    }
    @hidden action invalidheader19() {
        ih_1.setInvalid();
        ih_2.setInvalid();
        h = ih_1;
        e = ih_2;
        x_0 = h.f;
    }
    @hidden action invalidheader24() {
        h.setValid();
        h.f = x_0;
    }
    @hidden table tbl_invalidheader19 {
        actions = {
            invalidheader19();
        }
        const default_action = invalidheader19();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    @hidden table tbl_invalidheader24 {
        actions = {
            invalidheader24();
        }
        const default_action = invalidheader24();
    }
    apply {
        tbl_invalidheader19.apply();
        tbl_a.apply();
        if (e.isValid()) {
            tbl_invalidheader24.apply();
        }
    }
}

parser P(inout H h);
control C(inout H h, inout E e);
package top(P _p, C _c);
top(p(), c()) main;
