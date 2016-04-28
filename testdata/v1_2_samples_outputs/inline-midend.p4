control p() {
    bit<1> x_0;
    bit<1> z_0;
    bit<1> x_1;
    bit<1> y_0;
    bit<1> y0_0;
    bit<1> y0_1;
    @name("b") action b_0(bit<1> x, out bit<1> y) {
        x_0 = x;
        y0_0 = x & x_0;
        z_0 = y0_0;
        x_0 = z_0 & z_0;
        y0_1 = z_0 & z_0 & x_0;
        y = y0_1;
    }
    table tbl_b() {
        actions = {
            b_0;
        }
        const default_action = b_0(x_1, y_0);
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
m(p()) main;
