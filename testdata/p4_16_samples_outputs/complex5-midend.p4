extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    @name("c.tmp") bit<32> tmp;
    @hidden action complex5l13() {
        r = 32w1;
    }
    @hidden action complex5l15() {
        r = 32w2;
    }
    @hidden action act() {
        tmp = f(32w2);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_complex5l13 {
        actions = {
            complex5l13();
        }
        const default_action = complex5l13();
    }
    @hidden table tbl_complex5l15 {
        actions = {
            complex5l15();
        }
        const default_action = complex5l15();
    }
    apply {
        tbl_act.apply();
        if (tmp > 32w0) {
            tbl_complex5l13.apply();
        } else {
            tbl_complex5l15.apply();
        }
    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
