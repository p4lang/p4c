header H {
    bit<32>    a;
    varbit<32> b;
}

struct S {
    bit<32> a;
    H       h;
}

control c(out bit<1> x) {
    @name("c.a") varbit<32> a_0;
    @name("c.b") varbit<32> b_0;
    @name("c.h1") H h1_0;
    @name("c.h2") H h2_0;
    bit<32> s1_0_a;
    H s1_0_h;
    bit<32> s2_0_a;
    H s2_0_h;
    @name("c.a1") H[2] a1_0;
    @name("c.a2") H[2] a2_0;
    @hidden action equality29() {
        x = 1w1;
    }
    @hidden action equality31() {
        x = 1w1;
    }
    @hidden action equality33() {
        x = 1w1;
    }
    @hidden action equality35() {
        x = 1w1;
    }
    @hidden action equality37() {
        x = 1w0;
    }
    @hidden action equality20() {
        h1_0.setInvalid();
        h2_0.setInvalid();
        s1_0_h.setInvalid();
        s2_0_h.setInvalid();
        a1_0[0].setInvalid();
        a1_0[1].setInvalid();
        a2_0[0].setInvalid();
        a2_0[1].setInvalid();
    }
    @hidden table tbl_equality20 {
        actions = {
            equality20();
        }
        const default_action = equality20();
    }
    @hidden table tbl_equality29 {
        actions = {
            equality29();
        }
        const default_action = equality29();
    }
    @hidden table tbl_equality31 {
        actions = {
            equality31();
        }
        const default_action = equality31();
    }
    @hidden table tbl_equality33 {
        actions = {
            equality33();
        }
        const default_action = equality33();
    }
    @hidden table tbl_equality35 {
        actions = {
            equality35();
        }
        const default_action = equality35();
    }
    @hidden table tbl_equality37 {
        actions = {
            equality37();
        }
        const default_action = equality37();
    }
    apply {
        tbl_equality20.apply();
        if (a_0 == b_0) {
            tbl_equality29.apply();
        } else if (!h1_0.isValid() && !h2_0.isValid() || h1_0.isValid() && h2_0.isValid() && h1_0.a == h2_0.a && h1_0.b == h2_0.b) {
            tbl_equality31.apply();
        } else if (s1_0_a == s2_0_a && (!s1_0_h.isValid() && !s2_0_h.isValid() || s1_0_h.isValid() && s2_0_h.isValid() && s1_0_h.a == s2_0_h.a && s1_0_h.b == s2_0_h.b)) {
            tbl_equality33.apply();
        } else if ((!a1_0[0].isValid() && !a2_0[0].isValid() || a1_0[0].isValid() && a2_0[0].isValid() && a1_0[0].a == a2_0[0].a && a1_0[0].b == a2_0[0].b) && (!a1_0[1].isValid() && !a2_0[1].isValid() || a1_0[1].isValid() && a2_0[1].isValid() && a1_0[1].a == a2_0[1].a && a1_0[1].b == a2_0[1].b)) {
            tbl_equality35.apply();
        } else {
            tbl_equality37.apply();
        }
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;
