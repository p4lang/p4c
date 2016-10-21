control p() {
    bit<1> z;
    bit<1> x_0;
    bit<1> x_1;
    bit<1> tmp_2;
    bit<1> tmp_3;
    bit<1> tmp_4;
    bit<1> x_2;
    bit<1> y_0;
    @name("b") action b_0() {
        x_2 = tmp_3;
        x_0 = tmp_3;
        tmp_2 = tmp_3 & tmp_3;
        z = tmp_3 & tmp_3;
        y_0 = tmp_3 & tmp_3;
        tmp_4 = tmp_3 & tmp_3;
    }
    action act() {
        x_1 = 1w0;
        tmp_3 = x_1;
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
