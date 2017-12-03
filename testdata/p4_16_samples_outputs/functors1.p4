#include <core.p4>

parser p00() {
    state start {
        bit<1> z = 1w0 & 1w1;
        transition accept;
    }
}

parser nothing();
package m(nothing n);
m(p00()) main;

