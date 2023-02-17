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
    H1 u_0_h1;
    H2 u_0_h2;
    @name("c.u2") U[2] u2_0;
    @hidden action issue561l29() {
        u_0_h1.setInvalid();
        u_0_h2.setInvalid();
        u2_0[0].h1.setInvalid();
        u2_0[0].h2.setInvalid();
        u2_0[1].h1.setInvalid();
        u2_0[1].h2.setInvalid();
        x = u_0_h1.f + u_0_h2.g;
        u_0_h1.setValid();
        u_0_h2.setInvalid();
        u_0_h1.setValid();
        u_0_h1.f = 32w0;
        u_0_h2.setInvalid();
        x = x;
        u_0_h2.setValid();
        u_0_h2.g = 32w0;
        u_0_h1.setInvalid();
        x = x;
        u2_0[0].h1.setValid();
        u2_0[0].h2.setInvalid();
        u2_0[0].h1.setValid();
        u2_0[0].h1.f = 32w2;
        u2_0[0].h2.setInvalid();
        x = x + u2_0[1].h2.g + 32w2;
    }
    @hidden table tbl_issue561l29 {
        actions = {
            issue561l29();
        }
        const default_action = issue561l29();
    }
    apply {
        tbl_issue561l29.apply();
    }
}

top(c()) main;
