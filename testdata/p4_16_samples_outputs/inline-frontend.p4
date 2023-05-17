control p(out bit<1> y) {
    @name("p.x") bit<1> x_1;
    @name("p.z") bit<1> z_0;
    @name("p.x") bit<1> x_2;
    @name("p.x0") bit<1> x0;
    @name("p.y0") bit<1> y0;
    @name("p.x0") bit<1> x0_1;
    @name("p.y0") bit<1> y0_1;
    @name("p.x") bit<1> x_4;
    @name("p.y") bit<1> y_0;
    @name("p.b") action b() {
        x_4 = x_2;
        x0 = x_4;
        x_1 = x0;
        y0 = x0 & x_1;
        z_0 = y0;
        x0_1 = z_0;
        x_1 = x0_1;
        y0_1 = x0_1 & x_1;
        y_0 = y0_1;
        y = y_0;
    }
    apply {
        x_2 = 1w1;
        b();
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;
