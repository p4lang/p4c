extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bool tmp_1;
    @hidden action complex9l12() {
        f(32w3);
        tmp_1 = false;
    }
    @hidden action complex9l12_0() {
        tmp_1 = false;
    }
    @hidden action act() {
        tmp = f(32w2);
    }
    @hidden action complex9l13() {
        r = 32w1;
    }
    @hidden action complex9l15() {
        r = 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_complex9l12 {
        actions = {
            complex9l12();
        }
        const default_action = complex9l12();
    }
    @hidden table tbl_complex9l12_0 {
        actions = {
            complex9l12_0();
        }
        const default_action = complex9l12_0();
    }
    @hidden table tbl_complex9l13 {
        actions = {
            complex9l13();
        }
        const default_action = complex9l13();
    }
    @hidden table tbl_complex9l15 {
        actions = {
            complex9l15();
        }
        const default_action = complex9l15();
    }
    apply {
        tbl_act.apply();
        if (tmp > 32w0) {
            tbl_complex9l12.apply();
        } else {
            tbl_complex9l12_0.apply();
        }
        if (tmp_1) {
            tbl_complex9l13.apply();
        } else {
            tbl_complex9l15.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
