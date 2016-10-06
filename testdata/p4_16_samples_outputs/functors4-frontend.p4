parser p1(out bit<1> z1)(bit<1> b1) {
    state start {
        z1 = b1;
        transition accept;
    }
}

parser p(out bit<1> z) {
    @name("p1i") p1(1w0) p1i_0;
    state start {
        p1i_0.apply(z);
        transition accept;
    }
}

parser simple(out bit<1> z);
package m(simple n);
m(p()) main;
