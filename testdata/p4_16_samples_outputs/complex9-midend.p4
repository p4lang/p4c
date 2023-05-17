extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bool tmp_1;
    @name("c.tmp_2") bit<32> tmp_2;
    @hidden action complex9l21() {
        tmp_2 = f(32w3);
        tmp_1 = tmp_2 < 32w0;
    }
    @hidden action complex9l21_0() {
        tmp_1 = false;
    }
    @hidden action act() {
        tmp = f(32w2);
    }
    @hidden action complex9l22() {
        r = 32w1;
    }
    @hidden action complex9l24() {
        r = 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_complex9l21 {
        actions = {
            complex9l21();
        }
        const default_action = complex9l21();
    }
    @hidden table tbl_complex9l21_0 {
        actions = {
            complex9l21_0();
        }
        const default_action = complex9l21_0();
    }
    @hidden table tbl_complex9l22 {
        actions = {
            complex9l22();
        }
        const default_action = complex9l22();
    }
    @hidden table tbl_complex9l24 {
        actions = {
            complex9l24();
        }
        const default_action = complex9l24();
    }
    apply {
        tbl_act.apply();
        if (tmp > 32w0) {
            tbl_complex9l21.apply();
        } else {
            tbl_complex9l21_0.apply();
        }
        if (tmp_1) {
            tbl_complex9l22.apply();
        } else {
            tbl_complex9l24.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
