control p() {
    @name("x_1") bit<1> x_1_0_0;
    @name("y_0") bit<1> y_0_0_0;
    action b(in bit<1> x, out bit<1> y) {
        @name("z") bit<1> z_0_0;
        @name("x_0") bit<1> x_0_0_0;
        {
            x_0_0_0 = x;
            z_0_0 = x & x_0_0_0;
        }
    }
    apply {
        bool hasExited = false;
        b(x_1_0_0, y_0_0_0);
    }
}

package m(p pipe);
.m(.p()) main;
