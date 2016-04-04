parser p()(bit<1> b, bit<1> c) {
    state start {
        bit<1> z = b & c;
    }
}

parser p_0() {
    state start {
        bit<1> z = 1w0;
    }
}

const bit<1> bv = 1w0;
parser nothing();
package m(nothing n);
m(p_0()) main;
