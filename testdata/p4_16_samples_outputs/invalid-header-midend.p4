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
    @hidden action invalidheader25() {
        ih_1.setInvalid();
        ih_2.setInvalid();
        h = ih_1;
        e = ih_2;
        x_0 = h.f;
    }
    @hidden action invalidheader30() {
        h.setValid();
        h.f = x_0;
    }
    @hidden table tbl_invalidheader25 {
        actions = {
            invalidheader25();
        }
        const default_action = invalidheader25();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    @hidden table tbl_invalidheader30 {
        actions = {
            invalidheader30();
        }
        const default_action = invalidheader30();
    }
    apply {
        tbl_invalidheader25.apply();
        tbl_a.apply();
        if (e.isValid()) {
            tbl_invalidheader30.apply();
        }
    }
}

parser P(inout H h);
control C(inout H h, inout E e);
package top(P _p, C _c);
top(p(), c()) main;
