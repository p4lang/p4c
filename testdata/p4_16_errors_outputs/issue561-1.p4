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

control ct();
package top(ct _ct);
control c() {
    apply {
        U u = { { 10 }, { 20 } };
        u.setValid();
    }
}

top(c()) main;

