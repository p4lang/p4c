header H {
    bit<32> x;
}

struct S {
    H h;
}

control c(out bool b);
package top(c _c);
control d(out bool b) {
<<<<<<< c32cb7a0dac7bb9d5abd1dbee292690508bf513c
    H h_0;
    H[2] h3_0;
    S s_0;
    S s1_0;
    bool eout_0;
    H tmp;
    apply {
        h_0.setValid();
        h_0 = { 32w0 };
        s_0.h.setValid();
        s1_0.h.setValid();
        h3_0[0].setValid();
        h3_0[1].setValid();
        tmp.setValid();
        tmp = { 32w0 };
        eout_0 = tmp.isValid();
        b = h_0.isValid() && eout_0;
=======
    H h_1;
    H[2] h3;
    S s;
    S s1;
    bool eout;
    H tmp_0;
    apply {
        h_1.setValid();
        h_1 = {32w0};
        s.h.setValid();
        s1.h.setValid();
        s1.h = {32w0};
        h3[0].setValid();
        h3[1].setValid();
        h3[1] = {32w1};
        tmp_0.setValid();
        tmp_0 = {32w0};
        eout = tmp_0.isValid();
<<<<<<< 8af0e6b445341eac6e0f948de9f7331dab1b9c46
        b = h_1.isValid() && eout;
>>>>>>> Implemented struct initializers - currently inferred by type inference
=======
        b = h_1.isValid() && eout && h3[1].isValid() && s1.h.isValid();
>>>>>>> Moved structure initializer creation to a separate pass
    }
}

top(d()) main;

