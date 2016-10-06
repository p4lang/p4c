control p() {
    @name("x") bit<1> x_0;
    @name("z") bit<1> z_0;
    @name("x") bit<1> x_1;
    @name("y") bit<1> y_0;
    @name("a") action a_0(in bit<1> x0, out bit<1> y0) {
        bit<1> tmp;
        x_0 = x0;
        tmp = x0 & x_0;
        y0 = tmp;
    }
    @name("b") action b_0(in bit<1> x, out bit<1> y) {
        bit<1> tmp_0;
        bit<1> tmp_1;
        bit<1> tmp_2;
        bit<1> tmp_3;
        bit<1> tmp_4;
        tmp_0 = x;
        a_0(tmp_0, tmp_1);
        z_0 = tmp_1;
        tmp_2 = z_0 & z_0;
        tmp_3 = tmp_2;
        a_0(tmp_3, tmp_4);
        y = tmp_4;
    }
    bit<1> tmp_5;
    bit<1> tmp_6;
    apply {
        x_1 = 1w1;
        tmp_5 = x_1;
        b_0(tmp_5, tmp_6);
        y_0 = tmp_6;
    }
}

control simple();
package m(simple pipe);
m(p()) main;
