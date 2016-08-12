parser p00() {
    state start {
        transition accept;
    }
}

parser nothing();
package m(nothing n);
m(p00()) main;
