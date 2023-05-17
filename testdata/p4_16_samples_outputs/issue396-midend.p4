header H {
    bit<32> x;
}

struct S {
    H h;
}

control c(out bool b);
package top(c _c);
control d(out bool b) {
    @name("d.h") H h_0;
    @name("d.h3") H[2] h3_0;
    H s_0_h;
    H s1_0_h;
    @name("d.tmp") H tmp;
    @hidden action issue396l36() {
        h_0.setInvalid();
        h3_0[0].setInvalid();
        h3_0[1].setInvalid();
        h_0.setValid();
        h_0.x = 32w0;
        s_0_h.setInvalid();
        s1_0_h.setInvalid();
        s1_0_h.setValid();
        s1_0_h.x = 32w0;
        h3_0[1].setValid();
        h3_0[1].x = 32w1;
        tmp.setValid();
        tmp.x = 32w0;
        b = h_0.isValid() && tmp.isValid() && h3_0[1].isValid() && s1_0_h.isValid();
    }
    @hidden table tbl_issue396l36 {
        actions = {
            issue396l36();
        }
        const default_action = issue396l36();
    }
    apply {
        tbl_issue396l36.apply();
    }
}

top(d()) main;
