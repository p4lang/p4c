extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @name("c.tmp_1") bit<32> tmp_1;
    @hidden action complex6l23() {
        r = 32w1;
    }
    @hidden action complex6l25() {
        r = 32w3;
    }
    @hidden action act() {
        tmp_1 = f(32w2);
    }
    @hidden action complex6l27() {
        r = 32w2;
    }
    @hidden action act_0() {
        tmp = f(32w2);
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_complex6l23 {
        actions = {
            complex6l23();
        }
        const default_action = complex6l23();
    }
    @hidden table tbl_complex6l25 {
        actions = {
            complex6l25();
        }
        const default_action = complex6l25();
    }
    @hidden table tbl_complex6l27 {
        actions = {
            complex6l27();
        }
        const default_action = complex6l27();
    }
    apply {
        tbl_act.apply();
        if (tmp > 32w0) {
            tbl_act_0.apply();
            if (tmp_1 < 32w2) {
                tbl_complex6l23.apply();
            } else {
                tbl_complex6l25.apply();
            }
        } else {
            tbl_complex6l27.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
