#include <core.p4>

parser p()(bit<1> b, bit<1> c) {
    state start {
        bit<1> z = b & c;
        transition accept;
    }
}

const bit<1> bv = 1w0;
parser nothing();
package m(nothing n);
m(p(bv, 1w1)) main;

