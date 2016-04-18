control p() {
    action a(bit<1> x0, out bit<1> y0) {
        bool hasReturned_0 = false;
        @name("x") bit<1> x_0;
        x_0 = x0;
        y0 = x0 & x_0;
    }
    action b(bit<1> x, out bit<1> y) {
        bool hasReturned_1 = false;
        @name("z") bit<1> z_0;
        a(x, z_0);
        a(z_0 & z_0, y);
    }
    apply {
        bool hasReturned = false;
        @name("z") bit<1> z_0;
        @name("x") bit<1> x_1;
        @name("y") bit<1> y_0;
        b(x_1, y_0);
    }
}

package m(p pipe);
m(p()) main;
