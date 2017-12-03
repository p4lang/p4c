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
    H s_h;
    H s1_h;
    bool eout;
    H tmp_0;
    @hidden action act() {
        h_1.setValid();
        h_1.x = 32w0;
        s_h.setValid();
        s1_h.setValid();
        h3[0].setValid();
        h3[1].setValid();
        tmp_0.setValid();
        tmp_0.x = 32w0;
        eout = tmp_0.isValid();
        b = h_1.isValid() && eout;
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

