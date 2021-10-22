extern bit<32> f(in bit<32> x);
header H {
    bit<32> v;
}

control c(inout bit<32> r) {
    H hsVar0;
    @name("c.h") H[2] h_0;
    @name("c.tmp") bit<32> tmp;
    @hidden action complex2l25() {
        h_0[32w0].setValid();
    }
    @hidden action complex2l25_0() {
        h_0[32w1].setValid();
    }
    @hidden action complex2l25_1() {
        h_0[32w1].setValid();
    }
    @hidden action complex2l25_2() {
        h_0[32w1] = hsVar0;
    }
    @hidden action complex2l24() {
        h_0[0].setInvalid();
        h_0[1].setInvalid();
        tmp = f(32w2);
    }
    @hidden table tbl_complex2l24 {
        actions = {
            complex2l24();
        }
        const default_action = complex2l24();
    }
    @hidden table tbl_complex2l25 {
        actions = {
            complex2l25();
        }
        const default_action = complex2l25();
    }
    @hidden table tbl_complex2l25_0 {
        actions = {
            complex2l25_0();
        }
        const default_action = complex2l25_0();
    }
    @hidden table tbl_complex2l25_1 {
        actions = {
            complex2l25_2();
        }
        const default_action = complex2l25_2();
    }
    @hidden table tbl_complex2l25_2 {
        actions = {
            complex2l25_1();
        }
        const default_action = complex2l25_1();
    }
    apply {
        tbl_complex2l24.apply();
        if (tmp == 32w0) {
            tbl_complex2l25.apply();
        } else if (tmp == 32w1) {
            tbl_complex2l25_0.apply();
        } else {
            tbl_complex2l25_1.apply();
            if (tmp >= 32w1) {
                tbl_complex2l25_2.apply();
            }
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

