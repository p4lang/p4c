extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bool tmp_1;
    @name("c.tmp_4") bool tmp_4;
    @name("c.tmp_5") bit<32> tmp_5;
    @hidden action complex10l12() {
        f(32w3);
        tmp_1 = false;
    }
    @hidden action complex10l12_0() {
        tmp_1 = false;
    }
    @hidden action act() {
        tmp = f(32w2);
    }
    @hidden action complex10l12_1() {
        tmp_4 = true;
    }
    @hidden action complex10l12_2() {
        tmp_5 = f(32w5);
        tmp_4 = tmp_5 == 32w2;
    }
    @hidden action complex10l13() {
        r = 32w1;
    }
    @hidden action complex10l15() {
        r = 32w2;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_complex10l12 {
        actions = {
            complex10l12();
        }
        const default_action = complex10l12();
    }
    @hidden table tbl_complex10l12_0 {
        actions = {
            complex10l12_0();
        }
        const default_action = complex10l12_0();
    }
    @hidden table tbl_complex10l12_1 {
        actions = {
            complex10l12_1();
        }
        const default_action = complex10l12_1();
    }
    @hidden table tbl_complex10l12_2 {
        actions = {
            complex10l12_2();
        }
        const default_action = complex10l12_2();
    }
    @hidden table tbl_complex10l13 {
        actions = {
            complex10l13();
        }
        const default_action = complex10l13();
    }
    @hidden table tbl_complex10l15 {
        actions = {
            complex10l15();
        }
        const default_action = complex10l15();
    }
    apply {
        tbl_act.apply();
        if (tmp > 32w0) {
            tbl_complex10l12.apply();
        } else {
            tbl_complex10l12_0.apply();
        }
        if (tmp_1) {
            tbl_complex10l12_1.apply();
        } else {
            tbl_complex10l12_2.apply();
        }
        if (tmp_4) {
            tbl_complex10l13.apply();
        } else {
            tbl_complex10l15.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
