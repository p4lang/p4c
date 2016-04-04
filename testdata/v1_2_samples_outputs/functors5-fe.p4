parser p1()(bit<2> a) {
    state start {
        bit<2> w = 2w2;
    }
}

parser p1_0() {
    state start {
        bit<2> w = 2w2;
    }
}

parser p2()(bit<2> a) {
    p1(a) x;
    state start {
        x.apply();
    }
}

parser p2_0() {
    p1_0() x;
    state start {
        x.apply();
    }
}

parser nothing();
package m(nothing n);
m(p2_0()) main;
