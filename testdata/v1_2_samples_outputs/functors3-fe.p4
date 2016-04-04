parser p1()(bit<1> b1) {
    state start {
        bit<1> z1 = b1;
    }
}

parser p1_0() {
    state start {
        bit<1> z1 = 1w0;
    }
}

parser p()(bit<1> b, bit<1> c) {
    p1(b) p1i;
    state start {
        p1i.apply();
        bit<1> z = b & c;
    }
}

parser p_0() {
    p1_0() p1i;
    state start {
        p1i.apply();
        bit<1> z = 1w0;
    }
}

const bit<1> bv = 1w0;
parser nothing();
package m(nothing n);
m(p_0()) main;
