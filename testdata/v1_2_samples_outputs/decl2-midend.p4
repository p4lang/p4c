control p() {
    action b(in bit<1> x, out bit<1> y) {
        bool hasReturned_0 = false;
        @name("z") bit<1> z_0;
        @name("x_0") bit<1> x_0_0;
        {
            x_0_0 = x;
            z_0 = x & x_0_0;
        }
    }
    apply {
        bool hasReturned = false;
        @name("z") bit<1> z_0;
        @name("x_0") bit<1> x_0_0;
        @name("x_1") bit<1> x_1_0;
        @name("y_0") bit<1> y_0_0;
        b(x_1_0, y_0_0);
    }
}

package m(p pipe);
.m(.p()) main;
