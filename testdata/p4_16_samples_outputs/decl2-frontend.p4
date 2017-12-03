control p() {
    bit<1> z_0;
    bit<1> x_2;
    bit<1> x_3;
    bit<1> y_1;
    @name("b") action b_0(in bit<1> x_0, out bit<1> y_0) {
        x_2 = x_0;
        z_0 = x_0 & x_2;
        y_0 = z_0;
    }
    apply {
        x_3 = 1w0;
        b_0(x_3, y_1);
    }
}

control simple();
package m(simple pipe);
.m(.p()) main;

