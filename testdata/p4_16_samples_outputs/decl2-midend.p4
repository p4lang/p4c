control p() {
    bit<1> tmp_3;
    @name("b") action b_0() {
    }
    action act() {
        tmp_3 = 1w0;
    }
    table tbl_act() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_b() {
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
