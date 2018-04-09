header H1 {
    bit<32> f;
}

header H2 {
    bit<32> g;
}

header_union U {
    H1 h1;
    H2 h2;
}

control ct(out bit<32> b);
package top(ct _ct);
control c(out bit<32> x) {
    U u;
    U[2] u2;
    @hidden action act() {
        u.isValid();
        u.h1.isValid();
        x = u.h1.f + u.h2.g;
        u.h1.setValid();
        u.h1.f = 32w0;
        x = x + 32w0;
        u.h2.g = 32w0;
        x = x + 32w0;
        u2[0].h1.setValid();
        u2[0].h1.f = 32w2;
        x = x + u2[1].h2.g + 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

top(c()) main;

