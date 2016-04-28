parser p1()(bit<2> a) {
    state start {
    }
}

parser p2()(bit<2> b, bit<2> c) {
    p1(2w0) @name("p1a") p1a_0;
    p1(b) @name("p1b") p1b_0;
    p1(c) @name("p1c") p1c_0;
    state start {
        p1a_0.apply();
        p1b_0.apply();
        p1c_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2(2w1, 2w2)) main;
