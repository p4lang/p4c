control p(out bit<1> y) {
    @name("p.x") bit<1> x_0;
    @name("p.z") bit<1> z_0;
    @name("p.x") bit<1> x_1;
    @name("p.b") action b(in bit<1> x, out bit<1> y_1) {
        {
            @name("p.x0") bit<1> x0 = x;
            @name("p.y0") bit<1> y0;
            x_0 = x0;
            y0 = x0 & x_0;
            z_0 = y0;
        }
        {
            @name("p.x0_1") bit<1> x0_1 = z_0;
            @name("p.y0_1") bit<1> y0_1;
            x_0 = x0_1;
            y0_1 = x0_1 & x_0;
            y_1 = y0_1;
        }
    }
    apply {
        x_1 = 1w1;
        b(x_1, y);
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

