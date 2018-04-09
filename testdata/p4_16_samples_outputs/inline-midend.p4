control p(out bit<1> y) {
    bit<1> x_3;
    @name("p.b") action b_0() {
        y = x_3 & x_3 & (x_3 & x_3) & (x_3 & x_3 & (x_3 & x_3));
    }
    @hidden action act() {
        x_3 = 1w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_b {
        actions = {
            b_0();
        }
        const default_action = b_0();
    }
    apply {
        tbl_act.apply();
        tbl_b.apply();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

