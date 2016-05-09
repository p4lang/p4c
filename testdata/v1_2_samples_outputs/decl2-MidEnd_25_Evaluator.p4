control p() {
    bit<1> z_0;
    bit<1> x_2;
    bit<1> x_3;
    bit<1> y_1;
    @name("b") action b_0(in bit<1> x, out bit<1> y) {
        x_2 = x;
        z_0 = x & x_2;
    }
    table tbl_b() {
        actions = {
            b_0;
        }
        const default_action = b_0(x_3, y_1);
    }
    apply {
        tbl_b.apply();
    }
}

package m(p pipe);
.m(.p()) main;
