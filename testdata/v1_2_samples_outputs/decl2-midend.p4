control p() {
    bit<1> z_0;
    bit<1> x_2;
    bit<1> x_3;
    bit<1> y_1;
    bit<1> x_0;
    bit<1> y_0;
    @name("b") action b() {
        x_0 = x_3;
        x_2 = x_0;
        z_0 = x_0 & x_2;
        y_1 = y_0;
    }
    table tbl_b() {
        actions = {
            b();
        }
        const default_action = b();
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
.m(.p()) main;
