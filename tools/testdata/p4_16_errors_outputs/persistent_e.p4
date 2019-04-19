control p()(bit<1> y) {
    apply {
    }
}

control q(in bit<1> z) {
    p(z) p1;
    apply {
        p1.apply();
    }
}

control simple(in bit<1> z);
package m(simple s);
m(q()) main;

