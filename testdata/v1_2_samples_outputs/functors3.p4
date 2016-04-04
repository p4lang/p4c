parser p1()(bit<1> b1) {
    state start {
        bit<1> z1 = b1;
    }
}

parser p()(bit<1> b, bit<1> c) {
    p1(b) p1i;
    state start {
        p1i.apply();
        bit<1> z = b & c;
    }
}

const bit<1> bv = 1w0;
parser nothing();
package m(nothing n);
m(p(bv, 1w1)) main;
