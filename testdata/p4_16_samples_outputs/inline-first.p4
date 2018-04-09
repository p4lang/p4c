control p(out bit<1> y) {
    action a(in bit<1> x0, out bit<1> y0) {
        bit<1> x = x0;
        y0 = x0 & x;
    }
    action b(in bit<1> x, out bit<1> y) {
        bit<1> z;
        a(x, z);
        a(z & z, y);
    }
    apply {
        bit<1> x = 1w1;
        b(x, y);
    }
}

control simple(out bit<1> y);
package m(simple pipe);
m(p()) main;

