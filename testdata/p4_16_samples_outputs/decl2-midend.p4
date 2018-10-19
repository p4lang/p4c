control p() {
    bit<1> x_3;
    @name("p.b") action b() {
    }
    @hidden action act() {
        x_3 = 1w0;
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

control simple();
package m(simple pipe);
.m(.p()) main;

