parser p()(bit<1> b, bit<1> c) {
    state start {
        transition accept;
    }
}

parser nothing();
package m(nothing n);
m(p(1w0, 1w1)) main;
