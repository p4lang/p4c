parser p00() {
    state start {
        bit<1> z = 1w0;
    }
}

parser nothing();
package m(nothing n);
m(p00()) main;
