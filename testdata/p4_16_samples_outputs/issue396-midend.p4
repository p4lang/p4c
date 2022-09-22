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
        s_0_h.setInvalid();
        s1_0_h.setInvalid();
        s1_0_h.setValid();
        h3_0[1].setValid();
        tmp.setValid();
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

