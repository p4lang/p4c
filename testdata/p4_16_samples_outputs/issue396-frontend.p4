header H {
    bit<32> x;
}

struct S {
    H h;
}

control c(out bool b);
package top(c _c);
control d(out bool b) {
    H h_1;
    H[2] h3;
    S s;
    S s1;
    bool eout;
    H tmp_0;
    apply {
        h_1.setValid();
        h_1 = { 32w0 };
        s.h.setValid();
        s1.h.setValid();
        h3[0].setValid();
        h3[1].setValid();
        tmp_0.setValid();
        tmp_0 = { 32w0 };
        eout = tmp_0.isValid();
        b = h_1.isValid() && eout;
    }
}

top(d()) main;

