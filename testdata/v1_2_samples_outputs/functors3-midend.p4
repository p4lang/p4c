parser p1()(bit<1> b1) {
    state start {
    }
}

parser p()(bit<1> b, bit<1> c) {
    p1(b) @name("p1i") p1i_0;
    state start {
        p1i_0.apply();
    }
}

parser nothing();
package m(nothing n);
m(p(1w0, 1w1)) main;
