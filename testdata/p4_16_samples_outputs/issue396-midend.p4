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
    H s_0_h;
    H s1_0_h;
    bool eout_0;
    H tmp;
    @hidden action act() {
        h_0.setValid();
        h_0.x = 32w0;
        s_0_h.setValid();
        s1_0_h.setValid();
        h3_0[0].setValid();
        h3_0[1].setValid();
        tmp.setValid();
        tmp.x = 32w0;
        eout_0 = tmp.isValid();
        b = h_0.isValid() && eout_0;
=======
    H h_1;
    H[2] h3;
    H s_h;
    H s1_h;
    bool eout;
    H tmp_0;
    @hidden action act() {
        h_1.setValid();
        h_1.x = 32w0;
        s_h.setValid();
        s1_h.setValid();
        s1_h.x = 32w0;
        h3[0].setValid();
        h3[1].setValid();
        h3[1].x = 32w1;
        tmp_0.setValid();
        tmp_0.x = 32w0;
        eout = tmp_0.isValid();
<<<<<<< 8af0e6b445341eac6e0f948de9f7331dab1b9c46
        b = h_1.isValid() && eout;
>>>>>>> Implemented struct initializers - currently inferred by type inference
=======
        b = h_1.isValid() && eout && h3[1].isValid() && s1_h.isValid();
>>>>>>> Moved structure initializer creation to a separate pass
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

top(d()) main;

