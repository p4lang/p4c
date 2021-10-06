extern bit<32> f(in bit<32> x);
header H {
    bit<32> v;
}

control c(inout bit<32> r) {
    @name("c.h") H[2] h_0;
    @name("c.tmp") bit<32> tmp;
    @hidden action complex2l24() {
        h_0[0].setInvalid();
        h_0[1].setInvalid();
        tmp = f(32w2);
        if (tmp == 32w0) {
            h_0[0].setValid();
        } else if (tmp == 32w1) {
            h_0[1].setValid();
        }
    }
    @hidden table tbl_complex2l24 {
        actions = {
            complex2l24();
        }
        const default_action = complex2l24();
    }
    apply {
        tbl_complex2l24.apply();
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;

