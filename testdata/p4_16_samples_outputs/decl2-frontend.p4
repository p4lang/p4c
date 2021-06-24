control p() {
    @name("p.z") bit<1> z_0;
    @name("p.x_0") bit<1> x_0;
    @name("p.x_1") bit<1> x_3;
    @name("p.y_0") bit<1> y_0;
    @name("p.x") bit<1> x_4;
    @name("p.y") bit<1> y_2;
    @name("p.b") action b() {
        x_4 = x_3;
        x_0 = x_4;
        z_0 = x_4 & x_0;
        y_2 = z_0;
        y_0 = y_2;
    }
    apply {
        x_3 = 1w0;
        b();
    }
}

control simple();
package m(simple pipe);
.m(.p()) main;

