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
    U u_0;
    U[2] u2_0;
    bool b_0;
    apply {
        b_0 = u_0.isValid();
        u_0.h1.isValid();
        x = u_0.h1.f + u_0.h2.g;
        u_0.h1.setValid();
        u_0.h1.f = 32w0;
        x = x + u_0.h1.f;
        u_0.h2.g = 32w0;
        x = x + u_0.h2.g;
        u2_0[0].h1.setValid();
        u2_0[0].h1.f = 32w2;
        x = x + u2_0[1].h2.g + u2_0[0].h1.f;
    }
}

top(c()) main;

