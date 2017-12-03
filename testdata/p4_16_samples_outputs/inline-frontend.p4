control p(out bit<1> y) {
    bit<1> x_0;
    bit<1> z_0;
    bit<1> x_1;
    @name("a") action a_0(in bit<1> x0_0, out bit<1> y0_0) {
        x_0 = x0_0;
        y0_0 = x0_0 & x_0;
    }
    @name("b") action b_0(in bit<1> x_2, out bit<1> y_0) {
        a_0(x_2, z_0);
        a_0(z_0 & z_0, y_0);
    }
    apply {
        x_1 = 1w1;
        b_0(x_1, y);
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

