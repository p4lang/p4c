control p() {
    bit<1> z;
    bit<1> x;
    bit<1> x_1;
    bit<1> y;
    @name("p.b") action b_0(in bit<1> x_2, out bit<1> y_1) {
        x = x_2;
        z = x_2 & x;
        y_1 = z;
    }
    apply {
        x_1 = 1w0;
        b_0(x_1, y);
    }
}

control simple();
package m(simple pipe);
.m(.p()) main;

