control p() {
    @name("x") bit<1> x_1_0;
    @name("y") bit<1> y_0_0;
    action b(bit<1> x, out bit<1> y) {
        @name("z") bit<1> z_0_0;
        @name("y0_0") bit<1> y0_0_0;
        @name("x") bit<1> x_0_0;
        @name("y0_1") bit<1> y0_1_0;
        {
            x_0_0 = x;
            y0_0_0 = x & x_0_0;
            z_0_0 = y0_0_0;
        }
        {
            x_0_0 = z_0_0 & z_0_0;
            y0_1_0 = z_0_0 & z_0_0 & x_0_0;
            y = y0_1_0;
        }
    }
    table tbl_b() {
        actions = {
            b;
        }
        const default_action = b(x_1_0, y_0_0);
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
m(p()) main;
