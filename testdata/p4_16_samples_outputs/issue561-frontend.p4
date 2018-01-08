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
    bool b_1;
    apply {
        b_1 = u.isValid();
        u.h1.isValid();
        x = u.h1.f + u.h2.g;
        u.h1.setValid();
        u.h1.f = 32w0;
        x = x + u.h1.f;
        u.h2.g = 32w0;
        x = x + u.h2.g;
        u2[0].h1.setValid();
        u2[0].h1.f = 32w2;
        x = x + u2[1].h2.g + u2[0].h1.f;
    }
}

top(c()) main;

