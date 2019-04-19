control p(out bit<1> y) {
    bit<1> x_1;
    @name("p.b") action b() {
        y = x_1 & x_1 & (x_1 & x_1) & (x_1 & x_1 & (x_1 & x_1));
    }
    @hidden action act() {
        x_1 = 1w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_b {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_act.apply();
        tbl_b.apply();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

