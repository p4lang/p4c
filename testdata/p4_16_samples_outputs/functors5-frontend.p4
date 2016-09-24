parser p1(out bit<2> w)(bit<2> a) {
    state start {
        w = 2w2;
        transition accept;
    }
}

parser p2(out bit<2> w)(bit<2> a) {
    @name("x") p1(a) x_0;
    state start {
        x_0.apply(w);
        transition accept;
    }
}

parser simple(out bit<2> w);
package m(simple n);
m(p2(2w1)) main;
