#include <core.p4>

header H {
    bit<1>  a;
    bool    b;
    bit<32> c;
}

struct S {
    bit<1> i;
    H      h;
    bool   b;
}

struct tuple_0 {
    bool f0;
    H    f1;
    S    f2;
}

control c(inout S r) {
    H rightTmp_1_h;
    @name("c.hdr_or_hu") H hdr_or_hu_3;
    @name("c.hdr_or_hu_0") H hdr_or_hu_4;
    H leftTmp_0_h;
    H s_0_h;
    H sr_0_h;
    @name("c.hdr_or_hu_1") H hdr_or_hu_5;
    @name("c.hdr_or_hu_2") H hdr_or_hu_6;
    H tmp_h;
    bool tmp_b;
    @hidden action defaultvaluesoperations1l23() {
        r.i = 1w0;
    }
    @hidden action defaultvaluesoperations1l20() {
        rightTmp_1_h.setInvalid();
        hdr_or_hu_3.setInvalid();
        hdr_or_hu_4.setInvalid();
        leftTmp_0_h.setInvalid();
        s_0_h.setValid();
        s_0_h.a = 1w0;
        s_0_h.b = false;
        s_0_h.c = 32w1;
        sr_0_h.setInvalid();
    }
    @hidden action defaultvaluesoperations1l31() {
        tmp_h = s_0_h;
        tmp_b = true;
    }
    @hidden action defaultvaluesoperations1l31_0() {
        tmp_h = sr_0_h;
        tmp_b = false;
    }
    @hidden action defaultvaluesoperations1l26() {
        hdr_or_hu_5.setInvalid();
        hdr_or_hu_5.setInvalid();
        hdr_or_hu_6.setInvalid();
        hdr_or_hu_6.setInvalid();
    }
    @hidden action defaultvaluesoperations1l31_1() {
        s_0_h = tmp_h;
        r.b = tmp_b;
    }
    @hidden table tbl_defaultvaluesoperations1l20 {
        actions = {
            defaultvaluesoperations1l20();
        }
        const default_action = defaultvaluesoperations1l20();
    }
    @hidden table tbl_defaultvaluesoperations1l23 {
        actions = {
            defaultvaluesoperations1l23();
        }
        const default_action = defaultvaluesoperations1l23();
    }
    @hidden table tbl_defaultvaluesoperations1l26 {
        actions = {
            defaultvaluesoperations1l26();
        }
        const default_action = defaultvaluesoperations1l26();
    }
    @hidden table tbl_defaultvaluesoperations1l31 {
        actions = {
            defaultvaluesoperations1l31();
        }
        const default_action = defaultvaluesoperations1l31();
    }
    @hidden table tbl_defaultvaluesoperations1l31_0 {
        actions = {
            defaultvaluesoperations1l31_0();
        }
        const default_action = defaultvaluesoperations1l31_0();
    }
    @hidden table tbl_defaultvaluesoperations1l31_1 {
        actions = {
            defaultvaluesoperations1l31_1();
        }
        const default_action = defaultvaluesoperations1l31_1();
    }
    apply {
        tbl_defaultvaluesoperations1l20.apply();
        if ((!s_0_h.isValid() && !rightTmp_1_h.isValid() || s_0_h.isValid() && rightTmp_1_h.isValid() && 1w0 == rightTmp_1_h.a && !rightTmp_1_h.b && 32w1 == rightTmp_1_h.c) && false) {
            tbl_defaultvaluesoperations1l23.apply();
        }
        tbl_defaultvaluesoperations1l26.apply();
        if (!leftTmp_0_h.isValid() && !s_0_h.isValid() || leftTmp_0_h.isValid() && s_0_h.isValid() && leftTmp_0_h.a == 1w0 && !leftTmp_0_h.b && leftTmp_0_h.c == 32w1) {
            tbl_defaultvaluesoperations1l31.apply();
        } else {
            tbl_defaultvaluesoperations1l31_0.apply();
        }
        tbl_defaultvaluesoperations1l31_1.apply();
    }
}

control simple(inout S r);
package top(simple e);
top(c()) main;

