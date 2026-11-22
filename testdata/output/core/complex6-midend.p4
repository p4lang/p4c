extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bit<32> tmp_1;
    @hidden action complex6l14() {
        r = 32w1;
    }
    @hidden action complex6l16() {
        r = 32w3;
    }
    @hidden action complex6l13() {
        tmp_1 = f(32w2);
    }
    @hidden action complex6l18() {
        r = 32w2;
    }
    @hidden action complex6l12() {
        tmp = f(32w2);
    }
    @hidden table tbl_complex6l12 {
        actions = {
            complex6l12();
        }
        const default_action = complex6l12();
    }
    @hidden table tbl_complex6l13 {
        actions = {
            complex6l13();
        }
        const default_action = complex6l13();
    }
    @hidden table tbl_complex6l14 {
        actions = {
            complex6l14();
        }
        const default_action = complex6l14();
    }
    @hidden table tbl_complex6l16 {
        actions = {
            complex6l16();
        }
        const default_action = complex6l16();
    }
    @hidden table tbl_complex6l18 {
        actions = {
            complex6l18();
        }
        const default_action = complex6l18();
    }
    apply {
        tbl_complex6l12.apply();
        if (tmp > 32w0) {
            tbl_complex6l13.apply();
            if (tmp_1 < 32w2) {
                tbl_complex6l14.apply();
            } else {
                tbl_complex6l16.apply();
            }
        } else {
            tbl_complex6l18.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
