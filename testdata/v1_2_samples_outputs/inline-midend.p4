control p() {
    bit<1> x_0;
    bit<1> z_0;
    bit<1> x_1;
    bit<1> y_0;
    bit<1> x0_0;
    bit<1> y0_0;
    bit<1> x0_1;
    bit<1> y0_1;
    bit<1> x_2;
    bit<1> y_1;
    @name("b") action b() {
        x_2 = x_1;
        x0_0 = x_2;
        x_0 = x0_0;
        y0_0 = x0_0 & x_0;
        z_0 = y0_0;
        x0_1 = z_0 & z_0;
        x_0 = x0_1;
        y0_1 = x0_1 & x_0;
        y_1 = y0_1;
        y_0 = y_1;
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
m(p()) main;
