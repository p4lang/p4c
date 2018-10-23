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
    H s_0_h;
    H s1_0_h;
    bool eout_0;
    H tmp;
    @hidden action act() {
        h_0.setValid();
        h_0.x = 32w0;
        s_0_h.setValid();
        s1_0_h.setValid();
        s1_0_h.x = 32w0;
        h3_0[0].setValid();
        h3_0[1].setValid();
        h3_0[1].x = 32w1;
        tmp.setValid();
        tmp.x = 32w0;
        eout_0 = tmp.isValid();
        b = h_0.isValid() && eout_0 && h3_0[1].isValid() && s1_0_h.isValid();
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

