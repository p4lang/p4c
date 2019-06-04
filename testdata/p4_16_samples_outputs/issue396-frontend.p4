header H {
    bit<32> x;
}

struct S {
    H h;
}

control c(out bool b);
package top(c _c);
control d(out bool b) {
    H h_0;
    H[2] h3_0;
    S s_0;
    S s1_0;
    bool eout_0;
    H tmp;
    apply {
        h_0.setValid();
        h_0 = H {x = 32w0};
        s_0.h.setValid();
        s1_0.h.setValid();
        s1_0.h = H {x = 32w0};
        h3_0[0].setValid();
        h3_0[1].setValid();
        h3_0[1] = H {x = 32w1};
        tmp.setValid();
        tmp = H {x = 32w0};
        eout_0 = tmp.isValid();
        b = h_0.isValid() && eout_0 && h3_0[1].isValid() && s1_0.h.isValid();
    }
}

top(d()) main;

