parser p1()(bit<1> b1) {
    state start {
    }
}

parser p()(bit<1> b, bit<1> c) {
    p1(b) p1i;
    state start {
        p1i.apply();
    }
}

parser nothing();
package m(nothing n);
m(p(1w0, 1w1)) main;
