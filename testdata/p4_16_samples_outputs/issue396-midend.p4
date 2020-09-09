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
    H s1_0_h;
    H tmp;
    @hidden action issue396l39() {
        h_0.setValid();
        h_0.x = 32w0;
        s1_0_h.setValid();
        s1_0_h.x = 32w0;
        h3_0[1].setValid();
        h3_0[1].x = 32w1;
        tmp.setValid();
        tmp.x = 32w0;
        b = h_0.isValid() && tmp.isValid() && h3_0[1].isValid() && s1_0_h.isValid();
    }
    @hidden table tbl_issue396l39 {
        actions = {
            issue396l39();
        }
        const default_action = issue396l39();
    }
    apply {
        tbl_issue396l39.apply();
    }
}

top(d()) main;

