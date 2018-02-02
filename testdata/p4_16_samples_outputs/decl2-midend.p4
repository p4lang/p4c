control p() {
    bit<1> x_1;
    @name("p.b") action b_0() {
    }
    @hidden action act() {
        x_1 = 1w0;
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

control simple();
package m(simple pipe);
.m(.p()) main;

