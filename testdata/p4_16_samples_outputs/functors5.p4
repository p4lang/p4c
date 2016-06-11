parser p1()(bit<2> a) {
    state start {
        bit<2> w = 2;
    }
}

parser p2()(bit<2> a) {
    p1(a) x;
    state start {
        x.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2(2w1)) main;
