control p() {
    bit<1> x_0;
    bit<1> z_0;
    bit<1> x_1;
    bit<1> y_0;
    @name("b") action b(in bit<1> x_2, out bit<1> y_1) {
        x_0 = x_2;
        z_0 = x_2 & x_0;
        x_0 = z_0 & z_0;
        y_1 = z_0 & z_0 & x_0;
    }
    table tbl_b() {
        actions = {
            b;
        }
        const default_action = b(x_1, y_0);
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
m(p()) main;
