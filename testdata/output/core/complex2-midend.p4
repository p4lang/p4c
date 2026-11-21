extern bit<32> f(in bit<32> x);
header H {
    bit<32> v;
}

control c(inout bit<32> r) {
    @name("c.h") H[2] h_0;
    @name("c.tmp") bit<32> tmp;
    @hidden action complex2l16() {
        h_0[32w0].setValid();
    }
    @hidden action complex2l16_0() {
        h_0[32w1].setValid();
    }
    @hidden action complex2l15() {
        h_0[0].setInvalid();
        h_0[1].setInvalid();
        tmp = f(32w2);
    }
    @hidden table tbl_complex2l15 {
        actions = {
            complex2l15();
        }
        const default_action = complex2l15();
    }
    @hidden table tbl_complex2l16 {
        actions = {
            complex2l16();
        }
        const default_action = complex2l16();
    }
    @hidden table tbl_complex2l16_0 {
        actions = {
            complex2l16_0();
        }
        const default_action = complex2l16_0();
    }
    apply {
        tbl_complex2l15.apply();
        if (tmp == 32w0) {
            tbl_complex2l16.apply();
        } else if (tmp == 32w1) {
            tbl_complex2l16_0.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
